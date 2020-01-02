#include <Arduino.h>

#include <WiFi.h>
#include <U8g2lib.h>

#include "SecMilli.h"
#include "ntp.h"

const char* ssid = "iot";
const char* password = "iotlongpassword";

// Custom colors
#define CUSTOM_DARK 0x3000 // Background color

MiniNtp *mntp;

//U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 16, /* data=*/ 17);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ 16, /* clock=*/ 15, /* data=*/ 4); 

void setup() {
    u8g2.begin();
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

    setenv("TZ", "AEST-10AEDT,M10.1.0,M4.1.0/3", 1);
    tzset();
  
  delay(1500);
}

unsigned long last_print;

void loop() {
	mntp->run();
    if (last_print == 0 || last_print < millis() - 10) {
        last_print = millis();
        auto nn = mntp->now();
       
	  char buffer[40];
      //nn.as_iso(buffer, sizeof (buffer));
      //nn.local(buffer, sizeof (buffer), 10, true);
      nn.local(buffer, sizeof (buffer));
	  u8g2.firstPage();
	  do {
		u8g2.setFont(u8g2_font_logisoso16_tn);
		u8g2.drawStr(0, 63, buffer);
	  } while (u8g2.nextPage() );
    }
}
