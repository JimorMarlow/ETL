#include "etl_espnow.h"


// ВНИМАНИЕ, этот файл почему-то не компилируется - разобраться
namespace etl {
namespace espnow {
    String board::get_mac_address() {
        // Подключение к Wi-Fi не требуется для получения MAC-адреса.
        // WiFi.mode(WIFI_STA); // Устанавливаем режим работы (в данном случае, как станция)
        return WiFi.macAddress();
    }
} // namespace espnow
} // namespace etl