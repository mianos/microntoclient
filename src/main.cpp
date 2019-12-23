#include <Arduino.h>

#include <WiFi.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#include <TaskScheduler.h>

#include "SecMilli.h"
#include "ntp.h"

#define TFT_CS   27
#define TFT_DC   26
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_RST  5
#define TFT_MISO 12
//#define TFT_LED   5  // GPIO not managed by library

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

const char* ssid = "iot";
const char* password = "iotlongpassword";

// Custom colors
#define CUSTOM_DARK 0x3000 // Background color

MiniNtp *mntp;

Scheduler ts;
Task *get_time_task;
Task *receive_task;
Task *print_task;


void receive_proc() {
	if (mntp->receive()) {
        Serial.println("GOT");
		receive_task->disable();
	}
}

void print_proc() {
	if (mntp->is_good()) {
		char buf[100];
		auto nn = mntp->now();
		Serial.printf("RESULT: %s\n", nn.as_iso(buf, sizeof(buf)));
	}
}

void get_time() {
	mntp->send();
	receive_task->setIterations(3);
	receive_task->enableDelayed();
}

void setup() {

  Serial.begin(115200);

  tft.begin();
  //tft.setRotation(0); // 0 = 0°  1 = 90°  2 = 180°  3 = 270°
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
//  tft.setTextWrap(true);
//  tft.setCursor(0, 170);
//  tft.setTextSize(2);

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
	get_time_task = new Task(5000, TASK_FOREVER, get_time, &ts, true);
	receive_task = new Task(1000, 3, receive_proc, &ts);
	print_task = new Task(1000, TASK_FOREVER, print_proc, &ts, true);
  
  delay(1500);
#if 0
   tft.fillScreen(ILI9341_BLACK);
  
  #define BOXSIZE 40
  // make the color selection boxes
  tft.fillRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_RED);
  tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9341_YELLOW);
  tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, ILI9341_GREEN);
  tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, ILI9341_CYAN);
  tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, ILI9341_BLUE);
  tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, ILI9341_MAGENTA);
 
  // select the current color 'red'
  tft.drawRect(0, 0, BOXSIZE, BOXSIZE, ILI9341_WHITE);
  delay(5000);
#endif
}

void loop() {
	ts.execute();
}
