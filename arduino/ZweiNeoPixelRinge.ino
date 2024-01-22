#include <Adafruit_NeoPixel.h>

#define PIN_A0 A0
#define PIN_2 2
#define PIN_3 3
#define NUMPIXELS 13 // Anzahl der LEDs im Ring
#define THRESHOLD 70 // Schwellenwert

//                           Dies ist der Code für den "Deko" Arduino der nicht für die Fahrfunktionen zuständig ist.

Adafruit_NeoPixel pixels_2(NUMPIXELS, PIN_2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_3(NUMPIXELS, PIN_3, NEO_GRB + NEO_KHZ800);

void setup() {
  pixels_2.begin();
  pixels_3.begin();
}

void loop() {
  int sensorValue = analogRead(PIN_A0);
  
  if (sensorValue > THRESHOLD) {
    setRainbow(pixels_2, pixels_3); // Regenbogen
  } else {
    setColor(pixels_2, pixels_3, 255, 0, 0); // Rot
  }
}

void setColor(Adafruit_NeoPixel &pixels_2, Adafruit_NeoPixel &pixels_3, int r, int g, int b) {
  for(int i=0; i<NUMPIXELS; i++) {
    pixels_2.setPixelColor(i, pixels_2.Color(r, g, b));
    pixels_3.setPixelColor(i, pixels_3.Color(r, g, b));
  }
  pixels_2.show();
  pixels_3.show();
}

void setRainbow(Adafruit_NeoPixel &pixels_2, Adafruit_NeoPixel &pixels_3) {
  uint16_t i, j;

  for(j=0; j<256; j++) { // j iteriert über 256 Schritte
    for(i=0; i< pixels_2.numPixels(); i++) { // i iteriert über jede LED
      pixels_2.setPixelColor(i, Wheel((i+j) & 255));
      pixels_3.setPixelColor(i, Wheel((i+j) & 255));
    }
    pixels_2.show();
    pixels_3.show();
    delay(5);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return pixels_2.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels_2.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels_2.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
