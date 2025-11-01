#pragma once 
// Данные для датчиков температуры, влажности etc
#include "Arduino.h"

namespace etl {
namespace sensor {
namespace data {

    struct th_t {
        float temperature   = 0.0;
        float humidity      = 0.0;
        
        th_t() = default;
        th_t(float t, float hum) : temperature {t}, humidity {hum} {}
        constexpr th_t(std::nullptr_t) : temperature(0.0), humidity(0.0) {} // конструктор для static_cast<th_t>{0}

        // Оператор сложения (создает новый объект)
        th_t operator+(const th_t& rhs) const {
            return th_t{temperature + rhs.temperature, humidity + rhs.humidity};
        }

        // Оператор деления на число (создает новый объект)
        th_t operator/(float x) const {
            return th_t{temperature / x, humidity / x};
        }

        th_t operator/(int x) const {
            return th_t{temperature / x, humidity / x};
        }

        th_t operator/(size_t x) const {
            return th_t{temperature / x, humidity / x};
        }

        // Оператор += (поэлементное прибавление к текущему объекту)
        th_t& operator+=(const th_t& rhs) {
            temperature += rhs.temperature;
            humidity += rhs.humidity;
            return *this;
        }

        th_t& operator-=(const th_t& rhs) {
            temperature -= rhs.temperature;
            humidity -= rhs.humidity;
            return *this;
        }
    };

}//namespace data
}//namespace sensor
}// namespace etl