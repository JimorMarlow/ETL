#pragma once
// Настройки проекта с сохранение в EEPROM или операционную систему

#include <Arduino.h>
#include <FileData.h>
#include <LittleFS.h>

// Инициализация файловой системы
namespace etl
{
    class little_fs 
    {
    private:
        static bool _initialized;
        static bool _mount_failed;
        
    public:
        static bool begin() {
            if (_mount_failed) {
                return false; // Уже пробовали и не получилось
            }
            
            if (!_initialized) {
                Serial.print("Initializing LittleFS... ");
                
                // Для ESP32-C3 часто нужно явно указать форматирование при первом запуске
                if (!LittleFS.begin()) { // true = format если монтирование не удалось
                    Serial.println("FAILED - trying with formatting...");
                    
                    // Пробуем с форматированием
                    if (!format()) {
                        Serial.println("FAILED even with formatting!");
                        _mount_failed = true;
                        return false;
                    }
                    else {
                        // Проверяем, что файловая система действительно работает
                        if (testFileSystem()) {
                            Serial.println("LittleFS test: OK");
                        } else {
                            Serial.println("LittleFS test: FAILED");
                            _initialized = false;
                            _mount_failed = true;
                            return false;
                        }
                    }
                }
                
                _initialized = true;
                Serial.println("OK");
            }
            return _initialized;
        }
        
        static bool isReady() {
            return _initialized;
        }
        
        static bool format() {
            Serial.println("Formatting LittleFS...");
            bool result = LittleFS.format();
            _initialized = false;
            _mount_failed = false;
            if (result) {
                Serial.println("Format: OK");
                return begin(); // Пробуем снова после форматирования
            } else {
                Serial.println("Format: FAILED");
                return false;
            }
        }
        
    private:
        static bool testFileSystem() {
            // Простая проверка работы файловой системы
            const char* test_file = "/test.tmp";
            
            // Пробуем создать файл
            File file = LittleFS.open(test_file, "w");
            if (!file) {
                Serial.println("  Cannot create test file");
                return false;
            }
            
            // Пробуем записать
            if (file.write('T') != 1) {
                Serial.println("  Cannot write to test file");
                file.close();
                LittleFS.remove(test_file);
                return false;
            }
            file.close();
            
            // Пробуем прочитать
            file = LittleFS.open(test_file, "r");
            if (!file) {
                Serial.println("  Cannot read test file");
                LittleFS.remove(test_file);
                return false;
            }
            
            char data = file.read();
            file.close();
            
            // Удаляем тестовый файл
            LittleFS.remove(test_file);
            
            if (data != 'T') {
                Serial.println("  Test file content mismatch");
                return false;
            }
            
            return true;
        }
    };

    namespace settings 
    {
        // Детектор метода trace()
        template<typename T, typename = void>
        struct has_trace : std::false_type {};
        
        template<typename T>
        struct has_trace<T, std::void_t<decltype(std::declval<T>().trace())>> : std::true_type {};
        
        template<typename T>
        constexpr bool has_trace_v = has_trace<T>::value;

        // Управление всеми настройками
        template<typename T>
        class data 
        {
            String   _path; // Путь к файлу для сохранения настроек            
            FileData _fd;   // Управление загрузкой данных в файловую система
            T        _data; // структура данных
            

        public:
            // Путь к настройкам для этой структуры и интервал записи после обновленя в мс
            data(const String& path, uint16_t update_timeout = 5000)
            : _path(path)
            , _fd (&LittleFS, path.c_str(), 'B', &_data, sizeof(_data), update_timeout) 
            , _data()
            {}
            virtual ~data() = default;

            bool init()    // Инициализировать все настройки и считать значения из памяти или записать по-умолчанию в первый раз
            {
                if(bool fs_available = etl::little_fs::begin(); !fs_available) 
                {
                    Serial.printf("Error: LittleFS not available for settings: %s\n", _path.c_str());
                    return false;
                }
                
                Serial.printf("etl::setting::data init <%s> - ", _path.c_str());

                // прочитать данные из файла в переменную
                // при первом запуске в файл запишутся данные из структуры
                FDstat_t stat = _fd.read();

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

                // Вызываем trace() если он есть у структуры T
                if constexpr (has_trace_v<T>) {
                    _data.trace();
                }
                    
                return true;
            }

            void tick()    // Вызывать в loop() для контроля отложенной записи
            {
                _fd.tick();
            }

            // Получить настройки
            T get() const { return _data; }
            // Изменить настройки в памяти (отложенная запись по таймату, чтобы запоминать только последние данные)
            // update_now == true - записать без задержки
            bool set(const T& data, bool update_now = false)
            {
                _data = data;
                if(update_now) {
                    return _fd.updateNow() == FD_WRITE;
                } else {
                    _fd.update();
                }
                return true;  
            }

            // Получить ссылку на настройки (для прямого изменения)
            T& ref() { return _data; }

            // Принудительное сохранение
            bool save() {
                return _fd.updateNow() == FD_WRITE;
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

// main.cpp
void loop() 
{
  light.tick();
}

*/
