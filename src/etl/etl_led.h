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

// Тег для выбора режима «авто-канал» в init_pwm().
// Использование: led.init_pwm(etl::auto_channel, 5000, 10);
struct auto_channel_t {};
inline constexpr auto_channel_t auto_channel{};

class led
{
public:
    led(int pin, bool state = false, bool inverse = false);
    virtual ~led();

    virtual bool tick();        // out: true - изменилось состояние по внутреннему таймеру

    bool get_state();   // вернуть состояние из внутренней переменно
    bool set_state(bool state); // Установить новое состояние и записать в порт, если нужно

    bool read_state();  // считать состояние из порта
    void write_state(bool state); // записать состояние в порт

    // Минимальное значение ШИМ всегда 0
    static constexpr int MIN_PWMRANGE = 0;
    // Максимальное значение ШИМ зависит от разрешения канала.
    // Для 8 бит -> 255, для 10 бит -> 1023.
    int max_pwm() const { return (1 << _pwm_resolution) - 1; }

    // true, если ШИМ успешно инициализирован через init_pwm()
    bool is_pwm_initialized() const { return _pwm_initialized; }

    // Явное указание канала ШИМ.
    // На ESP8266 параметр pwm_channel игнорируется (нет каналов),
    // но принимается для совместимости API.
    bool init_pwm(int pwm_channel,
                  uint32_t frequency = DEFAULT_PWM_FREQUENCY,
                  uint8_t resolution = DEFAULT_PWM_RESOLUTION);

    // Авто-назначение канала (ESP32: ledcAttach; ESP8266: analogWrite).
    // Удобно, когда номер канала не важен — рекомендуется как дефолтный вариант.
    bool init_pwm(auto_channel_t,
                  uint32_t frequency = DEFAULT_PWM_FREQUENCY,
                  uint8_t resolution = DEFAULT_PWM_RESOLUTION);

    // Освободить ШИМ-канал и вернуть пин в цифровой режим
    void deinit_pwm();

    void set_pwm(int pwm_value);   // Управление ШИМ (PWM)
    int  get_pwm();

    virtual void on();
    virtual void off();
    virtual void blink(uint32_t delay_ms);
    virtual void toggle();
    virtual void reset();   // сбросить таймеры моргания, если были

    virtual void fade_in(uint32_t delay_ms);
    virtual void fade_out(uint32_t delay_ms);

    using fade_t = etl::lookup_t<int, float>;
    virtual void fade(const etl::vector<fade_t>& brightness_ranges);

    virtual float pwm_to_brightness(int pwm);
    virtual int   brightness_to_pwm(float brightness);
    virtual float gamma() const { return _pwm_gamma; }
    virtual void  set_gamma(float gamma) { _pwm_gamma = gamma; }

    virtual float get_brightness();
    virtual void  set_brightness(float brightness);

protected:
    int     _pin     = -1;
    bool    _state   = false;
    bool    _inverse = false;

    GTimer<millis> _timer_Blink;

    bool        _pwm_initialized = false;
    int         _pwm_channel    = -1;   // информативно; -1 = авто-канал или не инициализирован
    uint32_t    _pwm_frequency  = DEFAULT_PWM_FREQUENCY;
    uint8_t     _pwm_resolution = DEFAULT_PWM_RESOLUTION;
    int         _last_pwm       = 0;    // последнее записанное значение (для get_pwm)
    float       _pwm_gamma      = 2.2;

    uint32_t   _fade_time_ms;
    etl::unique_ptr<GTimer<millis>> _timer_Fade;
    etl::unique_ptr<etl::lookup<int, float, etl::vector<fade_t>>> _fade_brightness;
};

} // namespace etl