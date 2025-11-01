#pragma once
#include "Arduino.h"
// Реализация класса-обертки etl::array для статического массива
// - Только константный доступ - данные нельзя изменить через интерфейс
// - Копирование разрешено - копируются только указатель и размер
// - Легковесный - занимает только 2 указателя (данные + размер)
// - Безопасный - не управляет памятью, только предоставляет доступ
// - Совместимость с STL - итераторы и ranged for
// - Constexpr готовность - может использоваться в compile-time

namespace etl {

template<typename T>
class array {
private:
    const T* data_;
    size_t size_;

public:
    // Конструкторы
    constexpr array(const T* ptr, size_t size) noexcept 
        : data_(ptr), size_(size) {}
    
    template<size_t N>
    constexpr array(const T (&arr)[N]) noexcept 
        : data_(arr), size_(N) {}
    
    // Конструктор копирования
    constexpr array(const array& other) noexcept 
        : data_(other.data_), size_(other.size_) {}
    
    // Оператор присваивания копированием
    constexpr array& operator=(const array& other) noexcept {
        if (this != &other) {
            data_ = other.data_;
            size_ = other.size_;
        }
        return *this;
    }
    
    // Перемещение (тоже разрешаем, так как данные неизменяемы)
    constexpr array(array&& other) noexcept 
        : data_(other.data_), size_(other.size_) {
    }
    
    constexpr array& operator=(array&& other) noexcept {
        if (this != &other) {
            data_ = other.data_;
            size_ = other.size_;
        }
        return *this;
    }

    // Доступ к элементам (только константный)
    constexpr const T& operator[](size_t index) const noexcept {
        return data_[index];
    }
    
    constexpr const T& at(size_t index) const {
        if (index >= size_) {
            Serial.println("[ERROR] etl:aaray::at - out of range");            
            // Для constexpr функций нельзя использовать исключения,
            // поэтому просто возвращаем первый элемент при ошибке
            return (index < size_) ? data_[index] : data_[0];
        }
        return data_[index];
    }
    
    constexpr const T& front() const noexcept { return data_[0]; }
    constexpr const T& back() const noexcept { return data_[size_ - 1]; }
    
    // Итераторы (только константные)
    constexpr const T* begin() const noexcept { return data_; }
    constexpr const T* end() const noexcept { return data_ + size_; }
    constexpr const T* cbegin() const noexcept { return data_; }
    constexpr const T* cend() const noexcept { return data_ + size_; }
    
    // Размер и емкость
    constexpr size_t size() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }
    
    // Данные (только константные)
    constexpr const T* data() const noexcept { return data_; }
    
    // Поиск
    constexpr const T* find(const T& value) const noexcept {
        for (size_t i = 0; i < size_; ++i) {
            if (data_[i] == value) {
                return data_ + i;
            }
        }
        return end();
    }
    
    // Проверка наличия элемента
    constexpr bool contains(const T& value) const noexcept {
        return find(value) != end();
    }
    
    // Сравнение
    constexpr bool operator==(const array& other) const noexcept {
        if (size_ != other.size_) return false;
        for (size_t i = 0; i < size_; ++i) {
            if (data_[i] != other.data_[i]) return false;
        }
        return true;
    }
    
    constexpr bool operator!=(const array& other) const noexcept {
        return !(*this == other);
    }
    
    // Проверка валидности
    constexpr bool valid() const noexcept {
        return data_ != nullptr && size_ > 0;
    }
};

// Вспомогательная функция для вывода типа (CTAD)
template<typename T, size_t N>
array(const T (&)[N]) -> array<T>;

// Фабричная функция
template<typename T, size_t N>
constexpr array<T> make_array(const T (&arr)[N]) noexcept {
    return array<T>(arr);
}

} // namespace etl

//////////////////////////////////////////////////////////////////
// Вариант для работы с массивами, определенными в PROGMEM
// Ключевые особенности:
// PROGMEM доступ - все данные читаются через pgm_read_* функции
// Автоматическое управление - не нужно вручную вызывать pgm_read_byte
// Ranged for поддержка - через кастомные итераторы
// Безопасность - проверки границ массива
// Разные типы - поддержка uint8_t, uint16_t, uint32_t, float, char
// Эффективность - минимальные накладные расходы
// Теперь вы можете удобно работать с PROGMEM массивами используя современный C++ синтаксис!

namespace pgm {

// Обобщенная специализация для любых типов (кроме уже специализированных)
template<typename T>
struct pgm_traits {
    static T read(const void* ptr) {
        T result;
        // Для сложных типов читаем побайтово
        const uint8_t* byte_ptr = static_cast<const uint8_t*>(ptr);
        uint8_t* result_bytes = reinterpret_cast<uint8_t*>(&result);
        for (size_t i = 0; i < sizeof(T); ++i) {
            result_bytes[i] = pgm_read_byte(byte_ptr + i);
        }
        return result;
    }
};

template<> struct pgm_traits<uint8_t> {
    static uint8_t read(const void* ptr) { return pgm_read_byte(ptr); }
};

template<> struct pgm_traits<uint16_t> {
    static uint16_t read(const void* ptr) { return pgm_read_word(ptr); }
};

template<> struct pgm_traits<uint32_t> {
    static uint32_t read(const void* ptr) { return pgm_read_dword(ptr); }
};

template<> struct pgm_traits<float> {
    static float read(const void* ptr) { return pgm_read_float(ptr); }
};

template<> struct pgm_traits<char> {
    static char read(const void* ptr) { return pgm_read_byte(ptr); }
};

template<typename T>
class array {
private:
    const T* data_;
    size_t size_;

public:
    constexpr array(const T* ptr, size_t size) noexcept 
        : data_(ptr), size_(size) {}
    
    template<size_t N>
    constexpr array(const T (&arr)[N]) noexcept 
        : data_(arr), size_(N) {}
    
    // Копирование и перемещение
    constexpr array(const array&) noexcept = default;
    constexpr array& operator=(const array&) noexcept = default;
    constexpr array(array&&) noexcept = default;
    constexpr array& operator=(array&&) noexcept = default;

    // Доступ к элементам
    T operator[](size_t index) const noexcept {
        return (index < size_) ? pgm_traits<T>::read(data_ + index) : T{};
    }

    // Безопасный доступ с проверкой границ - метод at()
    T at(size_t index) const noexcept {
        if (index >= size_) {
            return T{}; // Возвращаем значение по умолчанию при выходе за границы
        }
        return pgm_traits<T>::read(data_ + index);
    }
    
    T front() const noexcept { return pgm_traits<T>::read(data_); }
    T back() const noexcept { return pgm_traits<T>::read(data_ + size_ - 1); }
    
    // Итератор
    class iterator {
    private:
        const T* ptr_;
        size_t index_;
        size_t size_;
        
    public:
        constexpr iterator(const T* ptr, size_t index, size_t size) 
            : ptr_(ptr), index_(index), size_(size) {}
        
        T operator*() const { 
            return (index_ < size_) ? pgm_traits<T>::read(ptr_ + index_) : T{};
        }
        
        iterator& operator++() { 
            ++index_; 
            return *this; 
        }
        
        bool operator!=(const iterator& other) const { 
            return index_ != other.index_; 
        }
    };
    
    iterator begin() const { return iterator(data_, 0, size_); }
    iterator end() const { return iterator(data_, size_, size_); }
    
    // Размер и данные
    constexpr size_t size() const noexcept { return size_; }
    constexpr bool empty() const noexcept { return size_ == 0; }
    constexpr const T* data() const noexcept { return data_; }
    
    // Поиск
    bool contains(const T& value) const noexcept {
        for (size_t i = 0; i < size_; ++i) {
            if (pgm_traits<T>::read(data_ + i) == value) return true;
        }
        return false;
    }
    
    // Копирование в RAM
    void copy_to(T* dest, size_t max_size) const {
        size_t count = (max_size < size_) ? max_size : size_;
        for (size_t i = 0; i < count; ++i) {
            dest[i] = pgm_traits<T>::read(data_ + i);
        }
    }
    
    // Получение как обычный массив в RAM (осторожно с памятью!)
    void to_array(T* dest) const {
        for (size_t i = 0; i < size_; ++i) {
            dest[i] = pgm_traits<T>::read(data_ + i);
        }
    }

    etl::array<T> to_array() const {
        etl::array<T> dest(size_);
        for (size_t i = 0; i < size_; ++i) {
            dest[i] = pgm_traits<T>::read(data_ + i);
        }
        return dest;
    }

    // Метод to_string() для char
    template<typename U = T>
    typename std::enable_if<std::is_same<U, char>::value, String>::type
    to_string() const {
        char buffer[size_ + 1];
        for (size_t i = 0; i < size_; ++i) {
            buffer[i] = pgm_read_byte(data_ + i);
        }
        buffer[size_] = '\0';
        return String(buffer);
    }
};

// CTAD
template<typename T, size_t N>
array(const T (&)[N]) -> array<T>;

} // namespace pgm