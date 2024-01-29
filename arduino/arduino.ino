#include <Adafruit_NeoPixel.h>

constexpr int LED_PIN = 2;
constexpr int NUM_PIXELS = 12;
constexpr int PIN_BEEP = 11;

// drive pins
constexpr int PIN_IA1 = 9;
constexpr int PIN_IA2 = 6;
constexpr int PIN_IB1 = 10;
constexpr int PIN_IB2 = 5;

// Sensor YL-70 Pins
constexpr int SENSOR_LEFT = A0;
constexpr int SENSOR_RIGHT = A1;

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
    float x = 0;
    float y = 0;
};

// YL-70 Code
struct SensorReadings {
    bool left;
    bool right;
};

SensorReadings readLineSensors() {
    auto left_val = analogRead(SENSOR_LEFT);
    auto right_val = analogRead(SENSOR_RIGHT);

    Serial.println(left_val);
    Serial.println(right_val);

    return {left_val > 127, right_val > 127};
}

auto decode_serial_drive(const char* command, size_t len) -> drive_coords {
    // Expect exactly 5 bytes
    if(len != 5) return {};
    auto x = decode_hex_byte(command + 1);
    auto y = decode_hex_byte(command + 3);

    auto out = drive_coords{};

    out.x = (static_cast<float>(x) - 127) / 127.0;
    out.y = (static_cast<float>(y) - 127) / 127.0;
    return out;
}

void set_motor_power(int pinA, int pinB, int power) {
  if(power > 0) analogWrite(pinA, (power > 0) ? +power : 0);
  else analogWrite(pinB, (power < 0) ? -power : 0);
}

void set_left_motor(int power) {
  set_motor_power(PIN_IA2, PIN_IB2, power);
}

void set_right_motor(int power) {
  set_motor_power(PIN_IA1, PIN_IB1, power);
}

void do_beep(int pin, bool enable) {
    static int next_change = millis();
    static bool current_state = false;

    auto current_time = millis();
    if(current_time > next_change) {
        next_change = current_time;
        current_state = !current_state;
        tone(pin, 220);
    }
}

void do_serial_drive(const char* command, size_t len) {
    auto [out_x, out_y] = decode_serial_drive(command, len);

    auto left_motor_power = out_x - out_y;
    auto right_motor_power = out_x + out_y;

    set_motor_power(5, 6, left_motor_power * 127);
    set_motor_power(9, 10, right_motor_power * 127);
}

void do_serial_color(const char* command, size_t len) {
    // expect exactly 9 bytes
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

void do_followLine(const SensorReadings& readings) {
    // Define a steering factor for smoother turns
    const int steering_factor = 30;  // Adjust this value based on testing

    if (!readings.left && !readings.right) {
        // Both sensors are off the line: move forward
        Serial.println("Moving forward");
        set_motor_power(PIN_IB1,PIN_IA1, 100);
        set_motor_power(PIN_IA2, PIN_IB2, 100);
    } else if (readings.left && !readings.right) {
        // Left sensor is on the line: turn left more gently
        Serial.println("steering right");
        set_motor_power(PIN_IB1,PIN_IA1, 100 - steering_factor);
        set_motor_power(PIN_IA2, PIN_IB2, 100);
    } else if (!readings.left && readings.right) {
        // Right sensor is on the line: turn right more gently
        Serial.println("steering left");
        set_motor_power(PIN_IB1,PIN_IA1, 100);
        set_motor_power(PIN_IA2, PIN_IB2, 100 - steering_factor);
    } else {
        // Both sensors are on the line: stop or move slowly forward
        Serial.println("das ist nicht");
        set_motor_power(PIN_IB1,PIN_IA1, 0);
        set_motor_power(PIN_IA2, PIN_IB2, 0);
    }
}


void setup(void) {
    Serial.begin(9600);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(9, OUTPUT);
    pinMode(10, OUTPUT);
    pix.begin();

    // setup YL-70 Sensor Modul
    pinMode(SENSOR_LEFT, INPUT);
    pinMode(SENSOR_RIGHT, INPUT);
}

void loop(void) {
    auto readings = readLineSensors();
    //do_followLine(readings);

    set_left_motor(100);
    set_right_motor(100);


    
// '    char buffer[64];

//     while(Serial.available() > 0) {
//         auto read_bytes = Serial.readBytesUntil('\n', buffer, sizeof(buffer) / sizeof(buffer[0]));

//         if(read_bytes == 0) return;

//         switch (buffer[0])
//         {
//         case 'M': do_serial_drive(buffer, read_bytes); break;
//         case 'C': do_serial_color(buffer, read_bytes); break;
//         case 'L': {auto readings = readLineSensors(); do_followLine(readings); break;}
//         default: break;
//         }
//     }'
}