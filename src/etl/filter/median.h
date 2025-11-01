#pragma once
// Медианный фильтр для 3 элементов для быстрой сортировки и удаления пиковых выбросов
// Median3 фильтр - оптимизированная версия для окна размером 3

#include "Arduino.h"
#include "filter/base.h"
#include "etl_queue.h"

namespace etl
{
namespace filter
{

    // Median3 фильр, с быстрым сравнением с помощью swap
    template<typename T>
    class median3 : public base<T>
    {
    private:
        etl::queue<T, 3> _values;
        
    public:
        median3() = default;

        static T fast_median(T a, T b, T c) // специально работаю с копиями объектов для упрощения перестановок
        {
            // Сортировка сети для 3 элементов
            if (a > b) std::swap(a, b); // теперь a <= b
            if (b > c) std::swap(b, c); // теперь b <= c, c - максимальное
            if (a > b) std::swap(a, b); // теперь a <= b
            // Медиана - средний элемент b
            return b;
        }
        
        virtual T update(const T& value) override
        {
            if(_values.full()) _values.pop();
            _values.push(value);   
            if(!_values.full())
            {
                if(_values.size() == 1)  { // первое значение - накапливаем и возвращаем сырые данные
                    return value;   
                }
                if(_values.size() == 2) {  // отдаем среднее первых двух    
                    return static_cast<T>((_values[0] + _values[1]) / 2); 
                }
            }        
            // Фильтрация по трем значениям 
            return fast_median(_values[0], _values[1], _values[2]);        
        }

        virtual void reset() override {
            _values.clear();
        }
    };

    // Median5 фильр, с быстрым сравнением с помощью swap
    template<typename T>
    class median5 : public base<T>
    {
    private:
        etl::queue<T, 5> _values;
        
    public:
        median5() = default;

        static T fast_median(T a, T b, T c, T d, T e) // специально работаю с копиями объектов для упрощения перестановок
        {
            // Находим только медиану без полной сортировки
            
            // Алгоритм нахождения медианы за 6 сравнений в среднем
            // Находим минимальный и максимальный, исключаем их, повторяем
            
            // Исключаем минимум и максимум
            if (a > b) std::swap(a, b);
            if (c > d) std::swap(c, d);
            if (a > c) std::swap(a, c);
            if (b > d) std::swap(b, d);
            
            // Теперь a - минимальный среди первых четырех
            // d - максимальный среди первых четырех
            
            // Вставляем e
            if (e < a) {
                // e минимальный, исключаем e и d
                // Медиана среди a, b, c
                if (b > c) return c;
                else return b;
            } 
            else if (e > d) {
                // e максимальный, исключаем e и a
                // Медиана среди b, c, d
                if (b > c) {
                    if (c > d) return c;
                    else return (b > d) ? d : b;
                } else {
                    if (b > d) return d;
                    else return (c > d) ? d : c;
                }
            } 
            else {
                // e между a и d, исключаем a и d
                // Медиана среди b, c, e
                if (b > c) {
                    if (c > e) return c;
                    else return (b > e) ? e : b;
                } else {
                    if (b > e) return b;
                    else return (c > e) ? e : c;
                }
            }
        }
        
        virtual T update(const T& value) override
        {
            if(_values.full()) _values.pop();
            _values.push(value);   
            if(!_values.full())
            {
                if(_values.size() == 1)  { // первое значение - накапливаем и возвращаем сырые данные
                    return value;   
                }
                if(_values.size() == 2) {  // отдаем среднее первых двух    
                    return static_cast<T>((_values[0] + _values[1]) / 2); 
                }
                if(_values.size() == 3) {  // median3
                    return median3<T>::fast_median(_values[0], _values[1], _values[2]); 
                }
                if(_values.size() == 4) {  // median3 по последним трем, игнорируя первый
                    return median3<T>::fast_median(_values[1], _values[2], _values[3]); 
                }
            }        
            // Фильтрация по пяти значениям 
            return fast_median(_values[0], _values[1], _values[2], _values[3], _values[4]);        
        }

        virtual void reset() override {
            _values.clear();
        }
    };

}// namespace filter
}// namespace etl