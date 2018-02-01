extern "C" {
#include "user_interface.h"
}

#include <WebSocketsServer.h>
#include <WebSocketsClient.h>

#include "webSocket.h"

WebSocketsServer webSocketServer = WebSocketsServer(81);

SignalKClientInfo signalKClientInfo = { 
  .host = "", 
  .port = 80, 
  .path = "/signalk/v1/stream",
  .connected = false
};

os_timer_t  wsClientReconnectTimer; // once request cycle starts, this timer set so we can send when ready
bool  readyToReconnectWs = false;

// forward declarations

void webSocketServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void interruptWsReconnect(void *pArg);
void connectWebSocketClient();
void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length);

void setupWebSocket() {
  os_timer_setfn(&wsClientReconnectTimer, interruptWsReconnect, NULL);

  webSocketServer.begin();
  webSocketServer.onEvent(webSocketServerEvent);
  signalKClientInfo.client.onEvent(webSocketClientEvent);

  connectWebSocketClient();
}


void handleWebSocket() {
  webSocketServer.loop();
  if (readyToReconnectWs) {
    readyToReconnectWs = false;
    connectWebSocketClient();
  }
  
  if (signalKClientInfo.connected) {
    signalKClientInfo.client.loop();    
  }
}


void restartWebSocketClient() {
  if (signalKClientInfo.connected) {
    signalKClientInfo.client.disconnect(); // disconnect will set timer to reconnect. if not connected it was already running
  }
}


void connectWebSocketClient() {
  SignalKClientInfo *skci = &signalKClientInfo;  // save some typing
  if ( (skci->host.length() > 0) && 
       (skci->port > 0) && (skci->port < 65535) && 
       (skci->path.length() > 0)) {
    Serial.println("Websocket client starting!");
  } else {
      os_timer_arm(&wsClientReconnectTimer, 10000, false);
      return;
  }

  skci->client.begin(skci->host, skci->port, skci->path + "?subscribe=none");
  skci->connected = true;
}


void interruptWsReconnect(void *pArg) {
  readyToReconnectWs = true;
}


void webSocketClientEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      signalKClientInfo.connected = false;
      os_timer_arm(&wsClientReconnectTimer, 10000, false);
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED: {
      signalKClientInfo.connected = true;
      Serial.printf("[WSc] Connected to url: %s\n", payload);
      // send message to server when Connected
      // webSocket.sendTXT("Connected");
    }
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);

      // send message to server
      // webSocket.sendTXT("message here");
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      //hexdump(payload, length);

      // send data to server
      // webSocket.sendBIN(payload, length);
      break;
  }

}


void webSocketServerEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocketServer.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        
                // send message to client
                //webSocketServer.sendTXT(num, "Connected");
            }
            break;
        case WStype_TEXT:
            //Serial.printf("[%u] get Text: %s\n", num, payload);

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            // webSocket.broadcastTXT("message here");
            break;
        case WStype_BIN:
            //Serial.printf("[%u] get binary length: %u\n", num, length);
            //hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
    }

}
