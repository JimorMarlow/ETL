// Для включения нужной wi-fi библиотеки
#pragma once
#ifdef ESP8266
  #include <ESP8266WiFi.h>
#elif ESP32
  #include <WiFi.h>
#else
  #pragma message("ERROR: no Wi-Fi lib specified")
#endif
