#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include <Adafruit_LIS3DH.h>
#include <RH_ASK.h>

#define PLAYER "2"
int THRESHOLD = 10000;

// Hardware setup
Adafruit_LIS3DH lis = Adafruit_LIS3DH();
RH_ASK transmitter(2000, 16, 10);

// Value buffers
#define BUFFER_SIZE 50
#define CHUNK_SIZE 24
float xBuffer[BUFFER_SIZE];
float yBuffer[BUFFER_SIZE];
float zBuffer[BUFFER_SIZE];
int bufferIndex = 0;
unsigned long lastRead = 0;

// Hit configuration
bool isHit = false;
unsigned long lastHit = 0;
int hitTimeout = 2000;

void ShowError() {
    while(true) {
        digitalWrite(LED_BUILTIN_RX, HIGH);
        digitalWrite(LED_BUILTIN_TX, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN_RX, LOW);
        digitalWrite(LED_BUILTIN_TX, LOW);
        delay(500);
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.print("Starting boot process");
    for(int i = 0; i < 2; i++) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println();

    // Initialize the buffers with 0
    for (int i = 0; i < BUFFER_SIZE; i++) {
        xBuffer[i] = 0.0;
        yBuffer[i] = 0.0;
        zBuffer[i] = 0.0;
    }

    if (!lis.begin(0x19)) {   // change this to 0x19 for alternative i2c address
        Serial.println("Couldnt start");
        ShowError();
    }

    lis.setRange(LIS3DH_RANGE_8_G);
    Serial.print("Range = "); Serial.print(2 << lis.getRange());
    Serial.println("G");

    lis.setDataRate(LIS3DH_DATARATE_400_HZ);

    Serial.print("Data rate set to: ");
    switch (lis.getDataRate()) {
        case LIS3DH_DATARATE_1_HZ: Serial.println("1 Hz"); break;
        case LIS3DH_DATARATE_10_HZ: Serial.println("10 Hz"); break;
        case LIS3DH_DATARATE_25_HZ: Serial.println("25 Hz"); break;
        case LIS3DH_DATARATE_50_HZ: Serial.println("50 Hz"); break;
        case LIS3DH_DATARATE_100_HZ: Serial.println("100 Hz"); break;
        case LIS3DH_DATARATE_200_HZ: Serial.println("200 Hz"); break;
        case LIS3DH_DATARATE_400_HZ: Serial.println("400 Hz"); break;

        case LIS3DH_DATARATE_POWERDOWN: Serial.println("Powered Down"); break;
        case LIS3DH_DATARATE_LOWPOWER_5KHZ: Serial.println("5 Khz Low Power"); break;
        case LIS3DH_DATARATE_LOWPOWER_1K6HZ: Serial.println("16 Khz Low Power"); break;
    }

    if (transmitter.init()) {
        Serial.println("433mhz TX initialized!");
    } else {
        ShowError();
    }

    pinMode(LED_BUILTIN_RX, INPUT);
    pinMode(LED_BUILTIN_TX, INPUT);
}

float findMaxValue(const float arr[], int size) {
    if (size <= 0) return NAN;  // Return NaN for invalid size

    float maxVal = arr[0];  // Initialize max value to the first element
    for (int i = 1; i < size; i++) {
        if (arr[i] > maxVal) {
            maxVal = arr[i];  // Update max value if current element is greater
        }
    }
    return maxVal;
}

void loop() {
    if (micros() - lastRead > 2500) {
        lastRead = micros();

        sensors_event_t event;
        lis.getEvent(&event);

        xBuffer[bufferIndex] = lis.x;
        yBuffer[bufferIndex] = lis.y;
        zBuffer[bufferIndex] = lis.z;

        bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;

        bool hitDetected = abs(lis.x) > THRESHOLD || abs(lis.y) > THRESHOLD || abs(lis.z) > THRESHOLD;

        if (hitDetected && !isHit && millis() - lastHit > hitTimeout) {
            Serial.println("Hit detected");
            Serial.print(lis.x);
            Serial.print(",");
            Serial.print(lis.y);
            Serial.print(",");
            Serial.println(lis.z);
            isHit = true;
            lastHit = millis();
        }

        if (!hitDetected && isHit) {
            isHit = false;

            // Build the strings from the buffer values
            String dataString = "hit,";
            dataString += PLAYER;
            dataString += ",";
            dataString += static_cast<int>(findMaxValue(xBuffer, BUFFER_SIZE));
            dataString += ",";
            dataString += static_cast<int>(findMaxValue(yBuffer, BUFFER_SIZE));
            dataString += ",";
            dataString += static_cast<int>(findMaxValue(zBuffer, BUFFER_SIZE));

            // Print the resulting strings to the Serial Monitor
            Serial.println(dataString);
            Serial.println("---------------");

            transmitter.setModeTx();
            transmitter.send((uint8_t *) dataString.c_str(), dataString.length());
            transmitter.waitPacketSent();
            transmitter.setModeIdle();
        }
    }
}