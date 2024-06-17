#define WEBSOCKETS_SERVER_CLIENT_MAX (10)
#define LED_BUILTIN 2

#include <Arduino.h>
#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WebSocketsServer.h>
#include "RollingAverage.h"
#include "Credentials.h"

RH_ASK receiver(2000, 26, 25, 0);
WiFiMulti WiFiMulti;
WebSocketsServer webSocket = WebSocketsServer(81);
unsigned long lastSendTime = 0;

#define MESSAGE_SIZE 50
uint8_t buf[MESSAGE_SIZE];
const uint8_t buflen = sizeof(buf);
char lastMessage[MESSAGE_SIZE] = "";
#define THRESHOLD_TIMEOUT 1000

RollingAverage averages[6] = {
    RollingAverage(15, 3900),  
    RollingAverage(15, 4005),
    RollingAverage(15, 4005),
    RollingAverage(15, 4005),
    RollingAverage(15, 4005),
    RollingAverage(15, 3500)
};
bool isTriggered[6] = {false, false, false, false, false, false};
unsigned long lastTriggered[6] = {0, 0, 0, 0, 0, 0};

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
    const uint8_t* src = (const uint8_t*) mem;
    Serial.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
    for(uint32_t i = 0; i < len; i++) {
        if(i % cols == 0) {
            Serial.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
        }
        Serial.printf("%02X ", *src);
        src++;
    }
    Serial.printf("\n");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

				// send message to client
				webSocket.sendTXT(num, "Connected<EOF>\n");
            }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\n", num, payload);

            // send message to client
            // webSocket.sendTXT(num, "message here");

            // send data to all connected clients
            webSocket.broadcastTXT("message here<EOF>\n");
            break;
        case WStype_BIN:
            Serial.printf("[%u] get binary length: %u\n", num, length);
            hexdump(payload, length);

            // send message to client
            // webSocket.sendBIN(num, payload, length);
            break;
        case WStype_PING:
        case WStype_PONG:
		case WStype_ERROR:
		case WStype_FRAGMENT_TEXT_START:
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
		case WStype_FRAGMENT_FIN:
            Serial.print("Unhandled socket event: ");
            Serial.println(type);
            break;
    }
}

void setup() {
    Serial.begin(115200);

    if (!receiver.init()) {
        Serial.println("Could not init 433mhz driver");
    }

    Serial.setDebugOutput(true);

    Serial.println();
    Serial.println();
    Serial.println();

    for(uint8_t t = 1; t > 0; t--) {
        Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }

    WiFiMulti.addAP(ACCESSPOINT.c_str(), WIFIKEY.c_str());

    Serial.print("Connecting: ");
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println(" - Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    pinMode(LED_BUILTIN, OUTPUT);
}

char sensorBuffer[100];
bool ledOn = true;

void loop() {
    uint16_t p1 = analogRead(33);
    uint16_t p2 = analogRead(32);
    uint16_t p3 = analogRead(35);
    uint16_t p4 = analogRead(34);
    uint16_t p5 = analogRead(39);
    uint16_t p6 = analogRead(36);

/*    Serial.print(p1);
    Serial.print(",");
    delay(100);
    return;*/

    averages[0].addValue(p1);
    averages[1].addValue(p2);
    averages[2].addValue(p3);
    averages[3].addValue(p4);
    averages[4].addValue(p5);
    averages[5].addValue(p6);

    for(int i = 0; i < 6; i++) {
        if (millis() - lastTriggered[i] > THRESHOLD_TIMEOUT && averages[i].isThresholdReached() && !isTriggered[i]) {
            lastTriggered[i] = millis();
            Serial.print("Threshold reached for: ");
            Serial.print(i);
            Serial.print(" with average values: ");
            Serial.println(averages[i].getAverage());

            sprintf(sensorBuffer, "pocket,%u", i);

            Serial.print("Message: ");
            Serial.println(sensorBuffer);

            webSocket.broadcastTXT(sensorBuffer);

            isTriggered[i] = true;
        } else if (!averages[i].isThresholdReached()) {
            isTriggered[i] = false;
        }
    }

    webSocket.loop();

    uint8_t msgLen;
    msgLen = buflen - 1;
    if (receiver.recv(buf, &msgLen)) {
        buf[msgLen] = '\0';
        char* message = (char *) buf;
        if (strncmp(message, "hit", 3) == 0 && strcmp(message, lastMessage) != 0) {
            strcpy(lastMessage, message);
            Serial.print("Received: ");
            Serial.println((char *) buf);
            webSocket.broadcastTXT((char *) buf);
        }
    }
}