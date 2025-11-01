#include "etl_led.h"
#include "etl_utility.h"

namespace etl {

led::led(int pin, bool state, bool inverse /*= false*/)
 : _pin  {pin}
 , _state{state}
 , _inverse{inverse}
{
    pinMode(_pin, OUTPUT); // Инициализация пина как выход
    bool cur_state = read_state();
    if(cur_state != _state)
        write_state(_state);
}

bool led::get_state()   // вернуть состояние из внутренней переменно
{
   return _state;
}
    
bool led::set_state(bool state) // Установить новое состояние и записать в порт, если нужно, вернуть новое состояние
{
    if(_state != state)
    {
        _state = state;
        if(read_state() != _state)
        {
            write_state(_state);
        }
    }

    return _state;
}
    
bool led::read_state()  // считать состояние из порта
{
    auto value = digitalRead(_pin);
    return _inverse ? value == LOW : value == HIGH;
}

void led::write_state(bool state) // записать состояние в порт
{
    auto new_state = (_inverse ? !state : state) ? HIGH : LOW;  
    digitalWrite(_pin, new_state);
}

void led::set_pwm(int pwm_value)   // Управление ШИМ (PWM) режимом, обычно 0-255 значения
{
    int value = etl::clamp(pwm_value, MIN_PWMRANGE, MAX_PWMRANGE); // PWMRANGE
    
#ifdef ESP8266
    analogWrite(_pin, value);
#elif ESP32
    ledcWrite(_pwm_channel, value);
#else
    #pragma message("ERROR: led::set_pwm board not defined")
#endif   
}

int led::get_pwm()
{
#ifdef ESP8266
    return analogRead(_pin);
#elif ESP32
    return ledcRead(_pwm_channel);
#else
    #pragma message("ERROR: led::get_pwm board not defined")
    return 0;
#endif    
}

void led::init_pwm(int pwm_channel, uint32_t frequency /*= DEFAULT_PWM_FREQUENCY*/, uint8_t resolution /*= DEFAULT_PWM_RESOLUTION*/)
{
    _pwm_channel = pwm_channel;
    _pwm_frequency = frequency;
    _pwm_resolution = resolution;
    
#ifdef ESP8266
    analogWriteResolution(_pwm_resolution);
    analogWriteFreq(_pwm_frequency);
#elif ESP32
    // задаём настройки ШИМ-канала:                                         
    ledcSetup(_pwm_channel, _pwm_frequency, _pwm_resolution);
    // подключаем ШИМ-канал к пину светодиода:                                         
    ledcAttachPin(_pin, _pwm_channel);
#else
  #pragma message("ERROR: led::init_pwm board not defined")
#endif    
}

void led::on()
{
    set_state(true);
}
    
void led::off()
{
    set_state(false);
}

void led::toggle()
{
    set_state(!get_state());
}

void led::blink(uint32_t delay_ms)
{
    toggle();
    _timer_Blink.start(delay_ms, GTMode::Timeout);
}

void led::reset()   // сбросить таймеры моргания, если были
{
    _timer_Blink.stop();
}

bool led::tick()
{
    if(_timer_Blink.tick())
    {
        toggle();
        return true;
    }

    if(_timer_Fade)
    {
        bool is_runnig = !_timer_Fade->tick();
        if(/*_timer_Fade->running()*/ is_runnig)
        {
            // Плавно гасим до нулевого уровня
            uint32_t ms_left = _fade_direction ? _fade_time_ms - _timer_Fade->getLeft(): _timer_Fade->getLeft();        
            float fade_ratio = static_cast<float>(ms_left) / _fade_time_ms;
            if(_inverse) fade_ratio = 1.0 - fade_ratio;
            fade_ratio = etl::clamp<float>(fade_ratio, 0.0, 1.0);
            int pwm = static_cast<int>(fade_ratio * (MAX_PWMRANGE - MIN_PWMRANGE)); // :TODO: Потом заменить на нормальные диапазоны
        //    Serial.printf("fade %s: pwm=%d, fade_ratio = %g, ms_left=%d, _fade_time_ms=%d\n", _fade_direction ? "in" : "out", pwm, fade_ratio, ms_left, _fade_time_ms);            
            set_pwm(pwm);
        }
        else{
            // Остановить таймер
        //    Serial.println("_timer_Fade.reset");
            _timer_Fade.reset();
            return true;
        }
    }

    return false;
}

void led::fade_in(uint32_t delay_ms)   // Правно погасить, используя ШИМ
{
    _fade_direction = true;
    _fade_time_ms = delay_ms;
    _timer_Fade = etl::make_unique<GTimer<millis>>(delay_ms, true, GTMode::Interval);
}

void led::fade_out(uint32_t delay_ms)   // Правно зажечь, используя ШИМ
{
    _fade_direction = false;
    _fade_time_ms = delay_ms;
    _timer_Fade = etl::make_unique<GTimer<millis>>(delay_ms, true, GTMode::Interval);
}

} //namespace etl