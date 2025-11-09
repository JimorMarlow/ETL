// LED - простой класс для мигания светодиодом
#pragma once
#include "Arduino.h"
#include <GTimer.h>
#include "etl_memory.h"
#include "etl_lookup.h"

#define DEFAULT_PWM_FREQUENCY 5000   // Hz
#define DEFAULT_PWM_RESOLUTION 8     // bit

// Настройки для плавного изменения за пределами слышимости >20КГц
#define ESP32_PWM_FREQUENCY 30000    // Hz
#define ESP32_PWM_RESOLUTION 10      // bit

namespace etl {
class led
{
public:
    led(int pin, bool state = false, bool inverse = false);
    virtual ~led() = default;

    virtual bool tick();        // out: true - изменилось состояние по внутреннему таймеру

    bool get_state();   // вернуть состояние из внутренней переменно
    bool set_state(bool state); // Установить новое состояние и записать в порт, если нужно, вернуть новое состояние

    bool read_state();  // считать состояние из порта
    void write_state(bool state); // записать состояние в порт

    const int MIN_PWMRANGE = 0;
    const int MAX_PWMRANGE = 255;
    void init_pwm(int pwm_channel, uint32_t frequency = DEFAULT_PWM_FREQUENCY, uint8_t resolution = DEFAULT_PWM_RESOLUTION);
    void set_pwm(int pwm_value);   // Управление ШИМ (PWM) режимом, обычно 0-255 значения
    int  get_pwm();

    virtual void on();
    virtual void off();
    virtual void blink(uint32_t delay_ms);
    virtual void toggle();
    virtual void reset();   // сбросить таймеры моргания, если были

    virtual void fade_in(uint32_t delay_ms);   // Правно погасить, используя ШИМ
    virtual void fade_out(uint32_t delay_ms);   // Правно зажечь, используя ШИМ

    using fade_t = etl::lookup_t<int, float>;  // delay_ms, brightness [0..1] - map to range [MIN_PWMRANGE, MAX_PWMRANGE]
    virtual void fade(const etl::vector<fade_t>& brightness_ranges); // Плавные переходы по ключевым точкам

    virtual float pwm_to_brightness(int pwm);           // Вернуть нормированную яркость
    virtual int   brightness_to_pwm(float brightness);   // Вернуть значение PWM 
    virtual float gamma() const {return _pwm_gamma; }    // логарифмическая яркость (1.0 - линейная)
    virtual void  set_gamma(float gamma) {_pwm_gamma = gamma;}

    virtual float get_brightness(); // Считать значение яркости по уровню pwn
    virtual void  set_brightness(float brightness); // установить яркость [0.0..1.0]

protected:
    int     _pin    = -1;
    bool    _state  = false;
    bool    _inverse = false;   // инвертировать состояние для некоторых пинов (н-р, встроенный led для esp8266)

    GTimer<millis> _timer_Blink;                    // создать миллисекундный таймер для управления морганием

    int         _pwm_channel = 0;
    uint32_t    _pwm_frequency  = DEFAULT_PWM_FREQUENCY;
    uint8_t     _pwm_resolution = DEFAULT_PWM_RESOLUTION;
    float       _pwm_gamma = 2.2;   // логарифмическая яркость, 1.0 - линейная

    uint32_t   _fade_time_ms;  // Сколько времени зажигать/тушить
 //   bool       _fade_direction = true;// true - fade_in 
    etl::unique_ptr<GTimer<millis>> _timer_Fade;
    etl::unique_ptr<etl::lookup <int, float, etl::vector<fade_t>>> _fade_brightness;    // Таблица интерполяции яркости для перехода
};

} //namespace etl