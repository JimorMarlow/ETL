// проверка elt - Embedded Lightweight Template library,
// STL-like облегченная библиотека для простых элементов из стандартной библиотеки
#pragma once 
#include "Arduino.h"

namespace etl {
namespace unittest {

    bool test_all(Stream& trace);
    void test_result(Stream& trace, const String& title, const String& result);

    // тесты модулей
    String test_empty();
    String test_optional();
    String test_unique();
    String test_shared_weak();
    String test_queue();
    String test_vector();
    String test_array();
    String test_espnow();
    String test_lookup();
    String test_color_lookup();
    String test_color_spectrum();
    String test_algorythm();
    
    // проверка алгоритмов 
    void profiler_average_filter(Stream& trace);
    void profiler_lookup_table(Stream& trace);
    void profiler_color_tds(Stream& trace);    // Градиентные цвета для таблицы TDS значений
    void profiler_lookup_ntc(Stream& trace);    // Для термодатчики ntc 3950 50K проверка перевода сопротивления в градусы цельсия

} //namespace unittest
} //namespace etl