#include "http_client.h"

String get_full_url(String url_part, PRINTER_CONFIG * config)
{
    if (config == NULL){
        config = get_current_printer_config();
    }

    return "http://" + String(config->klipper_host) + ":" + String(config->klipper_port) + url_part;
}

void configure_http_client(HTTPClient &client, String url, bool stream, int timeout, PRINTER_CONFIG * config)
{
    if (config == NULL){
        config = get_current_printer_config();
    }

    if (stream){
        client.useHTTP10(true);
    }

    if (timeout > 0){
        client.setTimeout(timeout);
        client.setConnectTimeout(timeout);
    }

    client.begin(url);

    if (config->auth_configured) {
        client.addHeader("X-Api-Key", config->klipper_auth);
    }
}