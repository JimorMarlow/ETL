/**
 * @file etl_queue.h
 * @brief Кольцевая очередь (циклический буфер) с динамической ёмкостью
 * 
 * Реализация STL-подобной очереди с использованием etl::vector для хранения данных.
 * Поддерживает итераторы, range-based for loop, изменение ёмкости на лету.
 * 
 * Особенности:
 * - Динамическое управление памятью через etl::vector
 * - Конструкторы: по умолчанию (ёмкость 16) и с указанием размера
 * - Метод reserve() для изменения ёмкости без потери данных
 * - Поддержка итераторов и range-based for
 * - Методы: push(), pop(), front(), back(), at(), [], get_at()
 * 
 * Примеры использования: см. примеры в начале файла
 * Тесты: см. test_queue() в src/etl/etl_test.cpp
 * 
 * @author Embedded Template Library
 * @date 2026
 */

#pragma once
#include "Arduino.h"
#include "etl_vector.h"

namespace etl {

template<typename T>
class queue {
private:
    etl::vector<T> data;
    size_t front_index = 0;
    size_t back_index = 0;
    size_t count = 0;

public:
    // Конструктор по умолчанию (ёмкость 16)
    queue() : data(16) {}

    // Конструктор с указанием начальной ёмкости
    explicit queue(size_t capacity) : data(capacity) {}

    // Добавление в конец
    bool push(const T& value) {
        if (count >= data.capacity()) {
            return false;
        }

        data[back_index] = value;
        back_index = (back_index + 1) % data.capacity();
        count++;
        return true;
    }

    // Удаление из начала
    bool pop() {
        if (empty()) {
            return false;
        }

        front_index = (front_index + 1) % data.capacity();
        count--;
        return true;
    }

    // Удаление из начала с возвратом значения, которое было вытолкнуто
    etl::optional<T> pop_front()
    {
        etl::optional<T> front_value;
        if (!empty()) {
            front_value = front();
            front_index = (front_index + 1) % data.capacity();
            count--;
        }
        return front_value;
    }

    // Просмотр первого элемента
    T& front() {
        return data[front_index];
    }

    const T& front() const {
        return data[front_index];
    }

    // Просмотр последнего элемента
    T& back() {
        return data[(back_index == 0) ? data.capacity() - 1 : back_index - 1];
    }

    const T& back() const {
        return data[(back_index == 0) ? data.capacity() - 1 : back_index - 1];
    }

    // Информация
    bool empty() const {
        return count == 0;
    }

    bool full() const {
        return count == data.capacity();
    }

    size_t size() const {
        return count;
    }

    size_t capacity() const {
        return data.capacity();
    }

    // Изменение ёмкости (пересоздаёт очередь с новой ёмкостью)
    bool reserve(size_t new_capacity) {
        if (new_capacity < count) {
            return false;  // Нельзя уменьшить ёмкость меньше текущего количества элементов
        }

        etl::vector<T> new_data(new_capacity);
        
        // Копируем элементы в новый вектор в правильном порядке
        for (size_t i = 0; i < count; i++) {
            new_data[i] = data[(front_index + i) % data.capacity()];
        }

        data = new_data;
        front_index = 0;
        back_index = count;
        return true;
    }

    // Очистка
    void clear() {
        front_index = back_index = count = 0;
    }

    // Доступ к элементу по индексу от начала (0 = front, 1 = следующий, и т.д.)
    T& at(size_t index) {
        if (index >= count) {
            // Обработка выхода за границы - можно бросить исключение или вернуть ссылку на static
            static T dummy;
            return dummy;
        }
        return data[(front_index + index) % data.capacity()];
    }

    const T& at(size_t index) const {
        if (index >= count) {
            static T dummy;
            return dummy;
        }
        return data[(front_index + index) % data.capacity()];
    }

    // Оператор [] для удобства (аналогично at)
    T& operator[](size_t index) {
        return at(index);
    }

    const T& operator[](size_t index) const {
        return at(index);
    }

    // Безопасная версия с проверкой границ (возвращает bool вместо ссылки)
    etl::optional<T> get_at(size_t index) const {
        etl::optional<T> value;
        if (index < count) {
            value = data[(front_index + index) % data.capacity()];
        }
        return etl::move(value);
    }

    // Итератор для range-based for loop
    class Iterator {
    private:
        queue* queue_ = nullptr;
        size_t position;
        size_t visited;

    public:
        Iterator(queue* q, size_t pos, size_t vis = 0)
            : queue_(q), position(pos), visited(vis) {}

        T& operator*() {
            return queue_->data[position];
        }

        Iterator& operator++() {
            position = (position + 1) % queue_->data.capacity();
            visited++;
            return *this;
        }

        bool operator!=(const Iterator& other) const {
            return position != other.position || visited != other.visited;
        }
    };

    // Константный итератор
    class ConstIterator {
    private:
        const queue* queue_ = nullptr;
        size_t position;
        size_t visited;

    public:
        ConstIterator(const queue* q, size_t pos, size_t vis = 0)
            : queue_(q), position(pos), visited(vis) {}

        const T& operator*() const {
            return queue_->data[position];
        }

        ConstIterator& operator++() {
            position = (position + 1) % queue_->data.capacity();
            visited++;
            return *this;
        }

        bool operator!=(const ConstIterator& other) const {
            return position != other.position || visited != other.visited;
        }
    };

    // Методы для итераторов
    Iterator begin() {
        if (empty()) {
            return end();
        }
        return Iterator(this, front_index);
    }

    Iterator end() {
        return Iterator(this, back_index, count);
    }

    ConstIterator begin() const {
        if (empty()) {
            return end();
        }
        return ConstIterator(this, front_index);
    }

    ConstIterator end() const {
        return ConstIterator(this, back_index, count);
    }
};

} //namespace etl

/**
 * @example
 * @brief Примеры использования etl::queue
 * 
 * @code{.cpp}
 * void setup() {
 *     Serial.begin(9600);
 * 
 *     // === Базовое использование ===
 *     etl::queue<int> queue(5);  // очередь ёмкостью 5 элементов
 * 
 *     // Заполнение очереди
 *     for (int i = 1; i <= 5; i++) {
 *         queue.push(i * 10);
 *     }
 * 
 *     // Перебор с помощью range-based for
 *     Serial.println("Элементы очереди:");
 *     for (auto& item : queue) {
 *         Serial.println(item);
 *     }
 * 
 *     // Изменение элементов через итератор
 *     for (auto& item : queue) {
 *         item += 5;  // модифицируем каждый элемент
 *     }
 * 
 *     // Const итератор (только для чтения)
 *     const etl::queue<int>& const_queue = queue;
 *     for (const auto& item : const_queue) {
 *         Serial.println(item);
 *     }
 * 
 *     // === Смешанные операции ===
 *     queue.pop();      // удалить первый элемент
 *     queue.push(60);   // добавить новый в конец
 * 
 *     // === Доступ к элементам ===
 *     int first = queue.front();    // первый элемент
 *     int last = queue.back();      // последний элемент
 *     int third = queue.at(2);      // третий элемент (0-based от начала)
 *     int fourth = queue[3];        // четвёртый элемент
 * 
 *     // === Изменение ёмкости ===
 *     etl::queue<float> data_queue(10);
 *     data_queue.push(1.5);
 *     data_queue.push(2.5);
 *     data_queue.push(3.5);
 *     
 *     // Увеличить ёмкость без потери данных
 *     data_queue.reserve(20);  // теперь ёмкость 20, данные сохранены
 *     
 *     // === Конструктор по умолчанию ===
 *     etl::queue<int> default_queue;  // ёмкость 16 по умолчанию
 * 
 *     // === Проверка состояния ===
 *     if (queue.empty()) { }  // очередь пуста
 *     if (queue.full()) { }   // очередь заполнена
 *     size_t count = queue.size();      // количество элементов
 *     size_t capacity = queue.capacity(); // текущая ёмкость
 * }
 * 
 * void loop() {
 *     // ...
 * }
 * @endcode
 * 
 * @see test_queue() в src/etl/etl_test.cpp для примеров тестирования
 */