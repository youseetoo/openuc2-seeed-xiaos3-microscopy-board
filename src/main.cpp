/*
 * XIAO ESP32S3 Microscopy Board -- Webserver Controller
 * Features: Captive Portal, openUC2 brand UI, touch LED toggle
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "pins.h"
#include "website.h"
#include "image.h"

#define STA_MODE      false
#define WIFI_SSID     "YourSSID"
#define WIFI_PASSWORD "YourPassword"
#define AP_SSID       "openUC2-Microscope"
#define AP_PASSWORD   ""

DNSServer dnsServer;
static const byte DNS_PORT = 53;

WebServer server(80);
Adafruit_NeoPixel strip(NEOPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

int     motor1Speed = 0, motor2Speed = 0;
int     servo1Angle = 90, servo2Angle = 90;
uint8_t npR = 0, npG = 50, npB = 0, npBrightness = 50;
bool    ledOn = false;

static bool     prevT1 = false, prevT2 = false;
static uint32_t lastT1 = 0, lastT2 = 0;
static const uint32_t DEBOUNCE_MS = 300;

void setMotor(int chA, int chB, int spd) {
    spd = constrain(spd, -255, 255);
    if (spd >= 0) { ledcWrite(chA, spd); ledcWrite(chB, 0); }
    else          { ledcWrite(chA, 0);   ledcWrite(chB, -spd); }
}

void setServo(int channel, int angle) {
    angle = constrain(angle, 0, 180);
    long periodUs = 1000000L / SERVO_FREQ;
    long pulseUs  = map(angle, 0, 180, SERVO_US_MIN, SERVO_US_MAX);
    long duty     = (pulseUs * ((1 << SERVO_RES) - 1)) / periodUs;
    ledcWrite(channel, (uint32_t)duty);
}

void updateNeopixel() {
    strip.setBrightness(npBrightness);
    strip.setPixelColor(0, strip.Color(npR, npG, npB));
    strip.show();
}

void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

void handleLogo() {
    server.sendHeader("Cache-Control", "max-age=86400");
    server.send_P(200, "image/png", (const char*)openuc2_logo_png, openuc2_logo_png_len);
}

void handleNeopixel() {
    npR          = (uint8_t)server.arg("r").toInt();
    npG          = (uint8_t)server.arg("g").toInt();
    npB          = (uint8_t)server.arg("b").toInt();
    npBrightness = (uint8_t)server.arg("brightness").toInt();
    updateNeopixel();
    server.send(200, "text/plain",
        "R=" + String(npR) + " G=" + String(npG) +
        " B=" + String(npB) + " BR=" + String(npBrightness));
}

void handleMotor() {
    int n = server.arg("n").toInt();
    int s = server.arg("speed").toInt();
    if (n == 1) { motor1Speed = s; setMotor(MOTOR_1_CH_A, MOTOR_1_CH_B, s); }
    else if (n == 2) { motor2Speed = s; setMotor(MOTOR_2_CH_A, MOTOR_2_CH_B, s); }
    server.send(200, "text/plain", "Motor " + String(n) + " spd=" + String(s));
}

void handleServo() {
    int n = server.arg("n").toInt();
    int a = server.arg("angle").toInt();
    if (n == 1) { servo1Angle = a; setServo(SERVO_CH_1, a); }
    if (n == 2) { servo2Angle = a; setServo(SERVO_CH_2, a); }
    server.send(200, "text/plain", "Servo " + String(n) + " angle=" + String(a));
}

void handleStatus() {
    bool t1 = touchRead(TOUCH_PIN_1) > TOUCH_THRESHOLD;
    bool t2 = touchRead(TOUCH_PIN_2) > TOUCH_THRESHOLD;
    String ip = (WiFi.getMode() == WIFI_AP) ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    server.send(200, "application/json",
        "{\"touch1\":" + String(t1 ? "true" : "false") +
        ",\"touch2\":" + String(t2 ? "true" : "false") +
        ",\"ledOn\":"  + String(ledOn ? "true" : "false") +
        ",\"motor1\":" + String(motor1Speed) +
        ",\"motor2\":" + String(motor2Speed) +
        ",\"servo1\":" + String(servo1Angle) +
        ",\"servo2\":" + String(servo2Angle) +
        ",\"ip\":\""   + ip + "\"}");
}

void handleI2CScan() {
    String json = "{\"devices\":[";
    bool first = true;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            if (!first) json += ",";
            json += String(addr);
            first = false;
        }
    }
    server.send(200, "application/json", json + "]}");
}

void handleCaptive() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
}

void checkTouchToggle() {
    uint32_t now = millis();
    bool cur1 = touchRead(TOUCH_PIN_1) > TOUCH_THRESHOLD;
    bool cur2 = touchRead(TOUCH_PIN_2) > TOUCH_THRESHOLD;
    if (cur1 && !prevT1 && (now - lastT1 > DEBOUNCE_MS)) {
        ledOn = true;
        npR = 255; npG = 255; npB = 255; npBrightness = 200;
        updateNeopixel();
        lastT1 = now;
    }
    if (cur2 && !prevT2 && (now - lastT2 > DEBOUNCE_MS)) {
        ledOn = false;
        npR = 0; npG = 0; npB = 0; npBrightness = 0;
        updateNeopixel();
        lastT2 = now;
    }
    prevT1 = cur1;
    prevT2 = cur2;
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== openUC2 XIAO ESP32S3 Boot ===");
    strip.begin();
    strip.setBrightness(30);
    strip.setPixelColor(0, strip.Color(255, 255, 255));
    strip.show();
    ledcSetup(MOTOR_1_CH_A, MOTOR_FREQ, MOTOR_RES); ledcAttachPin(MOTOR_1_IN1, MOTOR_1_CH_A);
    ledcSetup(MOTOR_1_CH_B, MOTOR_FREQ, MOTOR_RES); ledcAttachPin(MOTOR_1_IN2, MOTOR_1_CH_B);
    ledcSetup(MOTOR_2_CH_A, MOTOR_FREQ, MOTOR_RES); ledcAttachPin(MOTOR_2_IN1, MOTOR_2_CH_A);
    ledcSetup(MOTOR_2_CH_B, MOTOR_FREQ, MOTOR_RES); ledcAttachPin(MOTOR_2_IN2, MOTOR_2_CH_B);
    ledcSetup(SERVO_CH_1, SERVO_FREQ, SERVO_RES); ledcAttachPin(SERVO_PIN_1, SERVO_CH_1);
    ledcSetup(SERVO_CH_2, SERVO_FREQ, SERVO_RES); ledcAttachPin(SERVO_PIN_2, SERVO_CH_2);
    setServo(SERVO_CH_1, 90);
    setServo(SERVO_CH_2, 90);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
#if STA_MODE
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries++ < 20) { delay(500); Serial.print('.'); }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nSTA failed, AP fallback");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID);
    } else {
        Serial.println("\nSTA IP: " + WiFi.localIP().toString());
    }
#else
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);
    Serial.println("AP: " + String(AP_SSID) + "  IP: " + WiFi.softAPIP().toString());
#endif
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    server.on("/",                    HTTP_GET, handleRoot);
    server.on("/logo",                HTTP_GET, handleLogo);
    server.on("/neopixel",            HTTP_GET, handleNeopixel);
    server.on("/motor",               HTTP_GET, handleMotor);
    server.on("/servo",               HTTP_GET, handleServo);
    server.on("/status",              HTTP_GET, handleStatus);
    server.on("/i2c",                 HTTP_GET, handleI2CScan);
    server.on("/generate_204",        HTTP_GET, handleCaptive);
    server.on("/gen_204",             HTTP_GET, handleCaptive);
    server.on("/hotspot-detect.html", HTTP_GET, handleRoot);
    server.on("/connecttest.txt",     HTTP_GET, handleRoot);
    server.on("/ncsi.txt",            HTTP_GET, handleRoot);
    server.onNotFound(handleCaptive);
    server.begin();
    Serial.println("HTTP server started");
    strip.setPixelColor(0, strip.Color(255, 255, 255));
    strip.show();
}

void loop() {
    dnsServer.processNextRequest();
    server.handleClient();
    checkTouchToggle();
    delay(2);
}
