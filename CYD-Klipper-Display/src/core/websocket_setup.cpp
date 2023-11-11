#include <WebSocketsClient.h>
#include "../conf/global_config.h"

WebSocketsClient webSocket;

void websocket_event(WStype_t type, uint8_t * payload, size_t length) {
	switch(type) {
		case WStype_DISCONNECTED:
			Serial.printf("[WSc] Disconnected!\n");
			break;
		case WStype_CONNECTED:
			Serial.printf("[WSc] Connected to url: %s\n", payload);

			// send message to server when Connected
			webSocket.sendTXT("Connected");
			break;
		case WStype_TEXT:
			Serial.printf("[WSc] get text: %s\n", payload);

			// send message to server
			// webSocket.sendTXT("message here");
			break;
		case WStype_BIN:
		case WStype_ERROR:			
		case WStype_FRAGMENT_TEXT_START:
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
		case WStype_FRAGMENT_FIN:
			break;
	}
}

void websocket_process(){
    webSocket.loop();
}

void websocket_setup(){
    webSocket.begin("192.168.0.122", global_config.klipperPort, "/websocket");
    webSocket.onEvent(websocket_event);
    webSocket.setReconnectInterval(5000);
}