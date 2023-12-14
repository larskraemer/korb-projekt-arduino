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

auto hsl_to_rgb(int H, uint8_t S, uint8_t L) -> uint32_t {
    const auto Hp = (float) H / 60.0f;
    const auto k = (float) floor(Hp / 2);
    const auto h = Hp - 2 * k;

    const auto C = (1 - abs(2 * L/255.0f - 1)) * S/255.0f;
    const auto X = (1 - abs(h - 1)) * C;

    float r,g,b;

    switch((int) Hp) {
    case 0: r = C; g = X; b = 0; break;
    case 1: r = X; g = C; b = 0; break;
    case 2: r = 0; g = C; b = X; break;
    case 3: r = 0; g = X; b = C; break;
    case 4: r = X; g = 0; b = C; break;
    case 5: r = C; g = 0; b = X; break;
    }

    const auto m = L - 128 * C;
    r *= 255; r += m;
    g *= 255; g += m;
    b *= 255; b += m;

    return Adafruit_NeoPixel::Color(r, g, b);
}

void loop() {
    static int t = 0;

    pixels1.setPixelColor(t % NUM_PIXELS, hsl_to_rgb(t, 255, 128));
    pixels1.show();
    ++t;
    if(t == 11*360) t = 0;
    delay(100);
}
