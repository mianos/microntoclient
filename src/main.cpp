#include <Arduino.h>

#include <WiFi.h>

#include "SecMilli.h"
#include "ntp.h"

const char* ssid = "iot";
const char* password = "iotlongpassword";

// Custom colors
#define CUSTOM_DARK 0x3000 // Background color

MiniNtp *mntp;

void setup() {

  Serial.begin(115200);

  Serial.println(" Connecting to: ");
  Serial.println(" ");
  Serial.print(" ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("not connected");
    delay(200);
  }

  Serial.println(" ");
  Serial.println(" WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
    mntp = new MiniNtp{"131.84.1.10", [](){ printf("time good\n"); }};
  
  delay(1500);
}

unsigned long last_print;

void loop() {
	mntp->run();
    if (last_print == 0 || last_print < millis() - 1000) {
        last_print = millis();
        auto nn = mntp->now();
        nn.print();
    }
}
