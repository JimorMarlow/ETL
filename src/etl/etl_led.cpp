#include "etl_led.h"
#include "etl_utility.h"
#include "etl_defines.h"
#include <math.h>

namespace etl {

led::led(int pin, bool state, bool inverse /*= false*/)
 : _pin  {pin}
 , _state{state}
 , _inverse{inverse}
{
    pinMode(_pin, OUTPUT);
    bool cur_state = read_state();
    if (cur_state != _state)
        write_state(_state);
}

led::~led()
{
#ifdef ETL_ESP32
    // Освобождаем LEDC-канал, привязанный к пину
    if (_pwm_channel >= 0) {
        ledcDetach(_pin);
        _pwm_channel = -1;
    }
#elif defined(ETL_ESP8266)
    // Отключаем ШИМ и возвращаем пин в цифровой режим
    if (_pwm_channel >= 0) {
        analogWrite(_pin, 0);
        pinMode(_pin, OUTPUT);
        _pwm_channel = -1;
    }
#endif
}

bool led::get_state()
{
    return _state;
}

bool led::set_state(bool state)
{
    if (_state != state)
    {
        _state = state;
        if (read_state() != _state)
        {
            write_state(_state);
        }
    }
    return _state;
}

bool led::read_state()
{
    auto value = digitalRead(_pin);
    return _inverse ? value == LOW : value == HIGH;
}

void led::write_state(bool state)
{
    auto new_state = (_inverse ? !state : state) ? HIGH : LOW;
    digitalWrite(_pin, new_state);
}

void led::init_pwm(int pwm_channel, uint32_t frequency /*= DEFAULT_PWM_FREQUENCY*/, uint8_t resolution /*= DEFAULT_PWM_RESOLUTION*/)
{
    _pwm_channel    = pwm_channel;
    _pwm_frequency  = frequency;
    _pwm_resolution = resolution;
    _last_pwm       = 0;

#ifdef ETL_ESP8266
    analogWriteResolution(_pwm_resolution);
    analogWriteFreq(_pwm_frequency);
    // На ESP8266 понятия «канал» нет — просто помечаем ШИМ как инициализированный
#elif defined(ETL_ESP32)
    // ESP32 Arduino core 3.x: единый API ledcAttachChannel()
    // Отвязываем на случай повторного вызова init_pwm()
    ledcDetach(_pin);
    if (!ledcAttachChannel(_pin, _pwm_frequency, _pwm_resolution, (uint8_t)_pwm_channel)) {
        // Канал занят, пин не поддерживает LEDC и т.п.
        _pwm_channel = -1;
    }
#else
    #pragma message("ERROR: led::init_pwm board not defined")
    _pwm_channel = -1;
#endif
}

void led::set_pwm(int pwm_value)
{
    int value = etl::clamp(pwm_value, MIN_PWMRANGE, max_pwm());
    _last_pwm = value;

#ifdef ETL_ESP8266
    // На ESP8266 analogWrite работает и без явного init_pwm (с дефолтными частотой/разрешением)
    analogWrite(_pin, value);
#elif defined(ETL_ESP32)
    // Без успешного init_pwm писать некуда
    if (_pwm_channel >= 0) {
        ledcWriteChannel(_pwm_channel, value);
    }
#else
    #pragma message("ERROR: led::set_pwm board not defined")
#endif
}

int led::get_pwm()
{
    // Возвращаем последнее записанное значение.
    // На ESP8266 аппаратного чтения ШИМ нет, кэш — единственный вариант.
    // На ESP32 тоже используем кэш для одинакового поведения на всех платформах.
    return _last_pwm;
}

void led::on()  { set_state(true); }
void led::off() { set_state(false); }
void led::toggle() { set_state(!get_state()); }

void led::blink(uint32_t delay_ms)
{
    toggle();
    _timer_Blink.start(delay_ms, GTMode::Timeout);
}

void led::reset()
{
    _timer_Blink.stop();
}

bool led::tick()
{
    if (_timer_Blink.tick())
    {
        toggle();
        return true;
    }

    if (_timer_Fade)
    {
        bool is_runnig = !_timer_Fade->tick();
        if (is_runnig)
        {
            if (_fade_brightness)
            {
                float brigntness = _fade_brightness->raw_to_value(_fade_time_ms - _timer_Fade->getLeft());
                int pwm = brightness_to_pwm(brigntness);
                set_pwm(pwm);
            }
        }
        else {
            _timer_Fade.reset();
            return true;
        }
    }

    return false;
}

void led::fade_in(uint32_t delay_ms)
{
    etl::vector<fade_t> brigtness_points;
    brigtness_points.push_back({0, 1.0});
    brigtness_points.push_back({int(delay_ms), 0.0});
    fade(brigtness_points);
}

void led::fade_out(uint32_t delay_ms)
{
    etl::vector<fade_t> brigtness_points;
    brigtness_points.push_back({0, 0.0});
    brigtness_points.push_back({int(delay_ms), 1.0});
    fade(brigtness_points);
}

void led::fade(const etl::vector<fade_t>& brightness_ranges)
{
    _fade_brightness = etl::make_unique<etl::lookup<int, float, etl::vector<fade_t>>>(brightness_ranges);
    if (_fade_brightness)
    {
        _fade_time_ms = _fade_brightness->max_raw() - _fade_brightness->min_raw();
        _timer_Fade = etl::make_unique<GTimer<millis>>(_fade_time_ms, true, GTMode::Interval);
    }
}

float led::pwm_to_brightness(int pwm)
{
    const int m = max_pwm();
    if (m <= 0) return 0.0f;
    return pow((float)etl::clamp<int>(pwm, MIN_PWMRANGE, m) / float(m), 1.0f / gamma());
}

int led::brightness_to_pwm(float brightness)
{
    const int m = max_pwm();
    return (int)(float(m) * pow(etl::clamp<float>(brightness, 0.0f, 1.0f), gamma()));
}

float led::get_brightness()
{
    return pwm_to_brightness(get_pwm());
}

void led::set_brightness(float brightness)
{
    set_pwm(brightness_to_pwm(brightness));
}

} // namespace etl