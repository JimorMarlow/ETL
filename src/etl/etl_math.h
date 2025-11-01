#pragma once
#include "Arduino.h"

namespace etl
{

class math {
private:
#ifdef ESP32
  static constexpr double EPSILON = 1e-5;
#else
  // для ESP8266 стабильно работает только с 1e-5, 1e-6 уже начинает пропускать, из-за нехватки точности double (которая там фактически float)
  static constexpr double EPSILON = 1e-5;
#endif    

public:
    // Основная функция сравнения
    static bool equals(double a, double b, double epsilon = EPSILON) {
        double diff = fabs(a - b);
        return diff <= epsilon;
        
        // // Для очень маленьких чисел
        // if (diff < epsilon) return true;
        
        // // Для обычных чисел - относительная погрешность
        // double magnitude = fmax(fabs(a), fabs(b));
        // return diff <= (magnitude * epsilon);
    }
    
    static bool is_zero(double value, double epsilon = EPSILON) {
        return fabs(value) < epsilon;
    }
    
    static int compare(double a, double b, double epsilon = EPSILON) {
        if (equals(a, b, epsilon)) return 0;
        return (a < b) ? -1 : 1;
    }
};

}