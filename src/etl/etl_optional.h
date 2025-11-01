#pragma once

/* 
Минималистичный вариант для optional по типу stl
*/

namespace etl
{
    template<typename T>
    class optional {
    private:
        alignas(T) unsigned char storage[sizeof(T)];
        bool _has_value = false;

        T* ptr() { 
            return reinterpret_cast<T*>(storage); 
        }
        
        const T* ptr() const { 
            return reinterpret_cast<const T*>(storage); 
        }
        
        void construct(const T& value) {
            new(storage) T(value);
            _has_value = true;
        }
        
        void destruct() {
            if (_has_value) {
                ptr()->~T();
                _has_value = false;
            }
        }

    public:
        // Конструкторы
        optional() = default;
        
        optional(const T& value) {
            construct(value);
        }
        
        // Копирующий конструктор
        optional(const optional& other) {
            if (other.has_value()) {
                construct(*other);
            }
        }
        
        // Деструктор
        ~optional() {
            destruct();
        }
        
        // ОСНОВНОЙ ИСПРАВЛЕННЫЙ ОПЕРАТОР ПРИСВАИВАНИЯ
        optional& operator=(const optional& other) {
            if (this != &other) {
                destruct();
                if (other._has_value) {
                    construct(*other);
                } else {
                    _has_value = false;
                }
            }
            return *this;
        }
        
        // Присвоение значения
        optional& operator=(const T& value) {
            destruct();
            construct(value);
            return *this;
        }
        
        // Доступ к значению
        T& operator*() {
            return *ptr();
        }
        
        const T& operator*() const {
            return *ptr();
        }
        
        T* operator->() {
            return ptr();
        }
        
        const T* operator->() const {
            return ptr();
        }
        
        // Проверка наличия значения
        explicit operator bool() const {
            return _has_value;
        }
        
        bool has_value() const {
            return _has_value;
        }
        
        // Получение значения или альтернативы
        T& value() {
            return *ptr();
        }
        
        const T& value() const {
            return *ptr();
        }
        
        T value_or(const T& default_value) const {
            return _has_value ? *ptr() : default_value;
        }
        
        // Модификация
        void reset() {
            destruct();
        }
        
        template<typename... Args>
        void emplace(Args&&... args) {
            destruct();
            new(storage) T(std::forward<Args>(args)...);
            _has_value = true;
        }
    };
}

/* 
Пример использования:
cpp
struct SensorData {
    float temperature;
    float humidity;
    uint32_t timestamp;
    
    SensorData() : temperature(0), humidity(0), timestamp(0) {}
    SensorData(float t, float h, uint32_t ts) : temperature(t), humidity(h), timestamp(ts) {}
};

void setup() {
    Serial.begin(9600);
    
    Serial.println("=== Optional Demo ===");
    
    // Базовое использование
    atl::optional<int> maybe_number;
    Serial.print("Has value: "); Serial.println(maybe_number.hasValue());
    
    maybe_number = 42;
    Serial.print("Has value: "); Serial.println(maybe_number.hasValue());
    Serial.print("Value: "); Serial.println(*maybe_number);
    
    // Использование с структурами
    atl::optional<SensorData> sensor_reading;
    
    if (!sensor_reading) {
        Serial.println("No sensor data yet");
    }
    
    sensor_reading.emplace(23.5f, 65.2f, millis());
    
    if (sensor_reading) {
        Serial.print("Temp: "); Serial.println(sensor_reading->temperature);
        Serial.print("Humidity: "); Serial.println(sensor_reading->humidity);
    }
    
    // ValueOr пример
    atl::optional<String> maybe_string;
    String result = maybe_string.valueOr("default");
    Serial.print("Result: "); Serial.println(result);
    
    // Использование в функциях
    atl::optional<float> temp = readTemperature();
    if (temp) {
        Serial.print("Temperature: "); Serial.println(*temp);
    }
}

// Пример функции, которая может вернуть atl::optional
atl::optional<float> readTemperature() {
    // Симуляция чтения с датчика
    if (random(100) > 30) { // 70% успешных чтений
        return atl::optional<float>(random(150, 250) / 10.0f);
    }
    return atl::optional<float>(); // Ошибка чтения
}

void loop() {
    // Чтение данных с обработкой опциональности
    auto data = readSensorData();
    
    if (data) {
        Serial.print("Success: ");
        Serial.print(data->temperature);
        Serial.print("C, ");
        Serial.print(data->humidity);
        Serial.println("%");
    } else {
        Serial.println("Sensor read failed");
    }
    
    delay(5000);
}

atl::optional<SensorData> readSensorData() {
    // Симуляция чтения с датчиков
    bool success = random(100) > 20; // 80% успеха
    
    if (success) {
        return SensorData(
            random(200, 250) / 10.0f, // 20.0-25.0C
            random(400, 700) / 10.0f, // 40.0-70.0%
            millis()
        );
    }
    
    return atl::optional<SensorData>(); // Ошибка
}

*/