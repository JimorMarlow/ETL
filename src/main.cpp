#include <Arduino.h>
#include "etl/etl_test.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("ETL Test Project Started");

    /////////////////////////////////////////
    // etl - отладка функционала
    etl::unittest::test_all(Serial);
    /////////////////////////////////////////

}

void loop() {
    // Обязательно добавь задержку
    delay(1000);
    Serial.println("Running...");
}