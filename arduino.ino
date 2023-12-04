#include <Adafruit_NeoPixel.h>

#define PIN_BT_STATE PIN7
#define PIN_BT_RX PIN6
#define PIN_BT_TX PIN5
#define PIN_BT_EN PIN4

#define PIN_LEDRING_1 PIN3
#define PIN_LEDRING_2 PIN2

#define NUM_PIXELS 12

Adafruit_NeoPixel pixels1(NUM_PIXELS, PIN_LEDRING_1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels2(NUM_PIXELS, PIN_LEDRING_2, NEO_GRB + NEO_KHZ800);

void setup(){
    pinMode(LED_BUILTIN, OUTPUT);

    pixels1.begin();
    pixels2.begin();
}

void loop() {
    pixels1.clear();
    pixels2.clear();

    for(int i = 0; i < NUM_PIXELS; ++i) {
        pixels1.setPixelColor(i, pixels1.Color(150, 0, 0));
        pixels1.show();
        pixels2.setPixelColor(i, pixels2.Color(0, 150, 0));
        pixels2.show();
        delay(1000);
    }
}