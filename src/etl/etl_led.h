// LED - простой класс для мигания светодиодом
#pragma once
#include "Arduino.h"
#include <GTimer.h>
#include "etl_memory.h"

#define DEFAULT_PWM_FREQUENCY 5000   // Hz
#define DEFAULT_PWM_RESOLUTION 8     // bit

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
       
protected:
    int     _pin    = -1;
    bool    _state  = false;
    bool    _inverse = false;   // инвертировать состояние для некоторых пинов (н-р, встроенный led для esp8266)

    GTimer<millis> _timer_Blink;                    // создать миллисекундный таймер для управления морганием

    int         _pwm_channel = 0;
    uint32_t    _pwm_frequency  = DEFAULT_PWM_FREQUENCY;
    uint8_t     _pwm_resolution = DEFAULT_PWM_RESOLUTION;

    uint32_t   _fade_time_ms;  // Сколько времени зажигать/тушить
    bool       _fade_direction = true;// true - fade_in 
    etl::unique_ptr<GTimer<millis>> _timer_Fade;
};

} //namespace etl