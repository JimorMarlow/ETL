#include <Arduino.h>
#include "etl/etl_test.h"
#include "etl/etl_version.h"

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
    Serial.print("ETL version "); Serial.println(ETL_VERSION_STRING);
    Serial.println("Running...");
    delay(1000);    
}