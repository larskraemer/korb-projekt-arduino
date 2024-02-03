#include <Adafruit_NeoPixel.h>


constexpr int LED_PIN1 = 2;
constexpr int LED_PIN2 = 3;

constexpr int NUM_PIXELS = 12;
constexpr int PIN_BEEP = 11;

// Sensor YL-70 Pins
constexpr int SENSOR_LEFT = A0;
constexpr int SENSOR_RIGHT = A1;


// 
constexpr int PIN_DISTANCE_SENSOR_ECHO = 7;
constexpr int PIN_DISTANCE_SENSOR_TRIG = 8;

bool emergency_stop = false;

template<class T, size_t N>
struct vec {
    T comps[N];
};

template<class T>
struct vec<T, 2>
{
    union{
        struct { T x; T y; };
        struct { T left; T right; };
    };
};

using vec2 = vec<float, 2>;



vec2 motor_power = {0, 0};


enum Mode {
    MANUAL = 0,
    FOLLOWLINE = 1
};

int mode = MANUAL;


struct motor_driver {
    int pinA;
    int pinB;

    void begin() {
        pinMode(pinA, OUTPUT);
        pinMode(pinB, OUTPUT);
    }

    // Power is an int between -255 and +255
    void set_power(int power) {
        analogWrite(pinA, (power > 0) ? +power : 0);
        analogWrite(pinB, (power < 0) ? -power : 0);
    }
};

auto left_motor = motor_driver{10, 9};
auto right_motor = motor_driver{5, 6};

auto pix1 = Adafruit_NeoPixel{NUM_PIXELS, LED_PIN1, NEO_GRB + NEO_KHZ800};
auto pix2 = Adafruit_NeoPixel{NUM_PIXELS, LED_PIN2, NEO_GRB + NEO_KHZ800};

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

#define clamp(v, vmin, vmax) max(min((v), (vmax)), (vmin))

auto calculate_motor_power(vec2 d) -> vec2 {
    return {
        clamp(d.y + d.x, -1, 1),
        clamp(d.y - d.x, -1, 1)
    };

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


void do_serial_drive(const char* command, size_t len) {
    auto [out_x, out_y] = decode_serial_drive(command, len);

    motor_power = calculate_motor_power({ out_x, out_y });

}

void do_serial_color(const char* command, size_t len) {
    // expect exactly 9 bytes
    if(len != 9) return;
    auto a = decode_hex_byte(command + 1);
    auto r = decode_hex_byte(command + 3);
    auto g = decode_hex_byte(command + 5);
    auto b = decode_hex_byte(command + 7);

    for(int i = 0; i < NUM_PIXELS; i++) {
        pix1.setPixelColor(i, Adafruit_NeoPixel::Color(r, g, b));
        pix2.setPixelColor(i, Adafruit_NeoPixel::Color(r, g, b));
    }
    pix1.show();
    pix2.show();
}

void do_mode_switch(const char* command, size_t len) {
    switch(command[1]) {
    case 'E': {
        mode = MANUAL;
    } break;
    case 'A': {
        pix1.clear(); pix1.show();
        pix2.clear(); pix2.show();
        mode = FOLLOWLINE;
    } break;
    }
}


void do_followLine(const SensorReadings& readings) {
    // Define a steering factor for smoother turns
    const int steering_factor = 30;  // Adjust this value based on testing

    if (!readings.left && !readings.right) {
        // Both sensors are off the line: move forward
        left_motor.set_power(255);
        right_motor.set_power(255);
        pix1.clear();
        pix1.setPixelColor(0, Adafruit_NeoPixel::Color(255, 255, 255));
        pix1.show();
    } else if (readings.left && !readings.right) {
        // Left sensor is on the line: turn left more gently
        left_motor.set_power(100);
        right_motor.set_power(255);

        pix1.clear();
        pix1.setPixelColor(3, Adafruit_NeoPixel::Color(255, 255, 255));
        pix1.show();
    } else if (!readings.left && readings.right) {
        left_motor.set_power(0);
        right_motor.set_power(0);

        pix1.clear();
        pix1.setPixelColor(9, Adafruit_NeoPixel::Color(255, 255, 255));
        pix1.show();
        // Right sensor is on the line: turn right more gently
    } else {
        left_motor.set_power(0);
        right_motor.set_power(0);

        pix1.clear();
        pix1.setPixelColor(6, Adafruit_NeoPixel::Color(255, 255, 255));
        pix1.show();
        // Both sensors are on the line: stop or move slowly forward
    }
}


void setup(void) {
    Serial.begin(9600);
    left_motor.begin();
    right_motor.begin();
    pix1.begin();
    pix2.begin();


    // setup YL-70 Sensor Modul
    pinMode(SENSOR_LEFT, INPUT);
    pinMode(SENSOR_RIGHT, INPUT);
}

bool read_command(void* buffer, int* sz) {
    static char buf[64];
    static int size = 0;
    while(true) {
        auto c = Serial.read();
        if(c == '\n') {
            memcpy(buffer, buf, size);
            *sz = size;
            size = 0;
            return true;
        }
        if(c == -1){
            return false;
        }

        buf[size++] = c;
    }
}

void do_drive_gay(Adafruit_NeoPixel& pix, uint8_t brightness) {
    static int t = 0;

    for(int j = 0; j < NUM_PIXELS; j++) {
        pix.setPixelColor(
            j,
            Adafruit_NeoPixel::ColorHSV( t + j * 65535 / 12, 255, brightness )    
        );
    }
    pix.show();

    t = (t + 100) % 65536;
}

double measure_distance() {
    digitalWrite(PIN_DISTANCE_SENSOR_TRIG, LOW);
    delayMicroseconds(2);

    digitalWrite(PIN_DISTANCE_SENSOR_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_DISTANCE_SENSOR_TRIG, LOW);

    auto dur = pulseIn(PIN_DISTANCE_SENSOR_ECHO, HIGH);

    auto dist = dur * 0.034 / 2;
    return dist;
}

void do_reset(const char*, size_t) {
    emergency_stop = false;
}

void loop(void) {    
    char buffer[64];
    int size = 0;
    while(read_command(buffer, &size)) {
        if(size == 0) return;
        switch (buffer[0])
        {
        case 'M': do_serial_drive(buffer, size); break;
        case 'C': do_serial_color(buffer, size); break;
        case 'S': do_mode_switch(buffer, size); break;
        case 'R': do_reset(buffer, size); break;
        //case 'L': {auto readings = readLineSensors(); do_followLine(readings); break;}
        default: break;
        }
    }

    auto dist = measure_distance();

    //pix1.clear();
    //for(int i = 0; i < dist && i < NUM_PIXELS; i++) {
    //    pix1.setPixelColor(i, 
    //        emergency_stop ? Adafruit_NeoPixel::Color(50, 0, 0) :
    //            Adafruit_NeoPixel::Color(50, 50, 50));
    //}
    //pix1.show();

    pix1.clear();
    pix1.setPixelColor(5, Adafruit_NeoPixel::Color(255, 255, 255));
    pix1.setPixelColor(7, Adafruit_NeoPixel::Color(255, 255, 255));

    pix1.setPixelColor(1, Adafruit_NeoPixel::Color(255, 255, 255));
    pix1.setPixelColor(0, Adafruit_NeoPixel::Color(255, 255, 255));
    pix1.setPixelColor(11, Adafruit_NeoPixel::Color(255, 255, 255));
    pix1.setPixelColor(10, Adafruit_NeoPixel::Color(255, 255, 255));
    pix1.setPixelColor(2, Adafruit_NeoPixel::Color(255, 255, 255));

    pix1.show();

    if(dist < 10) {
        if(!emergency_stop) {
            left_motor.set_power(-255);
            right_motor.set_power(-255);

            delay(200);
        }
        emergency_stop = true;
    }

    if(emergency_stop) {
        left_motor.set_power(0);
        right_motor.set_power(0);
        return;
    }

    switch(mode){
    case MANUAL: {
        left_motor.set_power(255 * motor_power.left);
        right_motor.set_power(255 * motor_power.right);
        //do_drive_gay(pix1, 70);
        //do_drive_gay(pix2, 255);
    } break;
    case FOLLOWLINE: {
        do_followLine(readLineSensors());
    } break;
    };

}