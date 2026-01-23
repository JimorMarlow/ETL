#pragma once
// Настройки проекта с сохранение в EEPROM или операционную систему

#include <Arduino.h>
#include <FileData.h>
#include <LittleFS.h>

// Инициализация файловой системы
namespace etl
{
    class little_fs 
    {
    private:
        static bool _initialized;
                
    public:
        enum begin_mode_t :  bool {
            kSilentMode = false,    // Не выводить ничего в Serial
            kShowInfo = true        // Показывать результаты        
        };
        static bool begin(begin_mode_t flag = begin_mode_t::kSilentMode);
        static bool is_ready() {
            return _initialized;
        }

    static void show_partition_info();
        
    private:
        
    };

}//..etl
