// скользящее среднее

#pragma once
#include "Arduino.h"
#include "filter/base.h"
#include "etl_queue.h"
namespace etl
{
namespace filter
{
template<typename T, size_t MAX_SIZE = 5>
class moving_average : public filter::base<T>
{
private:
    etl::queue<T, MAX_SIZE> _values;
    T _summ = T{0};//static_cast<T>{0};  

public:
    virtual T update(const T& value) override
    {
        if(_values.full()) {
            _summ -= _values.front();
            _values.pop();
        }
        if(_values.push(value)) {
            _summ += value;
        }
        return _summ / _values.size();
    }

    virtual void reset() override {
        _values.clear();
        _summ = T{0}; //static_cast<T>(0);        
    }
};

}// namespace filter
}// namespace etl