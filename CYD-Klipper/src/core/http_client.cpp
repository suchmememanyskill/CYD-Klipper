#include "http_client.h"
#include "../conf/global_config.h"

String get_full_url(String url_part)
{
    return "http://" + String(global_config.klipperHost) + ":" + String(global_config.klipperPort) + url_part;
}

void configure_http_client(HTTPClient &client, String url, bool stream, int timeout)
{
    Serial.println(url);
    if (stream){
        client.useHTTP10(true);
    }

    if (timeout > 0){
        client.setTimeout(timeout);
        client.setConnectTimeout(timeout);
    }

    client.begin(url);

    if (global_config.auth_configured) {
        client.addHeader("X-Api-Key", global_config.klipper_auth);
    }
}