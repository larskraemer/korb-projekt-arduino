#include <Adafruit_NeoPixel.h>

constexpr int DRIVE_PINS[] = {
    3,5,6,9
};

constexpr int LED_PIN = 2;
constexpr int NUM_PIXELS = 99;

auto pix = Adafruit_NeoPixel{NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800};

auto decode_nibble(char c) -> uint8_t {
    if('0' <= c && c <= '9') return c - '0' + 0x00;
    if('a' <= c && c <= 'f') return c - 'a' + 0x0a;
    if('A' <= c && c <= 'F') return c - 'A' + 0x0a;
    return 0;
}

auto decode_hex_byte(const char* str) -> uint8_t {
    auto high = decode_nibble(*str);
    auto low = decode_nibble(*(str+1));
    return (high << 4) | low;
}

struct drive_coords {
    int x = 0;
    int y = 0;
};

auto decode_serial_drive(const char* command, size_t len) -> drive_coords {
    // Expect exactly 5 bytes
    if(len != 5) return {};
    auto x = decode_hex_byte(command + 1);
    auto y = decode_hex_byte(command + 3);

    auto out = drive_coords{};

    out.x = (static_cast<int>(x) - 127) * 2;
    out.y = (static_cast<int>(y) - 127) * 2;
    return out;
}   

void do_serial_drive(const char* command, size_t len) {
    auto [out_x, out_y] = decode_serial_drive(command, len);

    analogWrite(DRIVE_PINS[0], (out_x > 0) ? +out_x : 0);
    analogWrite(DRIVE_PINS[1], (out_x < 0) ? -out_x : 0);
    analogWrite(DRIVE_PINS[2], (out_y > 0) ? +out_y : 0);
    analogWrite(DRIVE_PINS[3], (out_y < 0) ? -out_y : 0);
}

void do_serial_color(const char* command, size_t len) {
    if(len != 9) return;
    auto a = decode_hex_byte(command + 1);
    auto r = decode_hex_byte(command + 3);
    auto g = decode_hex_byte(command + 5);
    auto b = decode_hex_byte(command + 7);

    for(int i = 0; i < NUM_PIXELS; i++) {
        pix.setPixelColor(i, Adafruit_NeoPixel::Color(r, g, b));
    }
    pix.show();
}

void setup(void) {
    Serial.begin(9600);
    for(int i = 0; i < (sizeof(DRIVE_PINS)/sizeof(DRIVE_PINS[0])); ++i) {
        pinMode(DRIVE_PINS[i], OUTPUT);
    }
    pix.begin();
}

void loop(void) {

    char buffer[64];
    auto read_bytes = Serial.readBytesUntil('\n', buffer, sizeof(buffer) / sizeof(buffer[0]));

    if(read_bytes == 0) return;

    switch (buffer[0])
    {
    case 'M': do_serial_drive(buffer, read_bytes); break;
    case 'C': do_serial_color(buffer, read_bytes); break;
    default: break;
    }
}