#include "etl_settings.h"

namespace etl 
{
    namespace settings 
    {
        String to_string(sender_id id) {
            switch (id) {
                case sender_id::broadcast:
                    return "broadcast";
                case sender_id::system:
                    return "system";
                case sender_id::webui:
                    return "webui";
                case sender_id::setup:
                    return "setup";
                case sender_id::view:
                    return "view";
                case sender_id::wifi:
                    return "wifi";
                case sender_id::mqtt:
                    return "mqtt";
                case sender_id::user1:
                    return "user1";
                case sender_id::user2:
                    return "user2";
                case sender_id::user3:
                    return "user3";
                default:
                    return "unknown";
            }
        }
    }//..settings 
}
