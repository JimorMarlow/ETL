#pragma once
#include "Arduino.h"
#include "etl_array.h"
#include "etl_vector.h"

namespace etl {

// Lookup таблица с возможностью интерполяции значений между точками
template <typename T, typename V>
struct lookup_t
{
    T raw   = T{0};   // сырое показание датчика
    V value = V{0};   // физическое значение
};

// Перечисление для режимов поиска
enum class lookup_mode {
    NEAREST,      // Ближайшее значение
    INTERPOLATE   // Интерполяция между точками
};

// Перечисление для обработки границ
enum class bounds_mode {
    CLAMP,        // Брать значение границы
    EXTRAPOLATE   // Экстраполировать по двум крайним точкам
};

// Вспомогательные метафункции для определения наличия метода clear()
namespace detail {
    template<typename C, typename = void>
    struct has_clear : std::false_type {};
    
    template<typename C>
    struct has_clear<C, std::void_t<decltype(std::declval<C>().clear())>> : std::true_type {};
    
    template<typename C>
    constexpr bool has_clear_v = has_clear<C>::value;
}

// Основной класс lookup таблицы
template <typename T, typename V, typename Container = etl::array<lookup_t<T, V>>>
class lookup {
private:
    Container table_;
    lookup_mode mode_ = lookup_mode::INTERPOLATE;
    bounds_mode bounds_ = bounds_mode::CLAMP;
        
    // Определяем направление сортировки
    bool is_ascending() const {
        if (table_.size() < 2) return true;
        return table_[0].raw < table_[1].raw;
    }
    
    // Бинарный поиск в отсортированном массиве
    size_t find_index(const T& raw) const {
        if (table_.empty()) return 0;
        
        size_t left = 0;
        size_t right = table_.size() - 1;
        
        if (is_ascending()) {
            if(raw <= table_[left].raw) return 0;    // less of left boundary
            if(raw >= table_[right].raw) return table_.size();  // greater of right boundary
            while (left <= right) {
                size_t mid = left + (right - left) / 2;
                if (table_[mid].raw == raw) return mid;
                if (table_[mid].raw < raw) left = mid + 1;
                else right = mid - 1;
            }
            return left; // left указывает на позицию вставки
        } else {
            // для убывающего порядка
            if(raw >= table_[left].raw) return 0;    // greater of left boundary
            if(raw <= table_[right].raw) return table_.size();  // less of right boundary
            while (left <= right) {
                size_t mid = left + (right - left) / 2;
                if (table_[mid].raw == raw) return mid;
                if (table_[mid].raw > raw) left = mid + 1;
                else right = mid - 1;
            }
            return left;
        }
    }
    
    // Интерполяция между двумя точками
    V interpolate(const lookup_t<T, V>& a, const lookup_t<T, V>& b, const T& raw) const {
        if (a.raw == b.raw) return a.value;
        
        // Для простых числовых типов
        if constexpr (std::is_arithmetic_v<V>) {
            double ratio = static_cast<double>(raw - a.raw) / static_cast<double>(b.raw - a.raw);
            return static_cast<V>(a.value + ratio * (b.value - a.value));
        } else {
            // Для сложных типов нужны определенные операторы
            return interpolate_custom(a, b, raw);
        }
    }
    
protected:
    // Интерполяция для пользовательских типов
    virtual V interpolate_custom(const lookup_t<T, V>& a, const lookup_t<T, V>& b, const T& raw) const {
        // Пользователь должен определить операторы для своего типа V
        // или специализировать эту функцию
        double ratio = static_cast<double>(raw - a.raw) / static_cast<double>(b.raw - a.raw);
        return a.value + (b.value - a.value) * ratio;
    }

public:
    // Конструкторы
    lookup() = default;
    
    lookup(const Container& table, 
           lookup_mode mode = lookup_mode::INTERPOLATE, 
           bounds_mode bounds = bounds_mode::CLAMP) 
        : table_(table), mode_(mode), bounds_(bounds) {}
    
    template<size_t N>
    lookup(const lookup_t<T, V> (&arr)[N],
           lookup_mode mode = lookup_mode::INTERPOLATE,
           bounds_mode bounds = bounds_mode::CLAMP)
        : table_(arr), mode_(mode), bounds_(bounds) {}
    
    lookup(lookup_mode mode, bounds_mode bounds)
        : mode_(mode), bounds_(bounds) {}

    virtual ~lookup() = default;
    
    // Основная рабочая функция
    V raw_to_value(const T& raw) const {
        if (table_.empty()) return V{};
        if (table_.size() == 1) return table_[0].value;
        
        size_t index = find_index(raw);
        
        // Обработка границ
        if (index == 0) {
            // raw меньше первого элемента
            if (bounds_ == bounds_mode::CLAMP) {
                return table_[0].value;
            } else { // EXTRAPOLATE
                if (table_.size() >= 2) {
                    return interpolate(table_[0], table_[1], raw);
                }
                return table_[0].value;
            }
        }
        
        if (index >= table_.size()) {
            // raw больше последнего элемента
            if (bounds_ == bounds_mode::CLAMP) {
                return table_.back().value;
            } else { // EXTRAPOLATE
                if (table_.size() >= 2) {
                    return interpolate(table_[table_.size() - 2], table_.back(), raw);
                }
                return table_.back().value;
            }
        }
        
        // Точное совпадение
        if (table_[index].raw == raw) {
            return table_[index].value;
        }
        
        // Между двумя точками
        size_t prev_index = index - 1;
        
        if (mode_ == lookup_mode::NEAREST) {
            // Ближайшее значение
            T dist_prev = std::abs(raw - table_[prev_index].raw);
            T dist_next = std::abs(raw - table_[index].raw);
            return (dist_prev <= dist_next) ? table_[prev_index].value : table_[index].value;
        } else { // INTERPOLATE
            // Интерполяция между точками
            return interpolate(table_[prev_index], table_[index], raw);
        }
    }
    
    // Set/Get методы для режимов
    void set_lookup_mode(lookup_mode mode) { mode_ = mode; }
    lookup_mode get_lookup_mode() const { return mode_; }
    
    void set_bounds_mode(bounds_mode bounds) { bounds_ = bounds; }
    bounds_mode get_bounds_mode() const { return bounds_; }
    
    // Получение минимального и максимального raw
    T min_raw() const {
        if (table_.empty()) return T{};
        if (is_ascending()) return table_[0].raw;
        return table_.back().raw;
    }
    
    T max_raw() const {
        if (table_.empty()) return T{};
        if (is_ascending()) return table_.back().raw;
        return table_[0].raw;
    }
    
    // Проверка на валидность
    bool valid() const {
        return !table_.empty();
    }
    
    // Размер таблицы
    size_t size() const {
        return table_.size();
    }
    
    // Доступ к таблице
    const Container& table() const {
        return table_;
    }
    
    // Установка новой таблицы
    void set_table(const Container& table) {
        table_ = table;
    }
};

// Вспомогательные функции для создания lookup таблиц
template<typename T, typename V, typename Container>
lookup<T, V, Container> make_lookup(const Container& container,
                                    lookup_mode mode = lookup_mode::INTERPOLATE,
                                    bounds_mode bounds = bounds_mode::CLAMP) {
    return lookup<T, V, Container>(container, mode, bounds);
}

template<typename T, typename V, size_t N>
lookup<T, V, etl::array<lookup_t<T, V>>> make_lookup(const lookup_t<T, V> (&arr)[N],
                                                    lookup_mode mode = lookup_mode::INTERPOLATE,
                                                    bounds_mode bounds = bounds_mode::CLAMP) {
    return lookup<T, V, etl::array<lookup_t<T, V>>>(arr, mode, bounds);
}

// Специализированные псевдонимы для удобства
template<typename T, typename V>
using array_lookup = lookup<T, V, etl::array<lookup_t<T, V>>>;

template<typename T, typename V>
using pgm_lookup = lookup<T, V, pgm::array<lookup_t<T, V>>>;

template<typename T, typename V>
using vector_lookup = lookup<T, V, etl::vector<lookup_t<T, V>>>;

} // namespace etl

// Пример использования с разными контейнерами:
// пример с тестами смотри etl_test.cpp test_lookup()

// #include "etl_lookup.h"
// // Пример данных
// const etl::lookup_t<uint16_t, float> voltage_data[] = {
//     {0, 0.0}, {1024, 3.3}, {2048, 5.0}, {3072, 7.5}, {4095, 10.0}
// };
// // PROGMEM данные
// const etl::lookup_t<uint16_t, float> pgm_voltage_data[] PROGMEM = {
//     {0, 0.0}, {100, 1.0}, {200, 2.0}, {300, 3.0}
// };
// void setup() {
//     Serial.begin(115200);
   
//     // 1. Использование с etl::array
//     etl::array voltage_array(voltage_data);
//     auto lookup1 = etl::make_lookup<uint16_t, float>(voltage_array);
    
//     // 2. Использование с массивом (автоматическое создание etl::array)
//     auto lookup2 = etl::make_lookup<uint16_t, float>(voltage_data);
    
//     // 3. Использование с pgm::array
//     pgm::array pgm_array(pgm_voltage_data);
//     auto lookup3 = etl::make_lookup<uint16_t, float>(pgm_array);
    
//     // 4. Использование с etl::vector
//     etl::vector<etl::lookup_t<uint16_t, float>, 10> voltage_vector;
//     voltage_vector.push_back({0, 0.0});
//     voltage_vector.push_back({100, 1.0});
//     voltage_vector.push_back({200, 2.0});
    
//     auto lookup4 = etl::make_lookup<uint16_t, float>(voltage_vector);
    
//     // 5. Прямое создание с указанием контейнера
//     etl::lookup<uint16_t, float, etl::array<etl::lookup_t<uint16_t, float>>> lookup5(voltage_data);
    
//     // Тестирование
//     Serial.println("=== Lookup Tests ===");
//     Serial.printf("Lookup1 (1500): %.2f\n", lookup1.raw_to_value(1500));
//     Serial.printf("Lookup2 (2500): %.2f\n", lookup2.raw_to_value(2500));
//     Serial.printf("Lookup3 (150): %.2f\n", lookup3.raw_to_value(150));
//     Serial.printf("Lookup4 (50): %.2f\n", lookup4.raw_to_value(50));
    
//     // Изменение режимов в runtime
//     Serial.println("\n=== Changing Modes ===");
//     lookup1.set_lookup_mode(etl::lookup_mode::NEAREST);
//     Serial.printf("Nearest (1500): %.2f\n", lookup1.raw_to_value(1500));
    
//     lookup1.set_bounds_mode(etl::bounds_mode::EXTRAPOLATE);
//     Serial.printf("Extrapolate (5000): %.2f\n", lookup1.raw_to_value(5000));
    
//     // Использование псевдонимов
//     Serial.println("\n=== Using Aliases ===");
//     etl::pgm_lookup<uint16_t, float> pgm_lookup(pgm_voltage_data);
//     Serial.printf("PGM Lookup (250): %.2f\n", pgm_lookup.raw_to_value(250));
// }

// void loop() {
// }

// Ключевые изменения:
// Шаблонный контейнер - поддержка etl::array, pgm::array, etl::vector и других
// Динамические параметры - режимы поиска как поля класса, а не параметры шаблона
// Set/Get методы - возможность изменения режимов в runtime
// Гибкие конструкторы - разные способы инициализации
// Вспомогательные функции - make_lookup для удобного создания
// Псевдонимы типов - для часто используемых комбинаций
// класс поддерживает любые контейнеры с индексным доступом и позволяет динамически менять параметры поиска

