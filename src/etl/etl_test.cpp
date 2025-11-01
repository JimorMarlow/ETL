#include "etl_test.h"
#include "etl_optional.h"
#include "etl_math.h"
#include "etl_memory.h"
#include "etl_queue.h"
#include "etl_vector.h"
#include "etl_array.h"
#include "etl_utility.h"
#include "etl_lookup.h"
#include "etl_color.h"
#include "etl_algorithm.h"
#include "filter/moving_average.h"
#include "filter/exponential.h"
#include "filter/median.h"
#include "sensor/temperature.h"
#include "sensor/ntc_temperature_3950_50K_table.h"
#include "tools/stop_watch.h"
#include "tools/strings.h"
#include "etl_espnow.h"

namespace etl {
namespace unittest { 

    static uint32_t mem_free = ESP.getFreeHeap();   // контроль памяти 
    static uint32_t mem_free_before = mem_free;

    bool test_all(Stream& trace)
    {
        trace.println("--- Embedded Template Library ---");
        mem_free = ESP.getFreeHeap();
        
        test_result(trace, "test_empty", test_empty());
        
        test_result(trace, "test_optional", test_optional());
        test_result(trace, "test_unique", test_unique());
        test_result(trace, "test_shared_weak", test_shared_weak());
        test_result(trace, "test_queue", test_queue());
        test_result(trace, "test_vector", test_vector());
        test_result(trace, "test_array", test_array());
        test_result(trace, "test_espnow", test_espnow());
        test_result(trace, "test_lookup", test_lookup());
        test_result(trace, "test_color_lookup", test_color_lookup());
        test_result(trace, "test_color_spectrum", test_color_spectrum());
        test_result(trace, "test_algorythm", test_algorythm());

        test_result(trace, "test_empty", test_empty());
        
        trace.println("--------------------------------");
        int memory_leaks = int(ESP.getFreeHeap()) - mem_free;
        trace.print("MEMORY LEAKS: "); trace.print(memory_leaks); trace.println(memory_leaks == 0 ? "\tOK" : "\tFAILED");
        trace.printf("sizeof: int = %d, float = %d, double = %d\n", sizeof(int), sizeof(float), sizeof(double));
        trace.println("--------------------------------");
     
    //    profiler_average_filter(trace);
    //    profiler_lookup_table(trace);
    //    profiler_color_tds(trace);
        profiler_lookup_ntc(trace);

        return true;
    }

    
    #define TEST_EQUAL(actual, expected, error_message) if(actual != expected) return error_message;
    #define TEST_NOT_EQUAL(actual, expected, error_message) if(actual == expected) return error_message;
    
    void test_result(Stream& trace, const String& title, const String& result)
    {
        mem_free_before = mem_free;     mem_free = ESP.getFreeHeap();   // контроль памяти 
        int diff = int(mem_free_before) - int(mem_free);
        trace.print("MEMORY: "); trace.print(mem_free); trace.print(" diff = "); trace.print(diff); trace.print("\t| ");
        trace.print(title + " - ");  trace.println(result.isEmpty() ? "OK" : "FAILED: " + result);
    }

    String test_empty()
    {
        return "";  // no errors
    }

    String test_optional()
    {
        etl::optional<float> temperature;
        if(temperature) return "empty optinal";
        TEST_EQUAL(temperature.has_value(), false, "empty has_value");
        temperature = 36.6;
        TEST_EQUAL(math::equals(temperature.value(), 36.6), true, "<float> assign and check value");
        TEST_EQUAL(math::equals(temperature.value(), 36.66), false, "<float> compare 36.66");

        etl::optional<int> hum(100);
        TEST_EQUAL(hum.has_value(), true, "<int>(100) initialize");
        TEST_EQUAL(*hum, 100, "<int> emplace assign and check value");
        hum.reset();
        TEST_EQUAL(hum.has_value(), false, "reset failed");

        int value1 = hum.value_or(5); 
        TEST_EQUAL(hum.has_value(), false, "value_or(5)");
        TEST_EQUAL(value1, 5, "value_or(5) failed");

        return "";  // no errors
    }

    String test_unique()
    {
        etl::unique_ptr<int> int_ptr;
        if(int_ptr) return "empty unique_ptr failed";
        TEST_EQUAL(int_ptr.operator bool(), false, "unique_ptr operator bool()");
        TEST_EQUAL(int_ptr.empty(), true, "empty");
        TEST_EQUAL(int_ptr, nullptr, "nullptr compare");
        int_ptr = etl::make_unique<int>(42);
        if(!int_ptr) return "unique_ptr should be filled";
        TEST_NOT_EQUAL(int_ptr, nullptr, "not nullptr compare");
        TEST_EQUAL(*int_ptr, 42, "unique_ptr equils 42");
        return "";  // no errors
    }

    String test_shared_weak()
    {
        etl::shared_ptr<int> int_ptr;
        if(int_ptr) return "shared_ptr failed";
        TEST_EQUAL(int_ptr.operator bool(), false, "shared_ptr operator bool()");
        TEST_EQUAL(int_ptr.empty(), true, "shared_ptr empty");
        TEST_EQUAL(int_ptr, nullptr, "shared_ptr nullptr compare");
        int_ptr = etl::make_shared<int>(42);
        if(!int_ptr) return "shared_ptr should be filled";
        TEST_NOT_EQUAL(int_ptr, nullptr, "shared_ptrnot nullptr compare");
        TEST_EQUAL(*int_ptr, 42, "shared_ptr equils 42");

        etl::weak_ptr<int> int_wptr;
        if(int_wptr) return "etl::weak_ptr failed";
        TEST_EQUAL(int_wptr.operator bool(), false, "etl::weak_ptr operator bool()");
        TEST_EQUAL(int_wptr.empty(), true, "etl::weak_ptr empty");
        TEST_EQUAL(int_wptr, nullptr, "etl::weak_ptr nullptr compare");

        // Использование weak_ptr
        int_wptr = int_ptr;
        TEST_EQUAL(int_wptr.expired(), false, "Weak ptr expired: false");
                
        {
            auto locked = int_wptr.lock(); 
            TEST_EQUAL(locked.empty(), false, "etl::weak_ptr shared lock() ");
        }
        
        // Сброс shared_ptr
        int_ptr.reset();
        TEST_EQUAL(int_wptr.expired(), true, "Weak ptr expired after reset: true");

        {
            auto locked = int_wptr.lock(); 
            TEST_EQUAL(locked.empty(), true, "etl::weak_ptr shared lock() after reset");
        }

        // Расчет счетчиков
        auto stop_watch = etl::make_shared<etl::tools::stop_watch>();
        TEST_EQUAL(stop_watch.unique(), true, "stop_watch.unique() true");
        TEST_EQUAL(stop_watch.use_count(), 1, "stop_watch.use_count() 1");
        {
            auto sw1 = stop_watch;
            TEST_EQUAL(stop_watch.use_count(), 2, "stop_watch.use_count() 2");
            auto sw2 = stop_watch;
            TEST_EQUAL(stop_watch.use_count(), 3, "stop_watch.use_count() 3");
        }
        TEST_EQUAL(stop_watch.use_count(), 1, "stop_watch.use_count() 1 after");
        stop_watch.reset();
        TEST_EQUAL(stop_watch.empty(), true, "shared_ptr stop_watch reset()");
    
        return "";
    }

    String test_queue()
    {
        etl::queue<int, 5> avgInt5; // Усреднение 5 значений
        TEST_EQUAL(avgInt5.empty(), true, "avgInt5.empty()");
        TEST_EQUAL(avgInt5.capacity(), 5, "avgInt5.capacity");
        for(int i = 1; i <= 5; ++i) avgInt5.push(i);
        TEST_EQUAL(avgInt5.full(), true, "avgInt5.full");
        TEST_EQUAL(avgInt5.size(), 5, "avgInt5.size 5");
        TEST_EQUAL(avgInt5.at(0), 1, "avgInt5.at(0)");
        TEST_EQUAL(avgInt5.at(1), 2, "avgInt5.at(1)");
        TEST_EQUAL(avgInt5.at(2), 3, "avgInt5.at(2)");
        TEST_EQUAL(avgInt5.at(3), 4, "avgInt5.at(3)");
        TEST_EQUAL(avgInt5.at(4), 5, "avgInt5.at(4)");

        int sum5 = 0; 
        for(auto value : avgInt5) sum5+= value;
        TEST_EQUAL(sum5, 15, "sum5 == 15");
        int avg5 = sum5 / avgInt5.size();
        TEST_EQUAL(avg5, 3, "avg5 == 3");

        TEST_EQUAL(avgInt5.push(6), false, "push to full queue");

        avgInt5.pop();
        TEST_EQUAL(avgInt5.full(), false, "pop one value");
        TEST_EQUAL(avgInt5.push(6), true, "push to not full queue");

        while(avgInt5.pop()) {}
        TEST_EQUAL(avgInt5.full(), false, "full() on empty queue");
        TEST_EQUAL(avgInt5.empty(), true, "empty queue");

        // Доступ по индексу (для медианного фильтра)
        avgInt5.clear();
        avgInt5.push(42);
        TEST_EQUAL(avgInt5[0], 42, "1: avgInt5.[0] 42)");
        TEST_EQUAL(avgInt5.front(), avgInt5.back(), "2: avgInt5.front() == back()");
        avgInt5.push(31);
        TEST_EQUAL(avgInt5[0], 42, "2: avgInt5.[0] 42)");
        TEST_EQUAL(avgInt5[1], 31, "2: avgInt5.[1] 31)");        
        TEST_EQUAL(avgInt5.front(), 42, "2: avgInt5.front() 42");
        TEST_EQUAL(avgInt5.back(), 31, "2: avgInt5.back() 31");
        avgInt5.push(20);
        TEST_EQUAL(avgInt5[0], 42, "3: avgInt5.[0] 42)");
        TEST_EQUAL(avgInt5[1], 31, "3: avgInt5.[1] 31)");
        TEST_EQUAL(avgInt5[2], 20, "3: avgInt5.[2] 20)");
        TEST_EQUAL(avgInt5.front(), 42, "3: avgInt5.front() 42");
        TEST_EQUAL(avgInt5.back(), 20, "3: avgInt5.back() 20");

        auto f1 = avgInt5.pop_front();
        TEST_EQUAL(f1.value(), 42, "4: avgInt5.pop_front() 42");
        auto f2 = avgInt5.pop_front();
        TEST_EQUAL(f2.value(), 31, "4: avgInt5.pop_front() 31");
        auto f3 = avgInt5.pop_front();
        TEST_EQUAL(f3.value(), 20, "4: avgInt5.pop_front() 20");

        TEST_EQUAL(avgInt5.empty(), true, "4: avgInt5.empty()");
        auto f4 = avgInt5.pop_front();
        TEST_EQUAL(f4.has_value(), false, "4: f4 empty has_value");

        return "";
    }

    String test_vector()
    {
        etl::vector<uint8_t> bytes;
        TEST_EQUAL(bytes.empty(), true, "vector empty");

        for(uint8_t i = 1; i <= 5; ++i ) bytes.push_back(i);
        TEST_EQUAL(bytes.empty(), false, "vector not empty");
        TEST_EQUAL(bytes.size(), 5, "vector size() 5");
        TEST_EQUAL(bytes.at(0), 1, "1: vector at(0) 1");
        TEST_EQUAL(bytes.at(1), 2, "1: vector at(1) 2");
        TEST_EQUAL(bytes.at(2), 3, "1: vector at(2) 3");
        TEST_EQUAL(bytes.at(3), 4, "1: vector at(3) 4");
        TEST_EQUAL(bytes.at(4), 5, "1: vector at(4) 5");

        bytes.clear();
        TEST_EQUAL(bytes.empty(), true, "2: vector empty");
        bytes.resize(20, 0xFF);
        TEST_EQUAL(bytes.size(), 20, "2: vector size() 20");
        TEST_EQUAL(bytes[10], 0xFF, "2: vector [10] 0xFF");
        bytes.reserve(30);
        TEST_EQUAL(bytes.capacity(), 30, "3: vector capacity() 30");
        TEST_EQUAL(bytes.size(), 20, "3: vector size() 20");
        TEST_EQUAL(bytes[10], 0xFF, "3: vector [10] 0xFF");        

        auto bytes_copy = bytes;
        TEST_EQUAL(bytes_copy.capacity(), bytes_copy.size(), "4: vector copy capacity() equl original size()");
        TEST_EQUAL(bytes_copy.size(), 20, "4: vector copy size() 20");
        TEST_EQUAL(bytes_copy[10], 0xFF, "4: vector copy [10] 0xFF");
        size_t capacity4 = bytes_copy.capacity();
        bytes_copy.push_back(0xAD);        
        TEST_EQUAL(bytes_copy.capacity(), capacity4 * 2, "5: vector copy capacity() increased x2");
        TEST_EQUAL(bytes_copy.size(), 21, "5: vector copy size() 20");
        TEST_EQUAL(bytes_copy.back(), 0xAD, "5: vector copy back() 0xAD");

        etl::vector<float> fv;
        float sum1 = 0;
        for(int i=0; i < 100; ++i) {
            float value = i * 2.0;
            fv.push_back(value);
            sum1 += value;
        }
        float sum2 = 0;
        for(const auto& value : fv) {
            sum2 += value;     
        }
        TEST_EQUAL(math::equals(sum1, sum2), true, "vector<float> assign and check summ");

        // Инициализация из статического массива
        const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
        etl::vector<uint8_t> mac (size_t(6), 0xFF);
        etl::vector<uint8_t> mac1 {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        etl::vector<uint8_t> mac2 {broadcastAddress}; 
        etl::vector<uint8_t> mac3 = broadcastAddress; 
        const etl::vector<uint8_t> mac4 {broadcastAddress}; 

        TEST_EQUAL(mac.size(), 6, "mac size() 6");
        TEST_EQUAL(mac.capacity(), 6, "mac capacity() 6");

        TEST_EQUAL(mac1.size(), 6, "mac1 size() 6");
        TEST_EQUAL(mac2.size(), 6, "mac2 size() 6");
        TEST_EQUAL(mac3.size(), 6, "mac3 size() 6");

        for(auto b : mac) TEST_EQUAL(b, 0xFF, "mac bytes oxFF");
        TEST_EQUAL(mac, mac1, "mac == mac1");
        TEST_EQUAL(mac1, mac2, "mac1 == mac2");
        TEST_EQUAL(mac2, mac3, "mac2 == mac3");
        TEST_EQUAL(mac3, mac4, "mac3 == mac4");
        
        return "";  // no errors
    }

    // PROGMEM массивы
    const uint8_t mac_address_p[] PROGMEM = {0xF4, 0xCF, 0xA2, 0x78, 0xDF, 0xF9};
    const uint16_t sensor_data_p[] PROGMEM = {100, 200, 300, 400, 500};
    const char welcome_text_p[] PROGMEM = "Hello from PROGMEM!";
   
    String test_array()
    {
        const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 
        etl::array mac1{broadcastAddress};
        TEST_EQUAL(mac1.size(), 6, "array mac1 size() 6");
        uint32_t summ = 0;
        for(auto bb : mac1) summ += bb;
        TEST_EQUAL(summ, 6*255, "array mac1 summ");

        const uint8_t test_arr1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; 
        etl::array<uint8_t> sub1{test_arr1+2, size_t(3)};
        TEST_EQUAL(sub1.size(), 3, "array sub1 size() 3");
        TEST_EQUAL(sub1.at(0), 2, "array sub1.at(0) == 2");

        int test_arr2[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}; 
        etl::array<int> arr2 = test_arr2;
        auto it5 = arr2.find(5);
        TEST_NOT_EQUAL(it5, arr2.end(), "array fond 5: it5 != arr2.end()");
        etl::array<int> sub2{it5, size_t(5)};
        TEST_EQUAL(sub2.size(), 5, "array sub2 size() 5");
        TEST_EQUAL(sub2.at(0), 5, "array sub2.at(0) == 5");
        TEST_EQUAL(sub2.at(4), 9, "array sub2.at(4) == 9");

        // Создание оберток
        pgm::array mac_arr(mac_address_p);
        pgm::array sensor_arr(sensor_data_p);
        pgm::array text_arr(welcome_text_p);  // Для строк тоже работает!
    
        // Ranged for loop
        String s_mac = ("MAC: ");
        for (size_t i = 0; i < mac_arr.size(); ++i) {
            if (i > 0) s_mac += ":";
            s_mac += String(mac_arr[i], HEX);
        }
        s_mac.toUpperCase();
        TEST_EQUAL(s_mac, "MAC: F4:CF:A2:78:DF:F9", "pgm::array mac_arr(mac_address_p)" );
    
        // Доступ по индексу
        TEST_EQUAL(sensor_arr[0], 100, "First sensor value: 100");
        TEST_EQUAL(sensor_arr.size(), 5, "Sensor array size 5");
    
        // Поиск
        TEST_EQUAL(mac_arr.contains(0xA2), true, "MAC contains 0xA2");
           
        // Копирование в RAM
        uint8_t mac_ram[6];
        mac_arr.copy_to(mac_ram, sizeof(mac_ram));
        etl::array mem_arr(mac_ram);
        TEST_EQUAL(mac_arr.size(), mem_arr.size(), "mac_arr.size() == mem_arr.size()");
        TEST_EQUAL(mac_arr.at(3), mem_arr.at(3), "mac_arr.at(3) == mem_arr.at(3)");
    
        // Работа со строками
        String pgm_text = "Text: ";
        for (auto ch : text_arr) {
            if (ch == 0) break;  // null terminator
            pgm_text += String(static_cast<char>(ch));
        }
        TEST_EQUAL(pgm_text, "Text: Hello from PROGMEM!", "pgm_text = Text: Hello from PROGMEM!");
        String pgm_text2 = "Text: " + text_arr.to_string();
        TEST_EQUAL(pgm_text, pgm_text2, "pgm_text == pgm_text2");
        
        return "";  // no errors
    }

    String test_espnow()
    {
        const espnow::endpoint_t s001 {"F4:CF:A2:78:DF:F9"};    
        espnow::endpoint_t s002 {0xF4,0xCF,0xA2,0x78,0xDF,0xF9}; 
        
        const espnow::endpoint_t s003 {"EC:FA:BC:D5:A2:50"};
        
        TEST_NOT_EQUAL(s001, s003, "espnow s001 != s003");
        TEST_EQUAL(s001, s002, "espnow s001 == s002");

        const espnow::endpoint_t esp_modules[] = {s001, s002, s003}; 
        etl::array<espnow::endpoint_t> modules (esp_modules);
        TEST_EQUAL(modules.contains(s003), true, "espnow modules.contains(s003)");

        return "";  // no errors
    }

    const etl::lookup_t<float, float> ntc_sensor_3950_50K_p[] PROGMEM = {{1000.0, 1.0}, {2000.0, 2.0}, {3000.0, 3.0}};
    const etl::lookup_t<float, float> ntc_sensor_3950_50K[] = {{1000.0, 1.0}, {2000.0, 2.0}, {3000.0, 3.0}};
    const etl::lookup_t<float, float> ntc_sensor_3950_50K_desc[] = {{3000.0, 1.0}, {2000.0, 2.0}, {1000.0, 3.0}};
    String test_lookup()
    {
        etl::array ntc_temp(ntc_sensor_3950_50K);
        etl::array ntc_desc(ntc_sensor_3950_50K_desc);
        pgm::array ntc_temp_p(ntc_sensor_3950_50K_p);
        etl::vector ntc_temp_v(ntc_sensor_3950_50K);

        /////////////////////////////////////////////////////////
        //  ascending raw mode

        // Обрезка по границам с интерполяцией между значениями
        etl::lookup <float, float, etl::array<lookup_t<float, float>>> lt_ntc3950 (ntc_temp);
        TEST_EQUAL(math::equals(lt_ntc3950.raw_to_value(1500), 1.5), true, "lt_ntc3950.raw_to_value(1500)");
        TEST_EQUAL(math::equals(lt_ntc3950.raw_to_value(500), 1.0), true, "lt_ntc3950.raw_to_value(500)");
        TEST_EQUAL(math::equals(lt_ntc3950.raw_to_value(3500), 3.0), true, "lt_ntc3950.raw_to_value(3500)");

        // С экстаполяцией за пределами крайних значений
        etl::array_lookup <float, float> lt_ntc3950_extra(ntc_temp, lookup_mode::INTERPOLATE, bounds_mode::EXTRAPOLATE);
        TEST_EQUAL(math::equals(lt_ntc3950_extra.raw_to_value(1500), 1.5), true, "lt_ntc3950_extra.raw_to_value(1500)");
        TEST_EQUAL(math::equals(lt_ntc3950_extra.raw_to_value(500), 0.5), true, "lt_ntc3950_extra.raw_to_value(500)");
        TEST_EQUAL(math::equals(lt_ntc3950_extra.raw_to_value(3500), 3.5), true, "lt_ntc3950_extra.raw_to_value(3500)");

        // Ближайшее значение без интеполяции с обрезкой
        auto lt_ntc3950_near = etl::make_lookup<float, float>(ntc_temp, lookup_mode::NEAREST, bounds_mode::CLAMP);
        TEST_EQUAL(math::equals(lt_ntc3950_near.raw_to_value(1500), 1.0), true, "lt_ntc3950_near.raw_to_value(1500)");
        TEST_EQUAL(math::equals(lt_ntc3950_near.raw_to_value(1501), 2.0), true, "lt_ntc3950_near.raw_to_value(1501)");
        TEST_EQUAL(math::equals(lt_ntc3950_near.raw_to_value(500), 1.0), true, "lt_ntc3950_near.raw_to_value(500)");
        TEST_EQUAL(math::equals(lt_ntc3950_near.raw_to_value(3500), 3.0), true, "lt_ntc3950_near.raw_to_value(3500)");

    //    float temp = lt_ntc3950_near.raw_to_value(1501);
    //    Serial.println(temp,3);       

        /////////////////////////////////////////////////////////
        //  descending raw mode

        // Обрезка по границам с интерполяцией между значениями
        etl::lookup <float, float, etl::array<lookup_t<float, float>>> lt_ntc_desc(ntc_desc);
        TEST_EQUAL(math::equals(lt_ntc_desc.raw_to_value(1500), 2.5), true, "lt_ntc_desc.raw_to_value(1500)");
        TEST_EQUAL(math::equals(lt_ntc_desc.raw_to_value(500), 3.0), true, "lt_ntc_desc.raw_to_value(500)");
        TEST_EQUAL(math::equals(lt_ntc_desc.raw_to_value(3500), 1.0), true, "lt_ntc_desc.raw_to_value(3500)");

        // С экстаполяцией за пределами крайних значений
        etl::array_lookup <float, float> lt_ntc_desc_extra(ntc_desc, lookup_mode::INTERPOLATE, bounds_mode::EXTRAPOLATE);
        TEST_EQUAL(math::equals(lt_ntc_desc_extra.raw_to_value(1500), 2.5), true, "lt_ntc_desc_extra.raw_to_value(1500)");
        TEST_EQUAL(math::equals(lt_ntc_desc_extra.raw_to_value(500), 3.5), true, "lt_ntc_desc_extra.raw_to_value(500)");
        TEST_EQUAL(math::equals(lt_ntc_desc_extra.raw_to_value(3500), 0.5), true, "lt_ntc_desc_extra.raw_to_value(3500)");

        // Ближайшее значение без интеполяции с обрезкой
        auto lt_ntc_desc_near = etl::make_lookup<float, float>(ntc_desc, lookup_mode::NEAREST, bounds_mode::CLAMP);
        TEST_EQUAL(math::equals(lt_ntc_desc_near.raw_to_value(1500), 2.0), true, "lt_ntc_desc_near.raw_to_value(1500)");
        TEST_EQUAL(math::equals(lt_ntc_desc_near.raw_to_value(1499), 3.0), true, "lt_ntc_desc_near.raw_to_value(1499)");
        TEST_EQUAL(math::equals(lt_ntc_desc_near.raw_to_value(1501), 2.0), true, "lt_ntc_desc_near.raw_to_value(1501)");
        TEST_EQUAL(math::equals(lt_ntc_desc_near.raw_to_value(500), 3.0), true, "lt_ntc_desc_near.raw_to_value(500)");
        TEST_EQUAL(math::equals(lt_ntc_desc_near.raw_to_value(3500), 1.0), true, "lt_ntc_desc_near.raw_to_value(3500)");

        /////////////////////////////////////////////////////////
        //  ascending raw mode PROGMEM version

        // Обрезка по границам с интерполяцией между значениями
        etl::lookup <float, float, pgm::array<lookup_t<float, float>>> lt_ntc3950_p (ntc_temp_p);
        TEST_EQUAL(math::equals(lt_ntc3950_p.raw_to_value(1500), 1.5), true, "lt_ntc3950_p.raw_to_value(1500)");
        TEST_EQUAL(math::equals(lt_ntc3950_p.raw_to_value(500), 1.0), true, "lt_ntc3950_p.raw_to_value(500)");
        TEST_EQUAL(math::equals(lt_ntc3950_p.raw_to_value(3500), 3.0), true, "lt_ntc3950_p.raw_to_value(3500)");

        // С экстаполяцией за пределами крайних значений
        etl::pgm_lookup <float, float> lt_ntc3950_extra_p(ntc_temp_p, lookup_mode::INTERPOLATE, bounds_mode::EXTRAPOLATE);
        TEST_EQUAL(math::equals(lt_ntc3950_extra_p.raw_to_value(1500), 1.5), true, "lt_ntc3950_extra_p.raw_to_value(1500)");
        TEST_EQUAL(math::equals(lt_ntc3950_extra_p.raw_to_value(500), 0.5), true, "lt_ntc3950_extra_p.raw_to_value(500)");
        TEST_EQUAL(math::equals(lt_ntc3950_extra_p.raw_to_value(3500), 3.5), true, "lt_ntc3950_extra_p.raw_to_value(3500)");

        // Ближайшее значение без интеполяции с обрезкой
        auto lt_ntc3950_near_p = etl::make_lookup<float, float>(ntc_temp_p, lookup_mode::NEAREST, bounds_mode::CLAMP);
        TEST_EQUAL(math::equals(lt_ntc3950_near_p.raw_to_value(1500), 1.0), true, "lt_ntc3950_near_p.raw_to_value(1500)");
        TEST_EQUAL(math::equals(lt_ntc3950_near_p.raw_to_value(1501), 2.0), true, "lt_ntc3950_near_p.raw_to_value(1501)");
        TEST_EQUAL(math::equals(lt_ntc3950_near_p.raw_to_value(500), 1.0), true, "lt_ntc3950_near_p.raw_to_value(500)");
        TEST_EQUAL(math::equals(lt_ntc3950_near_p.raw_to_value(3500), 3.0), true, "lt_ntc3950_near_p.raw_to_value(3500)");

        /////////////////////////////////////////////////////////
        //  ascending raw mode, VECTOR vesrion

        // Обрезка по границам с интерполяцией между значениями
        etl::lookup <float, float, etl::vector<lookup_t<float, float>>> lt_ntc3950_v (ntc_temp_v);
        TEST_EQUAL(math::equals(lt_ntc3950_v.raw_to_value(1500), 1.5), true, "lt_ntc3950_v.raw_to_value(1500)");
        TEST_EQUAL(math::equals(lt_ntc3950_v.raw_to_value(500), 1.0), true, "lt_ntc3950_v.raw_to_value(500)");
        TEST_EQUAL(math::equals(lt_ntc3950_v.raw_to_value(3500), 3.0), true, "lt_ntc3950_v.raw_to_value(3500)");

        // С экстаполяцией за пределами крайних значений
        etl::vector_lookup <float, float> lt_ntc3950_extra_v(ntc_temp_v, lookup_mode::INTERPOLATE, bounds_mode::EXTRAPOLATE);
        TEST_EQUAL(math::equals(lt_ntc3950_extra_v.raw_to_value(1500), 1.5), true, "lt_ntc3950_extra_v.raw_to_value(1500)");
        TEST_EQUAL(math::equals(lt_ntc3950_extra_v.raw_to_value(500), 0.5), true, "lt_ntc3950_extra_v.raw_to_value(500)");
        TEST_EQUAL(math::equals(lt_ntc3950_extra_v.raw_to_value(3500), 3.5), true, "lt_ntc3950_extra_v.raw_to_value(3500)");

        // Ближайшее значение без интеполяции с обрезкой
        auto lt_ntc3950_near_v = etl::make_lookup<float, float>(ntc_temp_v, lookup_mode::NEAREST, bounds_mode::CLAMP);
        TEST_EQUAL(math::equals(lt_ntc3950_near_v.raw_to_value(1500), 1.0), true, "lt_ntc3950_near_v.raw_to_value(1500)");
        TEST_EQUAL(math::equals(lt_ntc3950_near_v.raw_to_value(1501), 2.0), true, "lt_ntc3950_near_v.raw_to_value(1501)");
        TEST_EQUAL(math::equals(lt_ntc3950_near_v.raw_to_value(500), 1.0), true, "lt_ntc3950_near_v.raw_to_value(500)");
        TEST_EQUAL(math::equals(lt_ntc3950_near_v.raw_to_value(3500), 3.0), true, "lt_ntc3950_near_v.raw_to_value(3500)");
        
        return ""; // no errors 
    }

    String test_color_lookup() 
    {
    //    Serial.println("=== Testing Color Lookup ===");
        
        // Таблица соответствия температуры цвету
        const etl::lookup_t<int, etl::color_t> temperature_colors[] = {
            {-20, etl::color_t::BLUE()},        // -20°C: синий
            {0,   etl::color_t::WHITE()},       // 0°C: белый
            {20,  etl::color_t::GREEN()},       // 20°C: зеленый
            {30,  etl::color_t::YELLOW()},      // 30°C: желтый
            {70,  etl::color_t::RED()},         // 70°C: красный
            {100, etl::color_t::MAROON()}       // 100°C: бордовый
        };
        
        // Создаем lookup для цветов
        auto color_lookup = etl::make_color_lookup<int>(temperature_colors);
        
        // Тестируем ключевые точки        //Serial.println("Key points:");
        TEST_EQUAL("-20°C: " + color_lookup.raw_to_value(-20).to_string(), "-20°C: RGB(0,0,255)", "Должен быть синий");
        TEST_EQUAL("0°C: " + color_lookup.raw_to_value(0).to_string(), "0°C: RGB(255,255,255)", "Должен быть белый");
        TEST_EQUAL("20°C: " + color_lookup.raw_to_value(20).to_string(), "20°C: RGB(0,255,0)", "Должен быть зеленый");
        TEST_EQUAL("30°C: " + color_lookup.raw_to_value(30).to_string(), "30°C: RGB(255,255,0)", "Должен быть желтый");
        TEST_EQUAL("70°C: " + color_lookup.raw_to_value(70).to_string(), "70°C: RGB(255,0,0)", "Должен быть красный");
        TEST_EQUAL("100°C: " + color_lookup.raw_to_value(100).to_string(), "100°C: RGB(128,0,0)", "Должен быть бордовый");
        
        // Тестируем интерполяцию между точками       // Serial.println("\nInterpolated points:");
        
        // Между -20°C (синий) и 0°C (белый)
        auto color_m10 = color_lookup.raw_to_value(-10);    //Serial.println("-10°C: " + color_m10.to_string()); // Должен быть светло-синий
        TEST_EQUAL(color_m10, etl::color_t(127,127,255), "color_m10 Должен быть светло-синий");

        // Между 0°C (белый) и 20°C (зеленый)
        auto color_10 = color_lookup.raw_to_value(10);      //Serial.println("10°C: " + color_10.to_string()); // Должен быть светло-зеленый
        TEST_EQUAL(color_10, etl::color_t(127,255,127), "color_10 Должен быть светло-зеленый");
        
        // Между 20°C (зеленый) и 30°C (желтый)
        auto color_25 = color_lookup.raw_to_value(25);      //Serial.println("25°C: " + color_25.to_string()); // Должен быть желто-зеленый
        TEST_EQUAL(color_25, etl::color_t(127,255,0), "color_25 Должен быть желто-зеленый");
        
        // Между 30°C (желтый) и 70°C (красный)
        auto color_50 = color_lookup.raw_to_value(50);      // Serial.println("50°C: " + color_50.to_string()); // Должен быть оранжевый
        TEST_EQUAL(color_50, etl::color_t(255,127,0), "color_50 Должен быть оранжевый");
        
        // Между 70°C (красный) и 100°C (бордовый)
        auto color_85 = color_lookup.raw_to_value(85);      //Serial.println("85°C: " + color_85.to_string()); // Должен быть темно-красный
        TEST_EQUAL(color_85, etl::color_t(191,0,0), "color_85 Должен быть темно-красный");
        
        // Тестируем граничные значения        // Serial.println("\nBoundary values:");
        auto color_low = color_lookup.raw_to_value(-30);  // Ниже минимального  // Serial.println("-30°C: " + color_low.to_string());  // Должен быть синий (clamp)
        TEST_EQUAL(color_low, etl::color_t(0,0,255), "color_low Ниже минимального");
        auto color_high = color_lookup.raw_to_value(150); // Выше максимального // Serial.println("150°C: " + color_high.to_string()); // Должен быть бордовый (clamp)
        TEST_EQUAL(color_high, etl::color_t(128,0,0), "color_high Выше максимального");
        
        // Тестируем разные режимы      //Serial.println("\nDifferent modes:");
        
        // Режим ближайшего значения
        color_lookup.set_lookup_mode(etl::lookup_mode::NEAREST);
        auto color_12_nearest = color_lookup.raw_to_value(12);  // Serial.println("12°C (nearest): " + color_12_nearest.to_string()); // Ближе к 10°C или 20°C
        TEST_EQUAL(color_12_nearest, etl::color_t(0,255,0), "12°C (nearest): Ближе к 10°C или 20°C");
        
        auto color_28_nearest = color_lookup.raw_to_value(28);  // Serial.println("28°C (nearest): " + color_28_nearest.to_string()); // Ближе к 20°C или 30°C
        TEST_EQUAL(color_28_nearest, etl::color_t(255,255,0), "28°C (nearest): Ближе к 20°C или 30°C");
        
        // Возвращаем режим интерполяции
        color_lookup.set_lookup_mode(etl::lookup_mode::INTERPOLATE);
        
        // Тестируем экстраполяцию
        color_lookup.set_bounds_mode(etl::bounds_mode::EXTRAPOLATE);
        // 0°C: RGB(255,255,255)", "Должен быть белый"
        // -20°C: RGB(0,0,255)", "Должен быть синий"
        // -30 - должна быть экстраполяция 0,0,255
        // -40 - должна быть экстраполяция 0,0,255
        // auto color_m25_extra = color_lookup.raw_to_value(-25);  
        auto color_m30_extra = color_lookup.raw_to_value(-30);  
        // auto color_m40_extra = color_lookup.raw_to_value(-40);  
        // auto color_m45_extra = color_lookup.raw_to_value(-45);  
        // Serial.println(color_m25_extra.to_string());
        // Serial.println(color_m30_extra.to_string());
        // Serial.println(color_m40_extra.to_string());
        // Serial.println(color_m45_extra.to_string());
        TEST_EQUAL(color_m30_extra, etl::color_t(0,0,255), "-30°C (extrapolate)");
        auto color_150_extra = color_lookup.raw_to_value(150);
        TEST_EQUAL(color_150_extra, etl::color_t(0,0,0), "150°C (extrapolate)");
        
        return ""; // no errors 
    }

    // Функция для визуальной проверки (выводит цвета в последовательном виде)
    String test_color_spectrum() 
    {
        //Serial.println("\n=== Color Spectrum ===");        
        const etl::lookup_t<int, etl::color_t> temperature_colors[] = {
            {-20, etl::color_t(0, 0, 255)},     // Синий
            {0,   etl::color_t(255, 255, 255)}, // Белый
            {20,  etl::color_t(0, 255, 0)},     // Зеленый
            {30,  etl::color_t(255, 255, 0)},   // Желтый
            {70,  etl::color_t(255, 0, 0)},     // Красный
            {100, etl::color_t(128, 0, 0)}      // Бордовый
        };
        
        auto spectrum_lookup = etl::make_color_lookup<int>(temperature_colors);
        
        // Выводим спектр от -20 до 100 с шагом 10
        etl::vector<String> results_spectr;
        for (int temp = -20; temp <= 100; temp += 10) {
            auto color = spectrum_lookup.raw_to_value(temp);
            results_spectr.push_back( etl::tools::text::printf_format("Temp %3d°C: %s\n", temp, color.to_string().c_str()) );
        //    Serial.printf("Temp %3d°C: %s\n", temp, color.to_string().c_str());
        }
        TEST_EQUAL(results_spectr.size(), 13, "results_spectr.size()");
        TEST_EQUAL(results_spectr[0], "Temp -20°C: RGB(0,0,255)\n", "results_spectr[0]");
        TEST_EQUAL(results_spectr[1], "Temp -10°C: RGB(127,127,255)\n", "results_spectr[1]");
        TEST_EQUAL(results_spectr[2], "Temp   0°C: RGB(255,255,255)\n", "results_spectr[2]");
        TEST_EQUAL(results_spectr[3], "Temp  10°C: RGB(127,255,127)\n", "results_spectr[3]");
        TEST_EQUAL(results_spectr[4], "Temp  20°C: RGB(0,255,0)\n", "results_spectr[4]");
        TEST_EQUAL(results_spectr[5], "Temp  30°C: RGB(255,255,0)\n", "results_spectr[5]");
        TEST_EQUAL(results_spectr[6], "Temp  40°C: RGB(255,191,0)\n", "results_spectr[6]");
        TEST_EQUAL(results_spectr[7], "Temp  50°C: RGB(255,127,0)\n", "results_spectr[7]");
        TEST_EQUAL(results_spectr[8], "Temp  60°C: RGB(255,63,0)\n", "results_spectr[8]");
        TEST_EQUAL(results_spectr[9], "Temp  70°C: RGB(255,0,0)\n", "results_spectr[9]");
        TEST_EQUAL(results_spectr[10], "Temp  80°C: RGB(212,0,0)\n", "results_spectr[10]");
        TEST_EQUAL(results_spectr[11], "Temp  90°C: RGB(170,0,0)\n", "results_spectr[11]");
        TEST_EQUAL(results_spectr[12], "Temp 100°C: RGB(128,0,0)\n", "results_spectr[12]");

        return ""; // no errors 
    }
 
    void profiler_lookup_table(Stream& trace)
    {
        Serial.println();
        trace.println("--- PROFILER LOOKUP TABLE ---");
        const etl::lookup_t<float, float> ntc_sensor_3950_50K[] = {{1000.0, 1.0}, {2000.0, 2.0}, {3000.0, 3.0}};
        etl::array ntc_temp(ntc_sensor_3950_50K);
        
        etl::tools::stop_watch stat_arr_mem;
        etl::lookup lt_ntc3950(ntc_sensor_3950_50K /*ntc_temp*/);
        
        Serial.println("raw;\tT;\t");
        for(float raw = 1000; raw <= 3000; raw += 10)
        {
            Serial.print(raw,1); Serial.print(";\t");
            stat_arr_mem.start();
            float temp = lt_ntc3950.raw_to_value(raw);
            stat_arr_mem.stop();
            Serial.print(temp,4); Serial.print(";\t");
            Serial.println();
        }

        trace.println();
        trace.print("TIME;\tus;\t"); 
        trace.println(stat_arr_mem.get_averate_time()); 
    }

    void profiler_color_tds(Stream& trace)    
    {
        // Градиентные цвета для шкалы TDS значений (Total Dissolved Solids)
        const etl::lookup_t<int, etl::color_t> tds_colors[] = {
            {50,    etl::color_t(152, 211, 255)},   // Идеальная питьевая вода после обратного осмоса
            {100,   etl::color_t(2, 147, 128)},     // Приемлимая питьевая вода
            {200,   etl::color_t(82, 65, 255)},     // На грани допустимого
            {300,   etl::color_t(150, 210, 0)},     // Неприятная вода из-под крана
            {400,   etl::color_t(128, 123, 0)},     // Вода из минеральных источников
            {500,   etl::color_t(148, 31, 1)},      // Максимальный уровень загрязнения
            {600,   etl::color_t(51, 0, 0)}         // Опасность
        };

        auto spectrum_lookup = etl::make_color_lookup<int>(tds_colors);
        etl::tools::stop_watch stat;

        // Выводим спектр от -20 до 100 с шагом 10
        Serial.println("TDS;\tRGB;\t");
        for (int tds = 0; tds <= 650; tds++) 
        {
            stat.start();
            auto color = spectrum_lookup.raw_to_value(tds);
            stat.stop();
            Serial.printf("%d;\t%s;\t\n", tds, color.to_hex_string().c_str());
        }

        trace.println();
        trace.print("TIME;\tus;\t"); 
        trace.println(stat.get_averate_time()); 
        // ESP32C3:         TIME;   us;     15

        /* Для проверки используются гугл таблицы
        1. Откройте Google Таблицы, перейдите в меню Расширения → Apps Script
        2. Вставьте такой скрипт:

        function previewRGBColor() {
            var sheet = SpreadsheetApp.getActiveSheet();
            var lastRow = sheet.getLastRow();
            for (var i = 1; i <= lastRow; i++) {
                var hex = sheet.getRange(i, 2).getValue();
                sheet.getRange(i, 3).setBackground(hex);
            }
        }
         
        3. Для запуска вручную:
            На панели скрипта выберите функцию previewRGBColor и нажмите ► "Выполнить".
            Разрешите доступ к вашему аккаунту (первый запуск требует подтверждения).
        4. После запуска скрипта столбец D (Preview) окрасится в нужный цвет по значениям R/G/B.

        Файл с результатами: docs\result_profiler_color_tds.xlsx
        */
    }

    const int test_pgm[] PROGMEM = {0, 1, 2, 3, 4, 5}; 
        
    String test_algorythm()
    {
        // Создание контейнеров
        int test_arr[] = {0, 1, 2, 3, 4, 5}; 
        etl::array<int> arr = test_arr;
        etl::vector<int> vec = {10, 20, 30, 40, 50};
        pgm::array<int> pgm = test_pgm;
        
        // for_each с лямбдой
        String for_each_array;
        etl::for_each(arr, [&for_each_array](int x) {
            for_each_array += String(x) + String(" ");
        });
        TEST_EQUAL(for_each_array, "0 1 2 3 4 5 ", "for_each_array");
        
        String for_each_pgm;
        etl::for_each(pgm, [&for_each_pgm](int x) {
            for_each_pgm += String(x) + String(" ");
        });
        TEST_EQUAL(for_each_pgm, "0 1 2 3 4 5 ", "for_each_pgm");
        
        // find_if с лямбдой
        auto found = etl::find_if(arr, [](int x) { return x > 3; });
        TEST_NOT_EQUAL(found, arr.end(), "find_if(arr, x > 3)");
        TEST_EQUAL(*found, 4, "find_if(arr, x > 3) = 4");
        
        // accumulate
        int sum = etl::accumulate(arr, 0);  // 15
        TEST_EQUAL(sum, (0+1+2+3+4+5), "accumulate sum(arr) == 15");
        
        int product = etl::accumulate(vec, 1, [](int a, int b) { return a * b; });  // 12000000
        TEST_EQUAL(product, (10*20*30*40*50), "accumulate product(vec) == 12000000");
        
        // Работа с vector
        vec.push_back(60);
        vec.push_back(70);
        
        String for_each_vec = "Vector contents:";
        etl::for_each(vec, [&for_each_vec](int x) {
            for_each_vec += String(x)+ String(" ");
        });
        TEST_EQUAL(for_each_vec, "Vector contents:10 20 30 40 50 60 70 ", "for_each_vec");
        
        // Поиск в векторе
        auto vec_found = etl::find_if(vec, [](int x) { return x % 30 == 0; });
        TEST_NOT_EQUAL(vec_found, vec.end(), "find_if(vec");
        TEST_EQUAL(*vec_found, 30, "find_if(vec { return x % 30 == 0; }");

        // fill
        etl::fill(vec, 10);
        TEST_EQUAL(vec.size(), 7, "vector fill 10 size");
        String fill_result;
        etl::for_each(vec, [&fill_result](const auto x) {
            if(x != 10) fill_result += "vector fill 10: error x = " + String(x) + String("\n");});
        TEST_EQUAL(fill_result, "", fill_result);

        // copy
        etl::vector<int> vec2(vec.size());
        etl::copy(etl::begin(vec), etl::end(vec), etl::begin(vec2));    // vec2 должен иметь размер, необходимый для копирования нужных данных
        TEST_EQUAL(vec2.size(), 7, "vector copy size");
        String copy_result;
        etl::for_each(vec2, [&copy_result](const auto x) {
            if(x != 10) copy_result += "vector copy: error x = " + String(x) + String("\n");});
        TEST_EQUAL(copy_result, "", copy_result);
        
        return "";
    }
    
    //////////////////////////////////////////////////////////////////////
    // Фильры 

    // Функция для генерации редких выбросов
    float add_rare_noise(float base_value, int iteration) {
        // Генерируем выбросы примерно каждые 20-30 итераций
        if (iteration % random(20, 30) == 0 && iteration > 10) {
            // Большой выброс в положительную или отрицательную сторону
            return base_value + random(30, 60) * (random(0, 2) ? 1.0f : -1.0f);
        }
        // Иногда добавляем небольшие выбросы
        else if (random(0, 100) < 10 && iteration > 10) { // 10% chance
            return base_value + random(5, 15) * (random(0, 2) ? 1.0f : -1.0f);
        }
        return base_value;
    }

    void profiler_average_filter(Stream& trace)
    {
        ///////////////////////////////////////////////
        // скользящее окно на 5 целых чисел
        trace.println("--- PROFILER AVERAGE_FILETER ---");
        trace.println("i\tfilter_average<int, 5>");
        filter::moving_average<int, 5> avg5_int;
        for(float i = 0; i < 25; i += 1.0)
        {
            if(i == 15) { 
                avg5_int.reset();
                trace.println("reset");
            } 
            int avg = avg5_int.update(i);
            trace.print(i); trace.print("\t");
            trace.println(avg);
        }

        ///////////////////////////////////////////////
        // скользящее окно на 10 адщфе
        trace.println();
        trace.println("i\tfilter_average<float, 10>");
        filter::moving_average<float, 10> avg5_float;
        for(float i = 0; i < 25; i += 1.0)
        {
            if(math::equals(i, 15.0)) { // нифига не работает сравнение???
                avg5_float.reset();
                trace.println("reset");
            } 
            float avg = avg5_float.update(i);
            trace.print(i); trace.print("\t");
            trace.println(avg);
        }
        
        // Использование сложная структура для датчика температуры и влажности в бегущем среднем фильтра
        trace.println();
        trace.println("i;\tTraw;\tTm3;\tTavg;\tTexp;\tHraw;\tHm5;\tHavg;\tHexp;\t");
        
        // медианный фильтр на входе для удаления выбросов
        filter::median3<float> temperature_noice_remover; 
        filter::median5<float> humidity_noice_remover; 
        
        // Скользящее среднее
        filter::moving_average<sensor::data::th_t, 5> TH;
        // Экспоненциальный 
        filter::exponential<float> temperature_exp(0.3); // alpha = 0.1, 1.0 - без изменений
        filter::exponential<float> humidity_exp(0.5); // alpha = 0.1, 1.0 - без изменений
        sensor::data::th_t base{24.0f, 50};

        // Измерение скорости выполнения фильров и использование вектора для запоминания данных
        struct stats_t {
            etl::tools::stop_watch md3;
            etl::tools::stop_watch md5;
            etl::tools::stop_watch avg;
            etl::tools::stop_watch exp;
        } stats;        

        for(int i = 0; i < 200; ++i)
        {
            sensor::data::th_t raw = base + sensor::data::th_t{float(sin(i) * 2.0) + float(sin(i/50.0) * 30.0), float(sin(i) * 20.0)};
            raw.temperature = add_rare_noise(raw.temperature, i); // редкие выбросы для проверки медианных фильтров
            raw.humidity = add_rare_noise(raw.humidity, i); // редкие выбросы для проверки медианных фильтров
            //th_t median {raw};    // чтобы можно было убрать сглаживающий медианный фильтр
            
            stats.md3.start();
            float temperature_md3 = temperature_noice_remover.update(raw.temperature);
            stats.md3.stop();
            stats.md5.start();
            float humidity_md5 = humidity_noice_remover.update(raw.humidity);
            stats.md5.stop();
            sensor::data::th_t median {temperature_md3, humidity_md5};
            
            stats.avg.start();
            sensor::data::th_t avg = TH.update(median);
            stats.avg.stop();

            stats.exp.start();
            sensor::data::th_t exp {humidity_exp.update(median.temperature), temperature_exp.update(median.humidity)};
            stats.exp.stop();
            
            trace.print(i); trace.print(";\t");
            trace.print(raw.temperature); trace.print(";\t");
            trace.print(median.temperature); trace.print(";\t");
            trace.print(avg.temperature); trace.print(";\t");
            trace.print(exp.temperature); trace.print(";\t");
            trace.print(raw.humidity); trace.print(";\t");            
            trace.print(median.humidity); trace.print(";\t");  
            trace.print(avg.humidity); trace.print(";\t"); 
            trace.print(exp.humidity); trace.println(";\t");
        }

        // Статистика выполнения фильтров
        trace.println();
        trace.print("TIME;\tus;\t"); 
        trace.print(stats.md3.get_averate_time()); // Tm3
        trace.print(";\t");
        trace.print(stats.avg.get_averate_time()); // Tavg
        trace.print(";\t");
        trace.print(stats.exp.get_averate_time() / 2); // Texp
        trace.print(";\tus;\t");
        trace.print(stats.md5.get_averate_time()); // Hm5
        trace.print(";\t");
        trace.print(stats.avg.get_averate_time()); // Havg
        trace.print(";\t");
        trace.print(stats.exp.get_averate_time() / 2); // Hexp; 
        trace.print("\t");
        trace.println();

        // -------- WEMOS D1 MINI --------------------------------------------
        // TIME;   us;     4;      17;     4;      us;     14;     17;     4
        // median3 = 4us  vs median5 = 14u, md3 почти в 5 раз быстрее
        // -------- ESP32 C3 MINI --------------------------------------------
        // TIME;   us;     3;      7;      3;      us;     5;      7;      3
        // -------------------------------------------------------------------
    }

    template<typename T, typename L>
    uint32_t compare_lookup_time(const T& storage, const L& lookup_table, float step)
    {
        etl::tools::stop_watch stat;   
        for (float r = storage[0].raw; r >= storage[storage.size()-1].raw - 1.0; r-=step) 
        {
            stat.start();
            float t = lookup_table.raw_to_value(r);
            stat.stop();
            UNREFERENCED_PARAMETER(t);
        }
        return stat.get_averate_time();
    }

    void profiler_lookup_ntc(Stream& trace)    // Для термодатчики ntc 3950 50K проверка перевода сопротивления в градусы цельсия
    {
        pgm::array pgm_3950_50K(ntc_temperature_sensor_3950_50K_p);
        auto resistance_lookup = etl::pgm_lookup<float, float>(pgm_3950_50K, etl::lookup_mode::INTERPOLATE, etl::bounds_mode::EXTRAPOLATE);
        etl::tools::stop_watch stat;
        // Выводим темпратуру по сопротивлению
        Serial.println("R;\tT;\t");
        for (float r = pgm_3950_50K[0].raw; r >= pgm_3950_50K[pgm_3950_50K.size()-1].raw - 1.0; r-=0.3) 
        {
            stat.start();
            float t = resistance_lookup.raw_to_value(r);
            stat.stop();
            Serial.printf("%.3f;\t%0.2f;\t\n", r, t);
        }

        trace.println();
        trace.print("TIME;\tus;\t"); 
        trace.println(stat.get_averate_time()); 
        // ESP32C3:         TIME;   us;     15

        etl::vector<etl::lookup_t<float, float>> vec_3950_50K (pgm_3950_50K.size());
        etl::copy(etl::begin(pgm_3950_50K), etl::end(pgm_3950_50K), etl::begin(vec_3950_50K));

        etl::array arr_3950_50K(ntc_temperature_sensor_3950_50K);

        auto resistance_lookup_pgm = etl::pgm_lookup<float, float>(pgm_3950_50K, etl::lookup_mode::INTERPOLATE, etl::bounds_mode::EXTRAPOLATE);
        auto resistance_lookup_arr = etl::array_lookup<float, float>(arr_3950_50K, etl::lookup_mode::INTERPOLATE, etl::bounds_mode::EXTRAPOLATE);
        auto resistance_lookup_vec = etl::vector_lookup<float, float>(vec_3950_50K, etl::lookup_mode::INTERPOLATE, etl::bounds_mode::EXTRAPOLATE);

        float step = 0.1;
        trace.printf("%s;\t%d us;\t\n", "pgm_lookup", compare_lookup_time(pgm_3950_50K, resistance_lookup_pgm, step));  
        trace.printf("%s;\t%d us;\t\n", "arr_lookup", compare_lookup_time(arr_3950_50K, resistance_lookup_arr, step));  
        trace.printf("%s;\t%d us;\t\n", "vec_lookup", compare_lookup_time(vec_3950_50K, resistance_lookup_vec, step));  
    }

}// namespace unittest
}// namespace etl


