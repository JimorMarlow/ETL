#include <Arduino.h>
#include <etl.h>

void setup() {
    Serial.begin(115200);
    
    // Демонстрация etl::array
    etl::array<int, 5> arr = {1, 2, 3, 4, 5};
    
    Serial.println("ETL Array Demo:");
    for (auto val : arr) {
        Serial.println(val);
    }
    
    // Демонстрация shared_ptr
    auto ptr = etl::make_shared<int>(42);
    Serial.printf("Shared ptr value: %d\n", *ptr);
}

void loop() {
}