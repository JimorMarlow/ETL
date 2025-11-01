#### **[`ETL` (embedded template library)](https://github.com/JimorMarlow/ETL)** 
![Arduino](https://img.shields.io/badge/-Arduino-00979D?style=for-the-badge&logo=Arduino&logoColor=white) ![PlatformIO](https://img.shields.io/badge/PlatformIO-%23222.svg?style=for-the-badge&logo=platformio&logoColor=%23f5822a) ![Version](https://img.shields.io/github/package-json/v/JimorMarlow/ETL)

[Another one] Lightweight C++ template library optimized for embedded systems (Arduino, ESP8266/ESP32). Provides STL-like containers and algorithms with minimal memory footprint. Note: Tested on EPS8266/ESP32 boards only but used only Arduino functions.

[Ещё одна] Лековесная библиотека с минимально необходимым набором STL-подобных контейров и обработок с максимальным упрощением функционала и компактным использованием памяти. Я проверяю все на ESP8266/ESP32, но ипользую только Arduino функции.

Jimor Marlow, jimor@inbox.ru
https://github.com/JimorMarlow/etl

**Features:**
- `etl::filter` filters: moving average, median3, median5, exponential. Базовый набор для фильтрации выходного сигнала. То, что в Arduino библиотеке должно было быть из коробки.
- `etl::queue` упрощенный кольцевой буфер для усреднения и фильтрации входящих данных (чтобы каждый раз его не придумывать для того же фильтра скользящим окном)
- `etl::vector` динамический vector для хранения полученных данных
- `etl::array` with fixed capacity. Обертка над статическими массивами в оперативной и флеш памяти (RAM, PROGMEM array support) для однотипного использования различных контейнеров в алгоритмах (`etl::vector`, `etl::array`, `pgm::array`)
- `etl::lookup` Lookup tables with interpolation support (например, для NTC температурных датчиков). Таблица с интерполяцией результатов между опорными точками.
- Color manipulation utilities
- Memory-efficient algorithms

**Dependancy:**
`GTimer` by AlexGyver ([gyverlibs/GTimer@^1.1.1](https://github.com/GyverLibs/GTimer))

*Большое спасибо за уроки и библиотеки! etl, в общем-то, и началась только для того, чтобы добавить функционал, которого не было у Алекса. Например, при добавлении элементов в фильтр первые отсчеты были нулевые. Решил сделать свою реализацию медианного и скользящего среднего, и понеслось. Мой основной подход к написанию кода на Arduino: если есть у AlexGyver - используем его библиотеку (https://alexgyver.ru/), не хватает, или хочется сделать по-своему - делаю сам...*

## Supported Platforms
- ESP8266
- ESP32, ESP32-C3 ...
