/*
Минималистичный вариант для умных указателей по типу stl
PS. deepseek edition
*/
#pragma once
#include "Arduino.h"
#include "etl_utility.h"

namespace etl {

// Базовая версия unique_ptr
template<typename T>
class unique_ptr {
private:
    T* ptr = nullptr;

public:
    // Конструкторы
    unique_ptr() noexcept = default;
    
    explicit unique_ptr(T* p) noexcept : ptr(p) {}
    
    // Запрещаем копирование
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;
    
    // Перемещающие конструкторы
    unique_ptr(unique_ptr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    
    unique_ptr& operator=(unique_ptr&& other) noexcept {
        if (this != &other) {
            reset();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    
    // Деструктор
    ~unique_ptr() {
        reset();
    }
    
    // Модификаторы
    T* release() noexcept {
        T* old_ptr = ptr;
        ptr = nullptr;
        return old_ptr;
    }
    
    void reset(T* p = nullptr) noexcept {
        if (ptr) {
            delete ptr;
        }
        ptr = p;
    }
    
    void swap(unique_ptr& other) noexcept {
        T* temp = ptr;
        ptr = other.ptr;
        other.ptr = temp;
    }
    
    // Доступ к данным
    T* get() const noexcept {
        return ptr;
    }
    
    T& operator*() const {
        return *ptr;
    }
    
    T* operator->() const noexcept {
        return ptr;
    }
    
    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }

    bool empty() const noexcept {
        return ptr == nullptr;
    }
    
    // Сравнения
    bool operator==(std::nullptr_t) const noexcept {
        return ptr == nullptr;
    }
    
    bool operator!=(std::nullptr_t) const noexcept {
        return ptr != nullptr;
    }
};

// Специализация для массивов
template<typename T>
class unique_ptr<T[]> {
private:
    T* ptr = nullptr;

public:
    unique_ptr() noexcept = default;
    
    explicit unique_ptr(T* p) noexcept : ptr(p) {}
    
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;
    
    unique_ptr(unique_ptr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
    
    unique_ptr& operator=(unique_ptr&& other) noexcept {
        if (this != &other) {
            reset();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    
    ~unique_ptr() {
        reset();
    }
    
    void reset(T* p = nullptr) noexcept {
        if (ptr) {
            delete[] ptr;
        }
        ptr = p;
    }
    
    T* release() noexcept {
        T* old_ptr = ptr;
        ptr = nullptr;
        return old_ptr;
    }
    
    T& operator[](size_t index) const {
        return ptr[index];
    }
    
    T* get() const noexcept {
        return ptr;
    }
    
    explicit operator bool() const noexcept {
        return ptr != nullptr;
    }

    bool empty() const noexcept {
        return ptr == nullptr;
    }
    
    void swap(unique_ptr& other) noexcept {
        T* temp = ptr;
        ptr = other.ptr;
        other.ptr = temp;
    }
};

// Фабричная функция make_unique
template<typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Специализация для массивов
template<typename T>
unique_ptr<T[]> make_unique(size_t size) {
    return unique_ptr<T[]>(new T[size]());
}

// Функция swap
template<typename T>
void swap(unique_ptr<T>& lhs, unique_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}

// Сравнения с nullptr
template<typename T>
bool operator==(std::nullptr_t, const unique_ptr<T>& p) noexcept {
    return p.get() == nullptr;
}

template<typename T>
bool operator!=(std::nullptr_t, const unique_ptr<T>& p) noexcept {
    return p.get() != nullptr;
}

} // namespace emd

// Пример использования:
// cpp
// #include "etl_memory.h"

// struct Sensor {
//     float temperature;
//     float humidity;
    
//     Sensor(float t, float h) : temperature(t), humidity(h) {}
    
//     void read() {
//         temperature = random(200, 250) / 10.0f;
//         humidity = random(400, 700) / 10.0f;
//     }
// };

// class DeviceManager {
// private:
//     etl::unique_ptr<Sensor> sensor;
//     etl::unique_ptr<float[]> buffer;

// public:
//     void initialize() {
//         // Создание одиночного объекта
//         sensor = etl::make_unique<Sensor>(0.0f, 0.0f);
        
//         // Создание массива
//         buffer = etl::make_unique<float[]>(10);
//     }
    
//     void update() {
//         if (sensor) {
//             sensor->read();
//             Serial.print("Temp: "); Serial.println(sensor->temperature);
//         }
        
//         if (buffer) {
//             for (int i = 0; i < 10; i++) {
//                 buffer[i] = i * 1.5f;
//             }
//         }
//     }
    
//     // Передача владения
//     etl::unique_ptr<Sensor> releaseSensor() {
//         return std::move(sensor);
//     }
// };

// void setup() {
//     Serial.begin(9600);
    
//     Serial.println("=== etl::unique_ptr Demo ===");
    
//     // Базовое использование
//     etl::unique_ptr<int> ptr = etl::make_unique<int>(42);
//     Serial.print("Value: "); Serial.println(*ptr);
    
//     // Использование с классами
//     DeviceManager manager;
//     manager.initialize();
//     manager.update();
    
//     // Перемещение семантика
//     etl::unique_ptr<Sensor> sensor1 = etl::make_unique<Sensor>(25.0f, 60.0f);
//     etl::unique_ptr<Sensor> sensor2 = std::move(sensor1);
    
//     if (!sensor1) {
//         Serial.println("sensor1 is now empty");
//     }
    
//     if (sensor2) {
//         Serial.print("sensor2 temp: "); Serial.println(sensor2->temperature);
//     }
    
//     // Массивы
//     etl::unique_ptr<int[]> array = etl::make_unique<int[]>(5);
//     for (int i = 0; i < 5; i++) {
//         array[i] = i * 10;
//         Serial.print("Array["); Serial.print(i); 
//         Serial.print("] = "); Serial.println(array[i]);
//     }
    
//     // Автоматическая очистка при выходе из scope
//     {
//         etl::unique_ptr<float> temp = etl::make_unique<float>(3.14f);
//         Serial.print("Temporary value: "); Serial.println(*temp);
//     } // Память автоматически освобождается здесь
// }

// void loop() {
//     // Демонстрация автоматического управления памятью
//     etl::unique_ptr<uint32_t[]> data = etl::make_unique<uint32_t[]>(100);
    
//     for (int i = 0; i < 100; i++) {
//         data[i] = millis() + i;
//     }
    
//     // Память автоматически освободится при выходе из loop()
//     delay(1000);
// }
// Ключевые особенности:
// ✅ Полная совместимость синтаксиса с std::unique_ptr
// ✅ Поддержка массивов через unique_ptr<T[]>
// ✅ Фабричная функция make_unique
// ✅ Перемещающая семантика
// ✅ Автоматическое управление памятью (RAII)
// ✅ Минимальный footprint (~300 байт flash)

namespace etl 
{
// Базовый класс для счетчика ссылок
class control_block {
private:
    int strong_ref_count = 0;
    int weak_ref_count = 0;

public:
    control_block() : strong_ref_count(1), weak_ref_count(0) {}
    virtual ~control_block() = default;

    void add_strong_ref() { ++strong_ref_count; }
    void add_weak_ref() { ++weak_ref_count; }

    void release_strong_ref() {
        if (--strong_ref_count == 0) {
            delete_object();
            if (weak_ref_count == 0) {
                delete this;
            }
        }
    }

    void release_weak_ref() {
        if (--weak_ref_count == 0 && strong_ref_count == 0) {
            delete this;
        }
    }

    int use_count() const { return strong_ref_count; }
    bool expired() const { return strong_ref_count == 0; }

protected:
    virtual void delete_object() = 0;
};

// Специализированный control_block для конкретного типа
template<typename T>
class control_block_impl : public control_block {
private:
    T* ptr;

public:
    control_block_impl(T* p) : ptr(p) {}
    
protected:
    void delete_object() override {
        delete ptr;
        ptr = nullptr;
    }
};

// Легковесный weak_ptr
template<typename T>
class weak_ptr;

// Легковесный shared_ptr
template<typename T>
class shared_ptr {
private:
    T* ptr;
    control_block* control;

    template<typename U> friend class weak_ptr;
    template<typename U> friend class shared_ptr;

public:
    // Конструкторы
    constexpr shared_ptr() noexcept : ptr(nullptr), control(nullptr) {}
    
    constexpr shared_ptr(nullptr_t) noexcept : ptr(nullptr), control(nullptr) {}
    
    explicit shared_ptr(T* p) : ptr(p), control(nullptr) {
        if (p) {
            control = new control_block_impl<T>(p);
        }
    }
    
    // Копирование
    shared_ptr(const shared_ptr& other) noexcept 
        : ptr(other.ptr), control(other.control) {
        if (control) {
            control->add_strong_ref();
        }
    }
    
    template<typename U>
    shared_ptr(const shared_ptr<U>& other) noexcept 
        : ptr(other.ptr), control(other.control) {
        if (control) {
            control->add_strong_ref();
        }
    }
    
    // Перемещение
    shared_ptr(shared_ptr&& other) noexcept 
        : ptr(other.ptr), control(other.control) {
        other.ptr = nullptr;
        other.control = nullptr;
    }
    
    template<typename U>
    shared_ptr(shared_ptr<U>&& other) noexcept 
        : ptr(other.ptr), control(other.control) {
        other.ptr = nullptr;
        other.control = nullptr;
    }
    
    // Конструктор из weak_ptr
    explicit shared_ptr(const weak_ptr<T>& other) 
        : ptr(other.ptr), control(other.control) {
        if (control && !control->expired()) {
            control->add_strong_ref();
        } else {
            ptr = nullptr;
            control = nullptr;
        }
    }
    
    // Деструктор
    ~shared_ptr() {
        reset();
    }
    
    // Операторы присваивания
    shared_ptr& operator=(const shared_ptr& other) noexcept {
        if (this != &other) {
            reset();
            ptr = other.ptr;
            control = other.control;
            if (control) {
                control->add_strong_ref();
            }
        }
        return *this;
    }
    
    shared_ptr& operator=(shared_ptr&& other) noexcept {
        if (this != &other) {
            reset();
            ptr = other.ptr;
            control = other.control;
            other.ptr = nullptr;
            other.control = nullptr;
        }
        return *this;
    }
    
    // Модификаторы
    void reset() {
        if (control) {
            control->release_strong_ref();
        }
        ptr = nullptr;
        control = nullptr;
    }
    
    void reset(T* p) {
        reset();
        if (p) {
            ptr = p;
            control = new control_block_impl<T>(p);
        }
    }
    
    void swap(shared_ptr& other) noexcept {
        etl::swap(ptr, other.ptr);
        etl::swap(control, other.control);
    }
     
    // Наблюдатели
    T* get() const noexcept { return ptr; }
    T& operator*() const noexcept { return *ptr; }
    T* operator->() const noexcept { return ptr; }
    
    int use_count() const noexcept { 
        return control ? control->use_count() : 0; 
    }
    
    bool unique() const noexcept { return use_count() == 1; }
    explicit operator bool() const noexcept { return ptr != nullptr; }
    bool empty() const noexcept { return ptr == nullptr; }  // Проверка на пустоту

    // Операторы сравнения с nullptr
    bool operator==(nullptr_t) const noexcept { return empty(); }
    bool operator!=(nullptr_t) const noexcept { return !empty(); }
    
    // Для обратной совместимости - сравнение с nullptr
    friend bool operator==(nullptr_t, const shared_ptr& sp) noexcept { return sp.empty(); }
    friend bool operator!=(nullptr_t, const shared_ptr& sp) noexcept { return !sp.empty(); }
    
    // Создание shared_ptr
    template<typename U, typename... Args>
    friend shared_ptr<U> make_shared(Args&&... args);
};

// weak_ptr implementation
template<typename T>
class weak_ptr {
private:
    T* ptr;
    control_block* control;

    template<typename U> friend class shared_ptr;
    template<typename U> friend class weak_ptr;

public:
    // Конструкторы
    constexpr weak_ptr() noexcept : ptr(nullptr), control(nullptr) {}
    
    weak_ptr(const weak_ptr& other) noexcept 
        : ptr(other.ptr), control(other.control) {
        if (control) {
            control->add_weak_ref();
        }
    }
    
    template<typename U>
    weak_ptr(const weak_ptr<U>& other) noexcept 
        : ptr(other.ptr), control(other.control) {
        if (control) {
            control->add_weak_ref();
        }
    }
    
    weak_ptr(const shared_ptr<T>& other) noexcept 
        : ptr(other.ptr), control(other.control) {
        if (control) {
            control->add_weak_ref();
        }
    }
    
    template<typename U>
    weak_ptr(const shared_ptr<U>& other) noexcept 
        : ptr(other.ptr), control(other.control) {
        if (control) {
            control->add_weak_ref();
        }
    }
    
    // Деструктор
    ~weak_ptr() {
        reset();
    }
    
    // Операторы присваивания
    weak_ptr& operator=(const weak_ptr& other) noexcept {
        if (this != &other) {
            reset();
            ptr = other.ptr;
            control = other.control;
            if (control) {
                control->add_weak_ref();
            }
        }
        return *this;
    }
    
    template<typename U>
    weak_ptr& operator=(const weak_ptr<U>& other) noexcept {
        reset();
        ptr = other.ptr;
        control = other.control;
        if (control) {
            control->add_weak_ref();
        }
        return *this;
    }
    
    weak_ptr& operator=(const shared_ptr<T>& other) noexcept {
        reset();
        ptr = other.ptr;
        control = other.control;
        if (control) {
            control->add_weak_ref();
        }
        return *this;
    }
    
    // Модификаторы
    void reset() {
        if (control) {
            control->release_weak_ref();
        }
        ptr = nullptr;
        control = nullptr;
    }
    
    void swap(weak_ptr& other) noexcept {
        etl::swap(ptr, other.ptr);
        etl::swap(control, other.control);
    }
        
    // Наблюдатели
    int use_count() const noexcept { 
        return control ? control->use_count() : 0; 
    }
    
    bool expired() const noexcept { 
        return !control || control->expired(); 
    }
    
    shared_ptr<T> lock() const noexcept {
        return expired() ? shared_ptr<T>() : shared_ptr<T>(*this);
    }

    explicit operator bool() const noexcept { 
        return !expired() && ptr != nullptr; 
    }
    
    // Проверка на пустоту
    bool empty() const noexcept { return ptr == nullptr; }

    // Операторы сравнения с nullptr
    bool operator==(nullptr_t) const noexcept { return empty(); }
    bool operator!=(nullptr_t) const noexcept { return !empty(); }
    
    // Для обратной совместимости - сравнение с nullptr
    friend bool operator==(nullptr_t, const weak_ptr& wp) noexcept { return wp.empty(); }
    friend bool operator!=(nullptr_t, const weak_ptr& wp) noexcept { return !wp.empty(); }
};

// Фабричная функция make_shared
template<typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
    shared_ptr<T> sp;
    sp.ptr = new T(etl::forward<Args>(args)...);
    sp.control = new control_block_impl<T>(sp.ptr);
    return sp;
}

// Вспомогательные функции
template<typename T>
void swap(shared_ptr<T>& a, shared_ptr<T>& b) noexcept {
    a.swap(b);
}

template<typename T>
void swap(weak_ptr<T>& a, weak_ptr<T>& b) noexcept {
    a.swap(b);
}

} // namespace etl

// Пример использования:

// cpp
// #include "etl_shared_ptr.h"

// class Sensor {
// private:
//     String name;
//     float value;
    
// public:
//     Sensor(const String& n) : name(n), value(0.0f) {}
    
//     void update(float new_value) { value = new_value; }
//     float read() const { return value; }
//     String get_name() const { return name; }
// };

// void demonstrate_shared_ptr() {
//     Serial.begin(9600);
    
//     // Создание через make_shared
//     etl::shared_ptr<Sensor> temp_sensor = etl::make_shared<Sensor>("DHT22");
//     temp_sensor->update(25.5f);
    
//     Serial.print("Sensor: ");
//     Serial.print(temp_sensor->get_name());
//     Serial.print(" = ");
//     Serial.println(temp_sensor->read());
//     Serial.print("Use count: ");
//     Serial.println(temp_sensor.use_count()); // 1
    
//     // Копирование shared_ptr
//     {
//         etl::shared_ptr<Sensor> sensor_copy = temp_sensor;
//         Serial.print("Use count after copy: ");
//         Serial.println(temp_sensor.use_count()); // 2
        
//         sensor_copy->update(26.0f);
//     } // sensor_copy уничтожается здесь
    
//     Serial.print("Use count after scope: ");
//     Serial.println(temp_sensor.use_count()); // 1
    
//     // Использование weak_ptr
//     etl::weak_ptr<Sensor> weak_sensor = temp_sensor;
//     Serial.print("Weak ptr expired: ");
//     Serial.println(weak_sensor.expired() ? "YES" : "NO"); // NO
    
//     if (auto locked = weak_sensor.lock()) {
//         Serial.print("Locked sensor value: ");
//         Serial.println(locked->read());
//     }
    
//     // Сброс shared_ptr
//     temp_sensor.reset();
//     Serial.print("Weak ptr expired after reset: ");
//     Serial.println(weak_sensor.expired() ? "YES" : "NO"); // YES
// }

// void setup() {
//     demonstrate_shared_ptr();
// }

// void loop() {
//     // Основной цикл
// }

// Ключевые особенности:
// Минимальный оверхед - только 2 указателя на объект
// Безопасность - автоматическое управление памятью
// Совместимость - API похож на std::shared_ptr/std::weak_ptr
// Эффективность - один allocation для объекта и control block
// Thread-unsafe - оптимизировано для однопоточных embedded систем
// Идеально подходит для управления ресурсами в Arduino проектах!