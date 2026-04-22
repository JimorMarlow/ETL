#pragma once
// Настройки проекта с сохранение в EEPROM или операционную систему
// Для уменьшения циклов записи на флеш память используется отложенная запись на величину 
// update_timeout. 
// - В рабочем цикле нужно обязательно вызывать tick() - если в течении таймаута не было изменения данных,
// они запишутся в файловую систему. Для оптимизации записи использутеся библиотека 
// gyverlibs/FileData@^1.0.3 от Alex Gyver
// - ВНИМАНИЕ: перед первым чтением данных обязательно вызвать init(), можно в самом начале setup() 
// - Если нужно сохранить данные без задержки (например, перед перезагрузкой) - вызывать save()
// Примеры использования:
// - в этом файле внизу
// - src\etl\etl_test.cpp test_settings()

#include <Arduino.h>
#include <FileData.h>
#include "etl_littlefs.h"
#include "etl_optional.h"

// Инициализация файловой системы
namespace etl
{
    namespace settings 
    {
        // Детектор метода trace()
        template<typename T, typename = void>
        struct has_trace : std::false_type {};
        
        template<typename T>
        struct has_trace<T, std::void_t<decltype(std::declval<T>().trace())>> : std::true_type {};
        
        template<typename T>
        constexpr bool has_trace_v = has_trace<T>::value;

        // Режимы трассировки
        enum class trace_mode_t : uint8_t
        {
            SILENT = 0,    // Тихий режим - никаких сообщений
            ERRORS = 1,    // Только ошибки
            VERBOSE = 2    // Все сообщения
        };

        // Глобальный режим трассировки для всех настроек
        static inline trace_mode_t _trace_mode = trace_mode_t::SILENT;

        // Установить режим трассировки
        inline void set_trace_mode(trace_mode_t mode) { _trace_mode = mode; }
        // Получить режим трассировки
        inline trace_mode_t get_trace_mode() { return _trace_mode; }

        // Идентификаторы источников изменений настроек
        enum class sender_id : uint8_t
        {
            broadcast = 0,  // Широковещательная рассылка (системный)
            system = 1,     // Главное приложение (датчики, автоматика)
            webui,          // Веб-интерфейс (обновление от пользователя)
            setup,          // Сервер настроек
            view,           // Пользовательский интерфейс
            wifi,           // Менеджер wifi соединения
            mqtt,           // MQTT протокол
            user1,          // Пользовательское расширение 1
            user2,          // Пользовательское расширение 2
            user3,          // Пользовательское расширение 3
            count           // Счётчик для валидации (не используется клиентами)
        };

        String to_string(sender_id id); // NOTE: после добавления нового sender_id - нужно обновить эту функцию

        // Тип callback'а для уведомления об изменениях
        using change_callback = std::function<void(sender_id)>;

        // Базовый класс для организации подписки на изменения
        class notify
        {
        protected:
            // Статический массив указателей на callback'и (nullptr = не зарегистрирован)
            change_callback* _notify[static_cast<uint8_t>(sender_id::count)] = {nullptr};

        public:
            virtual ~notify()
            {
                for (uint8_t i = 0; i < static_cast<uint8_t>(sender_id::count); ++i) {
                    delete _notify[i];
                    _notify[i] = nullptr;
                }
            }

            // Зарегистрировать callback для источника изменений
            // Возвращает true при успешной регистрации
            // ВАЖНО: клиент обязан вызвать unsubscribe() при уничтожении объекта
            bool subscribe(sender_id id, change_callback cb)
            {
                uint8_t idx = static_cast<uint8_t>(id);
                if (idx >= static_cast<uint8_t>(sender_id::count)) return false;
                if (id == sender_id::broadcast) return false;  // broadcast не регистрируется

                // Удаляем предыдущий callback если был
                delete _notify[idx];
                _notify[idx] = nullptr;

                // Создаём новый callback в куче
                _notify[idx] = new (std::nothrow) change_callback(std::move(cb));
                return _notify[idx] != nullptr;
            }

            // Удалить callback источника
            // Возвращает true если callback был найден и удалён
            bool unsubscribe(sender_id id)
            {
                uint8_t idx = static_cast<uint8_t>(id);
                if (idx >= static_cast<uint8_t>(sender_id::count)) return false;

                if (_notify[idx] != nullptr) {
                    delete _notify[idx];
                    _notify[idx] = nullptr;
                    return true;
                }
                return false;
            }

            // Проверка регистрации callback'а для источника
            bool is_subscribed(sender_id id) const
            {
                uint8_t idx = static_cast<uint8_t>(id);
                if (idx >= static_cast<uint8_t>(sender_id::count)) return false;
                return _notify[idx] != nullptr;
            }

        protected:
            // Рассылка нотификаций всем подписчикам кроме исключённого источника
            void notify_changed(sender_id excluded_source)
            {
                uint8_t count = static_cast<uint8_t>(sender_id::count);

                for (uint8_t i = 0; i < count; ++i)
                {
                    sender_id id = static_cast<sender_id>(i);

                    // Если source != broadcast - пропускаем broadcast
                    if (excluded_source != sender_id::broadcast && id == sender_id::broadcast) continue;

                    // Пропускаем источник изменений (кроме broadcast - он получает всё)
                    if (excluded_source != sender_id::broadcast && id == excluded_source) continue;

                    // Проверяем наличие callback'а
                    if (_notify[i] != nullptr) {
                        // Вызываем callback напрямую (исключения на ESP отключены)
                        (*_notify[i])(excluded_source);
                    }
                }
            }
        };

        // Управление всеми настройками
        template<typename T>
        class data : public notify
        {
        protected:
            String   _path; // Путь к файлу для сохранения настроек
            FileData _fd;   // Управление загрузкой данных в файловую система
            T        _data; // структура данных

        public:
            // Путь к настройкам для этой структуры и интервал записи после обновленя в мс
            data(const String& path, uint16_t update_timeout = 5000, const T& default_data = T() )
            : _path(path)
            , _fd (&LittleFS, path.c_str(), 'B', &_data, sizeof(_data), update_timeout)
            , _data(default_data)
            {}

            virtual ~data() = default;

            bool init()    // Инициализировать все настройки и считать значения из памяти или записать по-умолчанию в первый раз
            {
                if (_trace_mode >= trace_mode_t::VERBOSE) {
                    Serial.printf("etl::setting::data init for <%s>:\n", _path.c_str());
                }
                if(bool fs_available = etl::little_fs::begin(); !fs_available)
                {
                    if (_trace_mode >= trace_mode_t::ERRORS) {
                        Serial.printf("Error: LittleFS not available for settings: %s\n", _path.c_str());
                    }
                    return false;
                }

                if (_trace_mode >= trace_mode_t::VERBOSE) {
                    Serial.printf("etl::setting::data read <%s> - ", _path.c_str());
                }

                // прочитать данные из файла в переменную
                // при первом запуске в файл запишутся данные из структуры
                FDstat_t stat = _fd.read();

                if (_trace_mode >= trace_mode_t::VERBOSE) {
                    switch (stat) {
                        case FD_FS_ERR: Serial.println("FS Error");
                            break;
                        case FD_FILE_ERR: Serial.println("FS File Open Error");
                            break;
                        case FD_WRITE: Serial.println("Data Write");
                            break;
                        case FD_ADD: Serial.println("Data Add");
                            break;
                        case FD_READ: Serial.println("Data Read");
                            break;
                        default:
                            Serial.println();
                            break;
                    }
                } else if (_trace_mode >= trace_mode_t::ERRORS) {
                    if (stat == FD_FS_ERR) {
                        Serial.println("FS Error");
                    } else if (stat == FD_FILE_ERR) {
                        Serial.println("FS File Open Error");
                    }
                }

                // Вызываем trace() если он есть у структуры T
                if constexpr (has_trace_v<T>) {
                    if (_trace_mode >= trace_mode_t::VERBOSE) {
                        _data.trace();
                    }
                }

                return true;
            }

            FDstat_t tick()    // Вызывать в loop() для контроля отложенной записи
            {
                return _fd.tick();
            }

            // Получить настройки
            T get() const { return _data; }
            // Изменить настройки в памяти (отложенная запись по таймату, чтобы запоминать только последние данные)
            // update_now == true - записать без задержки
            bool set(const T& data, bool update_now = false)
            {
                _data = data;
                if(update_now) {
                    auto fd_result = _fd.updateNow();
                    return (fd_result == FD_WRITE || fd_result == FD_NO_DIF);
                } else {
                    _fd.update();
                }
                return true;
            }

            // Изменить настройки с указанием источника (для фильтрации нотификаций)
            // source - источник изменений (не получит нотификацию)
            // Если source == broadcast - нотификацию получат ВСЕ подписчики
            // Возвращает результат базового метода set(), callback'и не влияют
            bool set(const T& data, sender_id source, bool update_now = false)
            {
                // Вызываем базовый set
                bool result = set(data, update_now);
                
                // Рассылаем нотификации (результат не влияет на возвращаемое значение)
                notify_changed(source);
                
                return result;
            }

            // Получить ссылку на настройки (для прямого изменения)
            T& ref() { return _data; }

            // Принудительное сохранение
            bool save() {
                auto fd_result = _fd.updateNow();
                return (fd_result == FD_WRITE || fd_result == FD_NO_DIF);
            }
        };

        // Расширенный шаблонный класс для хранения настроек приложения
        // T - тип данных настроек
        // Параметры конструктора: path, update_timeout, trace_name, default_data
        template<typename T>
        class app_data : public data<T>
        {
        protected:
            bool _is_initialized = false;
            String _trace_name;
            T      _default; // значение по умолчанию, запоминается в init()

            // Получить строку для трассировки
            String get_trace_prefix() const
            {
                if (!_trace_name.isEmpty()) {
                    return String("[") + _trace_name + "] ";
                }
                return "";
            }

        public:
            // Конструктор передаёт параметры в базовый класс data
            app_data(const String& path, uint16_t update_timeout = 5000, const String& trace_name = "", const T& default_data = T())
            : data<T>(path, update_timeout, default_data)
            , _trace_name(trace_name)
            {}

            // Проверка инициализации
            bool is_initialized() const { return _is_initialized; }

            /**
             * @brief Сбросить настройки к значениям по умолчанию
             * @return true при успешном сохранении
             */
            virtual bool reset()
            {
                String prefix = get_trace_prefix();
                if (!prefix.isEmpty()) {
                    Serial.print(prefix);
                    Serial.println(F("reset() ..."));
                }

                auto loaded_info = data<T>::get();
                if (!prefix.isEmpty()) {
                    Serial.print(prefix);
                    Serial.println(F("loaded from memory:"));
                }
                if constexpr (has_trace_v<T>) {
                    loaded_info.trace();
                }

                // Устанавливаем значения по умолчанию и сохраняем
                data<T>::set(_default);
                bool reset_result = data<T>::save();

                if (!prefix.isEmpty()) {
                    Serial.print(prefix);
                    Serial.print(F("reset: "));
                    Serial.println(reset_result ? F("OK") : F("FAILED"));
                }
                if (reset_result) {
                    if constexpr (has_trace_v<T>) {
                        _default.trace();
                    }
                }

                return reset_result;
            }

            /**
             * @brief Инициализация данных приложения
             * @param default_info Значения по умолчанию
             * @param reset_to_default Сбросить к значениям по умолчанию после инициализации
             * @return true при успешной инициализации
             */
            virtual bool init(const T& default_info, bool reset_to_default = false)
            {
                String prefix = get_trace_prefix();
                if (!prefix.isEmpty()) {
                    Serial.print(prefix);
                    Serial.println(F("init() ..."));
                }

                if (etl::little_fs::begin())
                {
                    // Создание директории для файла настроек
                    etl::little_fs::create_dir(data<T>::_path);
                }

                // Сохранение настроек в постоянной памяти
                if (!is_initialized())
                {
                    // Запоминаем значения по умолчанию
                    _default = default_info;

                    bool result = data<T>::init();
                    if (!prefix.isEmpty()) {
                        Serial.print(prefix);
                        Serial.print(F("init() result: "));
                        Serial.println(result ? F("OK") : F("FAILED"));
                    }

                    if (result && reset_to_default)
                    {
                        reset();
                    }

                    _is_initialized = true;
                    return result;
                }

                if (!prefix.isEmpty()) {
                    Serial.print(prefix);
                    Serial.println(F("init() result: ALREADY INITED"));
                }
                return true;
            }

            /**
             * @brief Установить новые значения
             * @param info Новые настройки
             * @param sender Указывает, кто изменил настройки
             * @return true при успешном сохранении
             */
            virtual bool set(const T& info, sender_id sender)
            {
                String prefix = get_trace_prefix();
                if (!prefix.isEmpty()) {
                    Serial.printf("%sset(), sender_id = %d\n", prefix.c_str(), static_cast<uint8_t>(sender));
                }

                if (is_initialized())
                {
                    bool result = data<T>::set(info, sender);
                    return result;
                }

                if (!prefix.isEmpty()) {
                    Serial.print(prefix);
                    Serial.println(F("set() error: data not initialized"));
                }
                return false;
            }

            /**
             * @brief Считать текущие значения
             * @return optional с данными или пустой optional
             */
            virtual etl::optional<T> get()
            {
                String prefix = get_trace_prefix();
                if (!prefix.isEmpty()) {
                    Serial.print(prefix);
                    Serial.println(F("get()"));
                }

                if (is_initialized())
                {
                    return data<T>::get();
                }
                else
                {
                    if (!prefix.isEmpty()) {
                        Serial.print(prefix);
                        Serial.println(F("get(): data not initialized, returning empty optional"));
                    }
                    return etl::nullopt;
                }
            }

            /**
             * @brief Вызывать в loop() для отложенного сохранения в постоянную память
             */
            virtual void tick()
            {
                if (is_initialized())
                {
                    auto result = data<T>::tick();
                    String prefix = get_trace_prefix();
                    if (!prefix.isEmpty()) {
                        if (result == FD_WRITE)
                        {
                            Serial.print(prefix);
                            Serial.println(F("tick(): pending write successful"));
                        }
                        else if (result == FD_NO_DIF)
                        {
                            Serial.print(prefix);
                            Serial.println(F("tick(): pending write skipped - data not changed"));
                        }
                    }
                }
            }
        };
    }//..settings
}//..etl

/// Использование
/*

// settings.h
namespace settings
{
    const String kitchen_light_path = "/kitchen_light.cfg";
    const uint16_t kitchen_light_update_delay = 30000;  // 30s
    struct kitchen_light_t
    {
        bool    state      = false;    // Велючен свет или нет
        float   brightness = 1.0;      // Целевой уровень яркости
        char[20] topic = "kitchen_light/state";

        void trace() {
            Serial.println("=== kitchen_light_t settings ===");
            Serial.printf("state = %s\n", state ? "ON" : "OFF");
            Serial.printf("brightness = %g\n", brightness);
            Serial.printf("topic = %s\n", topic);
            Serial.println("========================");
        }
    };
}// settings

// test.h
#include "settings.h"
class control
{
    etl::settings::data<settings::kitchen_light_t> _settings;   // Сохранение настроек в постоянной памяти

    public:
    bool init() { return _settings.init(); }
    void tick();    // call in main loop() for delayed update
}

// test.cpp
control::control()
: _settings(settings::kitchen_light_path, settings::kitchen_light_update_delay)
{
}

void control::tick() // true - fade timer finished
{
    _settings.tick();
}

float control::brightness() const {
    return _settings.get().brightness;
}
void control::set_brightness(float brightness_value)
{
    auto data = _settings.get();
    data.brightness = etl::clamp<float>(brightness_value, 0.0, 1.0);
    _settings.set(data);
}

// Settings
control light;  // глобальный экземпляр настроек

void setup()
{
    light.init();
}

// main.cpp
void loop()
{
  light.tick();
}

// ========================
// Работа с callback'ами
// ========================

// Глобальные настройки
etl::settings::data<settings::kitchen_light_t> kitchen_settings("/kitchen.cfg");

// --- 1. Простая подписка с лямбдой ---
void setup_callbacks()
{
    kitchen_settings.init();
    
    // Подписка из веб-сервера
    kitchen_settings.subscribe(etl::settings::sender_id::webui, [](etl::settings::sender_id source) {
        Serial.println("Settings changed - updating web UI");
        update_web_ui();
    });
    
    // Подписка из модуля сенсоров
    kitchen_settings.subscribe(etl::settings::sender_id::system, [](etl::settings::sender_id source) {
        Serial.printf("System notified by source: %d\n", static_cast<uint8_t>(source));
    });
}

// --- 2. Подписка из класса с захватом this ---
class web_server {
    etl::settings::data<settings::kitchen_light_t>& _settings;
    
public:
    web_server(etl::settings::data<settings::kitchen_light_t>& settings) 
    : _settings(settings) 
    {
        _settings.subscribe(etl::settings::sender_id::webui, [this](etl::settings::sender_id source) {
            this->on_settings_changed(source);
        });
    }
    
    ~web_server() {
        // ОБЯЗАТЕЛЬНО: удалить callback при уничтожении объекта
        _settings.unsubscribe(etl::settings::sender_id::webui);
    }
    
    void on_settings_changed(etl::settings::sender_id source) {
        auto data = _settings.get();
        Serial.printf("Web server: brightness = %f, source = %d\n", data.brightness, static_cast<uint8_t>(source));
    }
};

// --- 3. Изменение настроек с указанием источника ---
void on_web_slider_change(float value)
{
    auto data = kitchen_settings.get();
    data.brightness = value;
    // webui callback НЕ вызовется, system callback вызовется
    kitchen_settings.set(data, etl::settings::sender_id::webui);
}

void on_sensor_update(float value)
{
    auto data = kitchen_settings.get();
    data.brightness = value;
    // system callback НЕ вызовется, webui callback вызовется
    kitchen_settings.set(data, etl::settings::sender_id::system);
}

// --- 4. Широковещательная рассылка всем ---
void broadcast_change()
{
    auto data = kitchen_settings.get();
    // ВСЕ callback'и вызовутся (и webui, и system)
    kitchen_settings.set(data, etl::settings::sender_id::broadcast);
}

// --- 5. Проверка регистрации callback'а ---
void check_subscription()
{
    if (kitchen_settings.is_subscribed(etl::settings::sender_id::webui)) {
        Serial.println("Web UI is subscribed");
    }
}

// --- 6. Отписка и повторная подписка ---
void toggle_subscription(bool enable)
{
    if (enable) {
        kitchen_settings.subscribe(etl::settings::sender_id::webui, [](etl::settings::sender_id) {
            Serial.println("Web UI notified");
        });
    } else {
        kitchen_settings.unsubscribe(etl::settings::sender_id::webui);
    }
}

// --- 7. Использование пользовательских расширений ---
// Клиентские программы могут использовать user1, user2, user3
// без переопределения стандартных идентификаторов
void setup_custom_callbacks()
{
    // Алиас для своего модуля
    constexpr auto my_module = etl::settings::sender_id::user1;
    
    kitchen_settings.subscribe(my_module, [](etl::settings::sender_id source) {
        Serial.println("Custom module received update");
    });
    
    // Изменение из своего модуля
    auto data = kitchen_settings.get();
    data.state = true;
    kitchen_settings.set(data, my_module);  // my_module callback НЕ вызовется
}

// --- 8. Безопасное использование с динамическими объектами ---
class dynamic_module {
    etl::settings::data<settings::kitchen_light_t>& _settings;
    bool _active = true;
    
public:
    dynamic_module(etl::settings::data<settings::kitchen_light_t>& settings)
    : _settings(settings)
    {
        _settings.subscribe(etl::settings::sender_id::user2, [this](etl::settings::sender_id source) {
            if (_active) {
                Serial.println("Dynamic module received update");
            }
        });
    }
    
    void deactivate() {
        _active = false;
        // Отписываемся при деактивации
        _settings.unsubscribe(etl::settings::sender_id::user2);
    }
    
    ~dynamic_module() {
        // Двойная защита: отписываемся в деструкторе
        _settings.unsubscribe(etl::settings::sender_id::user2);
    }
};

// ========================
// Использование app_data
// ========================

// 1. Определяем структуру настроек
struct kitchen_light_t
{
    bool    state      = false;
    float   brightness = 1.0;
    char    topic[20]  = "kitchen_light/state";

    void trace() {
        Serial.println("=== kitchen_light_t settings ===");
        Serial.printf("state = %s\n", state ? "ON" : "OFF");
        Serial.printf("brightness = %g\n", brightness);
        Serial.printf("topic = %s\n", topic);
        Serial.println("========================");
    }
};

// 2. Используем — алиас для удобства (не обязательно)
using kitchen_light_data_t = etl::settings::app_data<kitchen_light_t>;

// 3. Создаём экземпляр с параметрами: path, update_timeout, trace_name
kitchen_light_data_t lc_data("/settings/light_control.cfg", 30000, "light_control::data");

void setup()
{
    kitchen_light_t default_settings;
    lc_data.init(default_settings, false);

    // Подписка на изменения
    lc_data.subscribe(etl::settings::sender_id::webui, [](etl::settings::sender_id source) {
        Serial.println("Light settings changed from web UI");
    });
}

void loop()
{
    lc_data.tick();  // Отложенная запись
}

void change_brightness(float value)
{
    auto current = lc_data.get();
    if (current) {
        current->brightness = value;
        lc_data.set(*current, etl::settings::sender_id::system);
    }
}

*/

