#pragma once
#include "Arduino.h"
#include <new> // для placement new

/* STL подобный динамический вектор с минималистичной реализацией (DeepSeek edition)
Пример использования:

void setup() {
    Serial.begin(9600);
    
    // Версия с начальной емкостью 4
    emb::vector<int, 4> numbers;
    
    Serial.println("=== vector Demo ===");
    
    // Добавляем элементы
    for(int i = 0; i < 10; i++) {
        numbers.push_back(i * 10);
        Serial.print("Added: "); Serial.print(i * 10);
        Serial.print(", Size: "); Serial.print(numbers.size());
        Serial.print(", Capacity: "); Serial.println(numbers.capacity());
    }
    
    // Доступ к элементам
    Serial.println("\nElements:");
    for(size_t i = 0; i < numbers.size(); i++) {
        Serial.println(numbers[i]);
    }
    
    // Использование итераторов
    Serial.println("\nUsing iterators:");
    for(auto& num : numbers) {
        Serial.println(num);
    }
    
    // Тестирование статистики
    DynamicVectorWithStats<float, 2> stats_vec;
    for(int i = 0; i < 8; i++) {
        stats_vec.push_back(i * 1.5f);
    }
    
    Serial.print("\nMemory usage: "); 
    Serial.print(stats_vec.getMemoryUsage()); 
    Serial.println(" bytes");
    Serial.print("Max size: "); Serial.println(stats_vec.getMaxSize());
}

void loop() {}
*/

namespace etl
{

template<typename T>
class vector {
private:
    T* data_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
    
    // Увеличивает емкость, если нужно
    bool reserve_more(size_t new_capacity) {
        if (new_capacity <= capacity_) return true;
        
        T* new_data = new (std::nothrow) T[new_capacity];
        if (!new_data) return false;
        
        // Копируем существующие элементы
        for (size_t i = 0; i < size_; i++) {
            new_data[i] = data_[i];
        }
        
        // Освобождаем старую память
        delete[] data_;
        data_ = new_data;
        capacity_ = new_capacity;
        return true;
    }
    
public:
    // Конструкторы
    vector() = default;
    
    explicit vector(size_t count) {
        resize(count);
    }
    
    vector(size_t count, const T& value) {
        resize(count, value);
    }

    // Конструктор из initializer_list
    vector(std::initializer_list<T> init) {
        if (init.size() > 0) {
            if (reserve_more(init.size())) {
                size_t i = 0;
                for (const auto& item : init) {
                    data_[i++] = item;
                }
                size_ = init.size();
            }
        }
    }

    // Конструктор из статического массива
    template<size_t N>
    vector(const T (&arr)[N]) {
        if (N > 0) {
            if (reserve_more(N)) {
                for (size_t i = 0; i < N; i++) {
                    data_[i] = arr[i];
                }
                size_ = N;
            }
        }
    }

    // Конструктор из указателя и размера
    vector(const T* arr, size_t size) {
        if (arr != nullptr && size > 0) {
            if (reserve_more(size)) {
                for (size_t i = 0; i < size; i++) {
                    data_[i] = arr[i];
                }
                size_ = size;
            }
        }
    }
    
    // Копирующий конструктор
    vector(const vector& other) {
        if (other.size_ > 0) {
            if (reserve_more(other.size_)) {
                for (size_t i = 0; i < other.size_; i++) {
                    data_[i] = other.data_[i];
                }
                size_ = other.size_;
            }
        }
    }
    
    // Оператор присваивания
    vector& operator=(const vector& other) {
        if (this != &other) {
            clear();
            if (reserve_more(other.size_)) {
                for (size_t i = 0; i < other.size_; i++) {
                    data_[i] = other.data_[i];
                }
                size_ = other.size_;
            }
        }
        return *this;
    }

    // Оператор присваивания из initializer_list
    vector& operator=(std::initializer_list<T> init) {
        clear();
        if (init.size() > 0) {
            if (reserve_more(init.size())) {
                size_t i = 0;
                for (const auto& item : init) {
                    data_[i++] = item;
                }
                size_ = init.size();
            }
        }
        return *this;
    }
    
    // Оператор присваивания из статического массива
    template<size_t N>
    vector& operator=(const T (&arr)[N]) {
        clear();
        if (N > 0) {
            if (reserve_more(N)) {
                for (size_t i = 0; i < N; i++) {
                    data_[i] = arr[i];
                }
                size_ = N;
            }
        }
        return *this;
    }

    bool operator==(const vector<T>& other) const {
        if(size() != other.size()) return false;
        for(size_t i = 0; i < size(); ++i )
        {
            if(at(i) != other.at(i)) return false;
        }
        return true;
    }

    bool operator!=(const vector<T>& other) const {
        return !operator==(other);
    }
    
    // Деструктор
    ~vector() {
        delete[] data_;
    }
    
    // Доступ к элементам
    T& operator[](size_t index) {
        return data_[index];
    }
    
    const T& operator[](size_t index) const {
        return data_[index];
    }
    
    T& at(size_t index) {
        if (index >= size_) {
            // В Arduino нет исключений, возвращаем первый элемент
            static T dummy;
            return dummy;
        }
        return data_[index];
    }
    
    const T& at(size_t index) const {
        if (index >= size_) {
            static T dummy;
            return dummy;
        }
        return data_[index];
    }
    
    T& front() {
        return data_[0];
    }
    
    const T& front() const {
        return data_[0];
    }
    
    T& back() {
        return data_[size_ - 1];
    }
    
    const T& back() const {
        return data_[size_ - 1];
    }
    
    T* data() {
        return data_;
    }
    
    const T* data() const {
        return data_;
    }
    
    // Итераторы (упрощенные)
    T* begin() {
        return data_;
    }
    
    const T* begin() const {
        return data_;
    }
    
    T* end() {
        return data_ + size_;
    }
    
    const T* end() const {
        return data_ + size_;
    }
    
    // Информация
    bool empty() const {
        return size_ == 0;
    }
    
    size_t size() const {
        return size_;
    }
    
    size_t capacity() const {
        return capacity_;
    }
    
    // Изменение размера
    void resize(size_t new_size) {
        if (new_size <= size_) {
            // Уменьшаем размер
            size_ = new_size;
        } else {
            // Увеличиваем размер
            if (reserve_more(new_size)) {
                // Инициализируем новые элементы значением по умолчанию
                for (size_t i = size_; i < new_size; i++) {
                    data_[i] = T();
                }
                size_ = new_size;
            }
        }
    }
    
    void resize(size_t new_size, const T& value) {
        if (new_size <= size_) {
            size_ = new_size;
        } else {
            if (reserve_more(new_size)) {
                // Инициализируем новые элементы заданным значением
                for (size_t i = size_; i < new_size; i++) {
                    data_[i] = value;
                }
                size_ = new_size;
            }
        }
    }
    
    // Резервирование памяти
    bool reserve(size_t new_capacity) {
        if (new_capacity <= capacity_) return true;
        return reserve_more(new_capacity);
    }
    
    // Добавление элементов
    bool push_back(const T& value) {
        if (size_ >= capacity_) {
            // Увеличиваем емкость в 2 раза, минимум до 1
            size_t new_capacity = (capacity_ == 0) ? 8 : capacity_ * 2;
            if(capacity_ > 1024) new_capacity = capacity_ + capacity_ / 2;  // после 1К элементов растем по половине для экономии памяти
            if (!reserve_more(new_capacity)) {
                return false;
            }
        }
        data_[size_] = value;
        size_++;
        return true;
    }
    
    void pop_back() {
        if (size_ > 0) {
            size_--;
        }
    }
    
    // Вставка (упрощенная)
    bool insert(size_t position, const T& value) {
        if (position > size_) return false;
        
        if (size_ >= capacity_) {
            size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
            if (!reserve_more(new_capacity)) {
                return false;
            }
        }
        
        // Сдвигаем элементы вправо
        for (size_t i = size_; i > position; i--) {
            data_[i] = data_[i - 1];
        }
        
        data_[position] = value;
        size_++;
        return true;
    }
    
    // Удаление
    void erase(size_t position) {
        if (position >= size_) return;
        
        // Сдвигаем элементы влево
        for (size_t i = position; i < size_ - 1; i++) {
            data_[i] = data_[i + 1];
        }
        size_--;
    }
    
    void clear() {
        size_ = 0;
    }
    
    // Освобождение неиспользуемой памяти
    void shrink_to_fit() {
        if (size_ < capacity_) {
            if (size_ == 0) {
                delete[] data_;
                data_ = nullptr;
                capacity_ = 0;
            } else {
                T* new_data = new (std::nothrow) T[size_];
                if (new_data) {
                    for (size_t i = 0; i < size_; i++) {
                        new_data[i] = data_[i];
                    }
                    delete[] data_;
                    data_ = new_data;
                    capacity_ = size_;
                }
            }
        }
    }
};

}