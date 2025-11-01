// Вспомогательный инструмента для работы по протоколу espnow для esp8266/esp32
#pragma once
#ifndef ESP_MANAGER_H
#define ESP_MANAGER_H

#include "Arduino.h"
#include "etl_espwifi.h"
#include "etl_optional.h"
#include "etl_vector.h"
#include "etl_utility.h"

#ifdef ESP32
    #include <WiFi.h>
    #include <esp_now.h>
    #include <esp_wifi.h>
#elif ESP8266
    #include <ESP8266WiFi.h>
    #include <espnow.h>
    // Добавим из ESP32 для совместимости
    typedef enum {
        ESP_NOW_SEND_SUCCESS = 0,       /**< Send ESPNOW data successfully */
        ESP_NOW_SEND_FAIL,              /**< Send ESPNOW data fail */
    } esp_now_send_status_t;
    #define ESP_OK          0       /*!< esp_err_t value indicating success (no error) */
    #define ESP_FAIL        -1      /*!< Generic esp_err_t code indicating failure */   
#endif

namespace etl {
namespace espnow {

    class board {
        public:
        static String get_mac_address();
    };// namespace board

    const uint8_t broadcast_address[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; 

    struct endpoint_t
    {
        etl::vector<uint8_t> mac {broadcast_address}; 

        endpoint_t() = default;
        endpoint_t(const uint8_t *mac_addr) : mac(6, 0x00) {
            memcpy(mac.data(), mac_addr, mac.size());        
        }
        endpoint_t(const endpoint_t& item) : mac{item.mac} {}
        endpoint_t(const String& text_macaddress) : mac{from_string(text_macaddress)} {}

        endpoint_t(std::initializer_list<uint8_t> init_list) : mac(6, 0x00) {
            if(init_list.size() == 6) {
                std::copy(init_list.begin(), init_list.end(), mac.begin());
            }
        }

        String string() const {
            return to_string(mac);
        }

        static String to_string(const etl::vector<uint8_t>& mac_address)
        {
            String s;
            for(auto b : mac_address)
            {
                if(!s.isEmpty()) s += ":";
                s += String(b, HEX);
            }
            s.toUpperCase();
            return s;
        }

        static etl::vector<uint8_t> from_string(const String& text)
        {
            etl::vector<uint8_t> temp_mac(6,0x00);
            if (sscanf(text.c_str(), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
                    &temp_mac[0], &temp_mac[1], &temp_mac[2],
                    &temp_mac[3], &temp_mac[4], &temp_mac[5]) != 6) {
                Serial.printf("[ERROR] in from_string %s to mac address\n", text.c_str());
            }
            return etl::move(temp_mac);
        }

        bool operator==(const endpoint_t& other) const {
            return mac == other.mac;
        }

        bool operator!=(const endpoint_t& other) const {
            return !operator==(other);
        }
    };

    template<typename T>   // define uint32_t timestamp; in T
    class manager {
    private:
        bool is_initialized = false;
        etl::optional<T> last_received_message;
        
        // Статические методы для callback'ов (требуются ESP-NOW)
        static void static_on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
            if (instance() != nullptr) {
                const endpoint_t to_addr (mac_addr);
                instance()->on_data_sent(to_addr, status);
            }
        }
        
        static void static_on_data_receive(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
            if (instance() != nullptr) {
                const endpoint_t from_addr (mac_addr);
                instance()->on_data_recieve(from_addr, incomingData, len);
            }
        }

    protected:    
        // Виртуальные методы для переопределения в производных классах
        virtual void on_data_sent(const endpoint_t& to, esp_now_send_status_t status) {
            // Базовая реализация - можно переопределить
            Serial.printf("Sent to %s %s\n", to.string().c_str(), status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
        }
        
        virtual void on_data_recieve(const endpoint_t& from, const uint8_t *incomingData, int len) {
            // Базовая реализация - можно переопределить
            T msg;
            memcpy(&msg, incomingData, sizeof(T));
            last_received_message = msg;
            Serial.printf("Recieved from %s bytes: %d \n", from.string().c_str(), len);
        }

    public:
        manager() {
            set_instance(this);
        }

        virtual ~manager() {
            if (is_initialized) {
                esp_now_deinit();
            }

            if (instance() == this) {
                set_instance(nullptr);
            }
        }

        // Статический метод для доступа к экземпляру
        static manager* instance() {
            static manager* instance_ptr = nullptr;
            return instance_ptr;
        }

        static void set_instance(manager* ptr) {
            auto instance_value = instance();
            instance_value = ptr;
        #ifdef ESP8266
            UNREFERENCED_PARAMETER(instance_value); // warning fix for ESP8266
        #endif    
        //    instance() = ptr;
        }

        bool begin() {
            WiFi.mode(WIFI_STA);
                
            if (esp_now_init() != ESP_OK) {
                Serial.println("Ошибка инициализации ESP-NOW");
                return false;
            }
            
            // Регистрация callback'ов со статическими методами
        #ifdef ESP8266
            esp_now_set_self_role(ESP_NOW_ROLE_COMBO);       // Установка роли для ESP8266 
        #endif
            esp_now_register_send_cb(static_on_data_sent);
            esp_now_register_recv_cb(static_on_data_receive);
            
            is_initialized = true;
            Serial.println("ESP-NOW инициализирован");
            Serial.print("MAC: ");  Serial.println(espnow::board::get_mac_address());
            
            return true;
        }

        bool add_peer(const uint8_t *mac_addr, uint8_t channel = 0, bool encrypt = false) {
            if (!is_initialized) return false;
            
        #ifdef ESP8266 
            int result = esp_now_add_peer(const_cast<uint8_t*>(mac_addr), ESP_NOW_ROLE_COMBO, channel, NULL, 0);
        #elif ESP32
            esp_now_peer_info_t peerInfo;
            memset(&peerInfo, 0, sizeof(peerInfo));
            memcpy(peerInfo.peer_addr, mac_addr, 6);
            peerInfo.channel = channel;
            peerInfo.encrypt = encrypt;
            
            esp_err_t result = esp_now_add_peer(&peerInfo);
        #endif

            if (result != ESP_OK) {
                Serial.println("Ошибка добавления пира");
                return false;
            }
            
            Serial.print("Пир добавлен: "); Serial.println(espnow::board::get_mac_address());
            return true;
        }

        bool remove_peer(const endpoint_t& mac_addr) {
            if (!is_initialized) return false;
        //    return esp_now_del_peer(mac_addr.mac.data()) == ESP_OK;
            return false;   // :DEBUG:
        }

        bool send_message(const endpoint_t& mac_addr, const T &message) {
            if (!is_initialized) return false;
            
            T msg = message;
            msg.timestamp = millis();
            
        #ifdef ESP8266
            int result = esp_now_send(mac_addr.mac.data(), (uint8_t *)&msg, sizeof(msg));
        #elif ESP32        
        //    esp_err_t result = esp_now_send(mac_addr.mac.data(), (uint8_t *)&msg, sizeof(msg));
        #endif
            //return result == ESP_OK;
            return false;   // :DEBUG:
        }

        // Broadcast сообщение
        bool broadcast_message(const T &message) {
            return send_message(broadcast_address, message);
        }

        // Получение последнего сообщения
        etl::optional<T> get_last_received_message() {
            return last_received_message;
        }

        bool ready() {
            return is_initialized;
        }
    };

}// namespace espnow
}// namespace etl
#endif//ESP_MANAGER_H