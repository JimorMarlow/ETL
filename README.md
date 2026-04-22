#### **[`ETL` (embedded template library)](https://github.com/JimorMarlow/ETL)** 
![Arduino](https://img.shields.io/badge/-Arduino-00979D?style=for-the-badge&logo=Arduino&logoColor=white) ![PlatformIO](https://img.shields.io/badge/PlatformIO-%23222.svg?style=for-the-badge&logo=platformio&logoColor=%23f5822a) ![Version](https://img.shields.io/github/package-json/v/JimorMarlow/ETL)

[Another one] Lightweight C++ template library optimized for embedded systems (Arduino, ESP8266/ESP32). Provides STL-like containers and algorithms with minimal memory footprint. Note: Tested on EPS8266/ESP32 boards only but used only Arduino functions.

[Ещё одна] Легковесная библиотека с минимально необходимым набором STL-подобных контейров и обработок с максимальным упрощением функционала и компактным использованием памяти. Я проверяю все на ESP8266/ESP32, но ипользую только Arduino функции.

Jimor Marlow, jimor@inbox.ru

https://github.com/JimorMarlow/etl

### Current status: ![Version](https://img.shields.io/github/package-json/v/JimorMarlow/ETL)
|Environment    |Status    |Duration    |
|-------------  |--------  |------------|
|esp8266        |SUCCESS   |00:00:10.518|
|esp32c3        |SUCCESS   |00:00:15.373|

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
`Filedata` by AlexGyver ([gyverlibs/FileData@^1.0.3](https://github.com/GyverLibs/FileData))
`ArduinoJson` by bblanchon ([bblanchon/ArduinoJson@^7.0.0](https://github.com/bblanchon/ArduinoJson))
`PubSubClient` by knolleary  ([knolleary/PubSubClient@^2.8](https://github.com/knolleary/pubsubclient))

Note to Alex Gyver:
*Большое спасибо за уроки и библиотеки! etl, в общем-то, и началась только для того, чтобы добавить функционал, которого не было у Алекса. Например, при добавлении элементов в фильтр первые отсчеты были нулевые. Решил сделать свою реализацию медианного и скользящего среднего, и понеслось. Мой основной подход к написанию кода на Arduino: если есть у AlexGyver - используем его библиотеки (https://alexgyver.ru/), не хватает, или хочется сделать по-своему - делаю сам...*

## Supported Platforms
- ESP8266 (tested boards: Node MCU, Wemos D1 mini)
- ESP32 (tested boards: ESP32-C3 ...)
- esp32-32u (tested boards: esp32-wroom-32u ...)

---

## Version Hystory | История версий

### v0.9.20 (2026-03-05)
- bug fixes: etl::queue — исправлено использование etl::move

### v0.9.19 (2026-03-05)
- рефакторинг etl::queue — заменён массив на etl::vector
- добавлен метод reserve() для изменения ёмкости очереди на лету
- добавлены конструкторы: по умолчанию (ёмкость 16) и с указанием размера
- обновлены фильтры: median3, median5, moving_average для нового API очереди
- добавлены тесты для queue::reserve()

### v0.9.18 (2026-03-05)
- etl_littlefs.h: добавлены методы:
  - `static bool is_dir_exist(const String& path)` — проверка существования директории
  - `static bool create_dir(const String& path)` — создание директории (и всех родительских)
- добавлены тесты для create_dir() и is_dir_exist() в test_settings()

### v0.9.17 (2026-03-05)
- etl_settings: документация и примеры использования
- тесты для etl::settings::data

### v0.9.16 (2026-03-05)
- etl_settings: добавлен trace_mode_t для управления выводом отладочной информации в Serial
- режимы трассировки: SILENT (тихий), ERRORS (только ошибки), VERBOSE (все сообщения)

### v0.9.15 (2026-03-05)
- test_settings [IN PROGRESS...]

### v0.9.14 (2026-03-05)
- форматирование вывода тестов: выравнивание статуса OK/FAILED

### v0.9.13 (2026-03-04)
- test_littlefs: добавлены тесты для LittleFS
- silent mode в etl::little_fs::begin()

### v0.9.12 (2026-03-04)
- range update: улучшения в range-based for loop

### v0.9.11 (2026-03-04)
- добавлен etl::range_t в etl_utilities.h

### v0.9.10 (2026-01-25)
- little_fs ESP8266 systax quick fix

### v0.9.9 (2026-01-23)
- little_fs starte work, testing

### v0.9.8 (2025-11-28)
- little_fs begin with format 

### v0.9.7 (2025-11-28)
- etl_settings for EEPROM/LittleFS settings store with pending write (to reduce write count)

### v0.9.6 (2025-11-17)
- added: [env:esp32-wroom-32u]

### v0.9.5 (2025-11-10)
- added: compare lookup tests

### v0.9.4 (2025-11-09)
- changed: LED logarifmic brightness 

### v0.9.2 (2025-11-05)
- added: fade using lookup table [... in progress]

### v0.9.1 (2025-11-04)
- added: automatic version update from etl_version.h

### v0.8.15 (2025-11-04)
- changed: library title `ETL` to `ETL-ESP` to avoid collisions with existing libs

