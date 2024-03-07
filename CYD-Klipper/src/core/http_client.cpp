#include "http_client.h"
#include "../conf/global_config.h"

String get_full_url(String url_part)
{
    return "http://" + String(get_current_printer_config()->klipper_host) + ":" + String(get_current_printer_config()->klipper_port) + url_part;
}

void configure_http_client(HTTPClient &client, String url, bool stream, int timeout)
{
    if (stream){
        client.useHTTP10(true);
    }

    if (timeout > 0){
        client.setTimeout(timeout);
        client.setConnectTimeout(timeout);
    }

    client.begin(url);

    if (get_current_printer_config()->auth_configured) {
        client.addHeader("X-Api-Key", get_current_printer_config()->klipper_auth);
    }
}