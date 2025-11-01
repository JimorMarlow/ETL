#pragma once
#include "Arduino.h"

// etl::color_t 
// Тестовый класс для хранения цвета и проверки работы lookup таблицы со сложным типом данных с интерполяцией цвета
// Ключевые особенности:
// Класс color_t - содержит RGB компоненты и операторы для интерполяции
// Специализированный color_lookup - переопределяет interpolate_custom для правильной интерполяции цветов
// Тестовые данные - соответствие температуры цветам от синего до бордового
// Различные режимы - тестирование интерполяции, ближайшего значения, clamp и экстраполяции
// Визуализация - вывод спектра цветов для наглядной проверки

#include "etl_lookup.h"
#include "etl_utility.h"

namespace etl {

class color_t {
public:
    uint8_t r, g, b;

    // Конструкторы
    constexpr color_t() : r(0), g(0), b(0) {}
    constexpr color_t(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
    
    // Конструктор из одного значения (серый цвет)
    constexpr explicit color_t(uint8_t gray) : r(gray), g(gray), b(gray) {}

    // Минималистичный конструктор из строки "#RRGGBB"
    color_t(const char* hex) : r(0), g(0), b(0) {
        if (hex && strlen(hex) == 7 && hex[0] == '#') {
            auto parse = [](char c) -> uint8_t {
                if ('0' <= c && c <= '9') return c - '0';
                if ('A' <= c && c <= 'F') return c - 'A' + 10;
                if ('a' <= c && c <= 'f') return c - 'a' + 10;
                return 0;
            };
            r = (parse(hex[1]) << 4) | parse(hex[2]);
            g = (parse(hex[3]) << 4) | parse(hex[4]);
            b = (parse(hex[5]) << 4) | parse(hex[6]);
        }
    }
    
    // Операторы для интерполяции
    color_t operator+(const color_t& other) const {
        return color_t(
            static_cast<uint8_t>(etl::clamp<int32_t>(int32_t(r) + int32_t(other.r), 0, 255)),
            static_cast<uint8_t>(etl::clamp<int32_t>(int32_t(g) + int32_t(other.g), 0, 255)),
            static_cast<uint8_t>(etl::clamp<int32_t>(int32_t(b) + int32_t(other.b), 0, 255))
        );
    }
    
    color_t operator-(const color_t& other) const {
        return color_t(
            static_cast<uint8_t>(etl::clamp<int32_t>(int32_t(r) - int32_t(other.r), 0, 255)),
            static_cast<uint8_t>(etl::clamp<int32_t>(int32_t(g) - int32_t(other.g), 0, 255)),
            static_cast<uint8_t>(etl::clamp<int32_t>(int32_t(b) - int32_t(other.b), 0, 255))
        );
    }
    
    color_t operator*(double ratio) const {
        return color_t(
            static_cast<uint8_t>(etl::clamp(r * ratio, 0.0, 255.0)),
            static_cast<uint8_t>(etl::clamp(g * ratio, 0.0, 255.0)),
            static_cast<uint8_t>(etl::clamp(b * ratio, 0.0, 255.0))
        );
    }
    
    color_t operator/(double divisor) const {
        if (divisor == 0.0) return *this;
        return color_t(
            static_cast<uint8_t>(r / divisor),
            static_cast<uint8_t>(g / divisor),
            static_cast<uint8_t>(b / divisor)
        );
    }
    
    // Операторы сравнения
    bool operator==(const color_t& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
    
    bool operator!=(const color_t& other) const {
        return !(*this == other);
    }
    
    // Методы для удобства
    String to_string() const {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "RGB(%d,%d,%d)", r, g, b);
        return String(buffer);
    }

    String to_hex_string() const {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", r, g, b);
        return String(buffer);
    }
    
    // Предопределенные цвета
    static constexpr color_t BLACK() { return color_t(0, 0, 0); }
    static constexpr color_t WHITE() { return color_t(255, 255, 255); }
    static constexpr color_t RED() { return color_t(255, 0, 0); }
    static constexpr color_t GREEN() { return color_t(0, 255, 0); }
    static constexpr color_t BLUE() { return color_t(0, 0, 255); }
    static constexpr color_t YELLOW() { return color_t(255, 255, 0); }
    static constexpr color_t CYAN() { return color_t(0, 255, 255); }
    static constexpr color_t MAGENTA() { return color_t(255, 0, 255); }
    static constexpr color_t MAROON() { return color_t(128, 0, 0); }
};

} // namespace etl

///////////////////////////////////////////////////////////////////////
// Специализированный lookup для палитный цветов
namespace etl {

// Специализация interpolate_custom для color_t
template <typename T, typename Container>
class color_lookup : public lookup<T, color_t, Container> {
protected:
    virtual color_t interpolate_custom(const lookup_t<T, color_t>& a, 
                              const lookup_t<T, color_t>& b, 
                              const T& raw) const override {
        if (a.raw == b.raw) return a.value;
        
        double ratio = static_cast<double>(static_cast<int32_t>(raw) - static_cast<int32_t>(a.raw)) 
                     / static_cast<double>(static_cast<int32_t>(b.raw) - static_cast<int32_t>(a.raw));
                     
     //   ratio = static_cast<double>((raw - a.raw) / (b.raw - a.raw));
        
        // Безопасная интерполяция с clamp
        auto interpolate_component = [ratio](uint8_t a_comp, uint8_t b_comp) -> uint8_t {
            double result = a_comp + ratio * (static_cast<int32_t>(b_comp) - static_cast<int32_t>(a_comp));
            return static_cast<uint8_t>(etl::clamp(result, 0.0, 255.0));
        };
        
        return color_t(
            interpolate_component(a.value.r, b.value.r),
            interpolate_component(a.value.g, b.value.g),
            interpolate_component(a.value.b, b.value.b)
        );
    }

public:
    using lookup<T, color_t, Container>::lookup; // Наследуем конструкторы
};

// Вспомогательные функции для создания color_lookup
template<typename T, typename Container>
color_lookup<T, Container> make_color_lookup(const Container& container,
                                            lookup_mode mode = lookup_mode::INTERPOLATE,
                                            bounds_mode bounds = bounds_mode::CLAMP) {
    return color_lookup<T, Container>(container, mode, bounds);
}

template<typename T, size_t N>
color_lookup<T, etl::array<lookup_t<T, color_t>>> make_color_lookup(const lookup_t<T, color_t> (&arr)[N],
                                                                   lookup_mode mode = lookup_mode::INTERPOLATE,
                                                                   bounds_mode bounds = bounds_mode::CLAMP) {
    return color_lookup<T, etl::array<lookup_t<T, color_t>>>(arr, mode, bounds);
}

} // namespace etl

// Тестовые данные и тест в файле etl_test.cpp
