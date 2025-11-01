// Базовые класс для фильтров
#pragma once

namespace etl
{
namespace filter
{
    template<typename T>
    class base
    {
    public:
        base() = default;
        virtual ~base() = default;
        virtual T update(const T& value) { return value; }   // Добавить новое значение для фильтрации
        virtual void reset() {}                              // Сброс фильтра
    };

}// namespace filter
}// namespace etl