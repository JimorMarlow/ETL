#pragma once
//////////////////////////////////////////////////
// Измерение скорости выполнения обработок и использование вектора для запоминания данных
#include "etl_vector.h"
namespace etl {
namespace tools {

    class stop_watch
    {
        private:
            uint32_t us_start = 0;     
            uint32_t us_stop = 0;
            etl::vector<uint32_t> samples;
        public:
            stop_watch() = default;
            void start() { us_start = micros(); }
            uint32_t stop() { us_stop = micros(); samples.push_back(get_time()); return samples.back(); }
            uint32_t get_time() { return us_stop - us_start; }
            uint32_t get_averate_time() {
                if(samples.empty()) return 0;

                uint64_t us_total = 0; 
                for(const auto& sample : samples) us_total += sample;
                return static_cast<uint32_t>(us_total / samples.size());
            }
    };

}// namespace tools
}// namespace etl