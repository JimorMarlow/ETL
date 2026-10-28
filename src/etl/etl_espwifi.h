// Для включения нужной wi-fi библиотеки
#pragma once
#include "etl_defines.h"
#ifdef ETL_ESP8266
  #include <ESP8266WiFi.h>
#elif defined(ETL_ESP32)
  #include <WiFi.h>
#else
  #pragma message("ERROR: no Wi-Fi lib specified")
#endif
