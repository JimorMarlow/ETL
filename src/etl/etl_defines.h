#pragma once

// ETL platform detection header
// Based on reliable Arduino platform macros

// 1. Определение платформы ESP8266
// В ядре ESP8266 Arduino всегда определены оба макроса
#if defined(ARDUINO_ARCH_ESP8266) || defined(ESP8266)
    #define ETL_ESP8266 1
#endif

// 2. Определение платформы ESP32 (все семейство: ESP32, S2, S3, C3, C6, H2 и т.д.)
// ARDUINO_ARCH_ESP32 - самый надежный макрос для Arduino-ESP32
#if defined(ARDUINO_ARCH_ESP32)
    #define ETL_ESP32 1
#endif

// 3. Дополнительная проверка для ESP-IDF (на случай использования без Arduino)
// ESP_PLATFORM определяется в ESP-IDF
#if defined(ESP_PLATFORM) && !defined(ETL_ESP32)
    #define ETL_ESP32 1
#endif

// 4. Проверка на неопределенную платформу (для отладки)
#if !defined(ETL_ESP8266) && !defined(ETL_ESP32)
    #warning "ETL: Platform not detected. ETL_ESP8266 / ETL_ESP32 are undefined."
#endif