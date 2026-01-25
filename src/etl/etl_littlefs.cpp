#include "etl_littlefs.h"

namespace etl 
{
    bool little_fs::_initialized = false;   // ститическая переменная состояния инициализации LittleFS

    bool little_fs::begin(begin_mode_t flag /*= begin_mode_t::kSilentMode*/)
    {
        if(is_ready()) return true;

        if(flag == begin_mode_t::kShowInfo) Serial.println("Initializing LittleFS...");
        // Пробуем несколько способов
        // Способ 1: Без форматирования
#ifdef ESP32
        if (LittleFS.begin(false)) 
#elif ESP8266
        if (LittleFS.begin()) 
#endif//ESP32-ESP8266 
        {
            if(flag == begin_mode_t::kShowInfo) Serial.println("✓ LittleFS mounted successfully");
            _initialized = true;
        } 
        else 
        {
            if(flag == begin_mode_t::kShowInfo) Serial.println("✗ LittleFS mount failed, trying with formatting...");
            
            // Способ 2: С форматированием
#ifdef ESP32
            if (LittleFS.begin(true)) 
#elif ESP8266
            if (LittleFS.begin()) 
#endif//ESP32-ESP8266 
            {
                if(flag == begin_mode_t::kShowInfo) Serial.println("✓ LittleFS mounted after formatting");
                _initialized = true;
            } 
            else 
            {
                if(flag == begin_mode_t::kShowInfo) Serial.println("✗ All LittleFS mount attempts failed!");
                
                // Способ 3: Форматируем вручную
                if(flag == begin_mode_t::kShowInfo) Serial.println("Attempting manual format...");
                if (LittleFS.format()) 
                {
                    if(flag == begin_mode_t::kShowInfo) Serial.println("✓ Manual format successful");
#ifdef ESP32
                    if (LittleFS.begin(false)) 
#elif ESP8266
                    if (LittleFS.begin()) 
#endif//ESP32-ESP8266 
                    {
                        if(flag == begin_mode_t::kShowInfo) Serial.println("✓ LittleFS mounted after manual format");
                        _initialized = true;
                    }
                }
            }
        }

        if(is_ready())
        {
            show_partition_info();  // отладочная информация
        }

        return is_ready();
    }

    void little_fs::show_partition_info() 
    {
        // Выводим информацию о системе
        Serial.println("========================================");
        Serial.println("LittleFS info");
#ifdef ESP32
        Serial.printf("\tChip: %s\n", ESP.getChipModel());
        Serial.printf("\tCores: %d\n", ESP.getChipCores());
        Serial.printf("\tFlash size: %u bytes\n", ESP.getFlashChipSize());
        Serial.printf("\tFree heap: %u bytes\n", ESP.getFreeHeap());

        if(little_fs::begin(begin_mode_t::kShowInfo))
        {
            Serial.println("\nPartition information:");
            
            esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, 
                                                            ESP_PARTITION_SUBTYPE_ANY, 
                                                            NULL);
            while (it != NULL) {
                const esp_partition_t* p = esp_partition_get(it);
                Serial.printf("  %-12s: 0x%06X - 0x%06X (size: 0x%06X = %u bytes)\n",
                            p->label, p->address, p->address + p->size, 
                            p->size, p->size);
                it = esp_partition_next(it);
            }
            esp_partition_iterator_release(it);

            // Получаем информацию о файловой системе
            size_t total = LittleFS.totalBytes();
            size_t used = LittleFS.usedBytes();
            
            Serial.println("\nLittleFS Info:");
            Serial.printf("\tTotal space: %u bytes\n", total);
            Serial.printf("\tUsed space:  %u bytes\n", used);
            Serial.printf("\tFree space:  %u bytes\n", total - used);
        }
#elif ESP8266
            Serial.println("TODO...");
#endif//ESP32-ESP8266 
    }
}
