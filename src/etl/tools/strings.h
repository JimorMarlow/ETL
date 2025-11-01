#pragma once
#include "Arduino.h"
namespace etl {
namespace tools {

struct text {  // Пока не собираются cpp файлы, пока оберну вот так
    static String printf_format(const char* format, ...) {
        char buffer[128];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        return String(buffer);
    }
};

}// namespace tools
}//namespace etl