/*
 * XIAO ESP32S3 Robot Board — Webserver Controller
 * ------------------------------------------------
 * Features:
 *   - WiFi Access Point (or STA mode if credentials provided)
 *   - Web UI to control:
 *       • NeoPixel colour & brightness
 *       • Motor 1 & 2 speed/direction via PWM
 *       • Servo 1 & 2 angle
 *   - Touch pad 1/2 state reported via /status JSON
 *   - I2C scan on /i2c endpoint
 *
 * NOTE: ESP32S3 touchRead() returns CAPACITANCE values (higher when touched),
 *       opposite to the older ESP32. Threshold in pins.h must be tuned.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "pins.h"

// ── WiFi credentials ──────────────────────────────────────────────────────────
// Set AP mode: board creates its own hotspot named "XiaoRobot"
// To connect to an existing network instead, set STA_MODE true and fill creds.
#define STA_MODE      false
#define WIFI_SSID     "YourSSID"
#define WIFI_PASSWORD "YourPassword"
#define AP_SSID       "openUC2-Microscope"
#define AP_PASSWORD   ""   // min 8 chars

// ── Globals ───────────────────────────────────────────────────────────────────
WebServer server(80);
Adafruit_NeoPixel strip(NEOPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Motor state  (-255 … +255, where sign = direction, magnitude = speed)
int motor1Speed = 0;
int motor2Speed = 0;

// Servo angles
int servo1Angle = 90;
int servo2Angle = 90;

// Neopixel state
uint8_t npR = 0, npG = 50, npB = 0;
uint8_t npBrightness = 50;

// ── Helper: Write motor PWM ───────────────────────────────────────────────────
// speed: -255 … +255
void setMotor(int chA, int chB, int speed) {
    speed = constrain(speed, -255, 255);
    if (speed >= 0) {
        ledcWrite(chA, speed);
        ledcWrite(chB, 0);
    } else {
        ledcWrite(chA, 0);
        ledcWrite(chB, -speed);
    }
}

// ── Helper: Write servo angle ─────────────────────────────────────────────────
void setServo(int channel, int angle) {
    angle = constrain(angle, 0, 180);
    long periodUs = 1000000L / SERVO_FREQ;
    long pulseUs  = map(angle, 0, 180, SERVO_US_MIN, SERVO_US_MAX);
    long duty     = (pulseUs * ((1 << SERVO_RES) - 1)) / periodUs;
    ledcWrite(channel, (uint32_t)duty);
}

// ── Helper: Update NeoPixel ───────────────────────────────────────────────────
void updateNeopixel() {
    strip.setBrightness(npBrightness);
    strip.setPixelColor(0, strip.Color(npR, npG, npB));
    strip.show();
}

// ── Web UI HTML (served from RAM) ─────────────────────────────────────────────
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>XIAO Robot</title>
<style>
  :root {
    --bg: #0d0d0d; --surface: #161616; --border: #2a2a2a;
    --accent: #39ff14; --accent2: #ff3c78; --text: #e8e8e8; --muted: #666;
    --radius: 8px;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { background: var(--bg); color: var(--text); font-family: 'Courier New', monospace;
         min-height: 100vh; padding: 20px; }
  h1 { color: var(--accent); letter-spacing: 4px; text-transform: uppercase;
       font-size: 1.1rem; margin-bottom: 4px; }
  .subtitle { color: var(--muted); font-size: .75rem; margin-bottom: 24px; }
  .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 16px; }
  .card { background: var(--surface); border: 1px solid var(--border); border-radius: var(--radius); padding: 18px; }
  .card h2 { font-size: .8rem; letter-spacing: 2px; text-transform: uppercase;
              color: var(--accent); margin-bottom: 14px; border-bottom: 1px solid var(--border); padding-bottom: 8px; }
  label { font-size: .75rem; color: var(--muted); display: block; margin-bottom: 4px; margin-top: 10px; }
  input[type=range] { width: 100%; accent-color: var(--accent); height: 4px; }
  input[type=color] { width: 100%; height: 40px; border: 1px solid var(--border); border-radius: 4px;
                       background: none; cursor: pointer; }
  .val { float: right; color: var(--accent); font-size: .75rem; }
  button { margin-top: 14px; width: 100%; padding: 10px; background: transparent;
           border: 1px solid var(--accent); color: var(--accent); border-radius: var(--radius);
           cursor: pointer; font-family: inherit; font-size: .8rem; letter-spacing: 2px;
           text-transform: uppercase; transition: all .2s; }
  button:hover { background: var(--accent); color: #000; }
  button.stop { border-color: var(--accent2); color: var(--accent2); }
  button.stop:hover { background: var(--accent2); color: #000; }
  .status-bar { display: flex; gap: 16px; margin-bottom: 20px; flex-wrap: wrap; }
  .badge { background: var(--surface); border: 1px solid var(--border); border-radius: 4px;
           padding: 6px 12px; font-size: .72rem; }
  .badge span { color: var(--accent); }
  #log { color: var(--muted); font-size: .7rem; margin-top: 20px; height: 60px;
         overflow-y: auto; border-top: 1px solid var(--border); padding-top: 10px; }
</style>
</head>
<body>
<h1>&#9632; XIAO Robot Controller</h1>
<p class="subtitle">ESP32S3 // WebServer UI</p>

<div class="status-bar">
  <div class="badge">T1: <span id="t1">-</span></div>
  <div class="badge">T2: <span id="t2">-</span></div>
  <div class="badge">IP: <span id="ip">-</span></div>
</div>

<div class="grid">

  <!-- NeoPixel -->
  <div class="card">
    <h2>&#9632; NeoPixel</h2>
    <label>Colour</label>
    <input type="color" id="npColor" value="#003200">
    <label>Brightness <span class="val" id="brightVal">50</span></label>
    <input type="range" id="brightness" min="0" max="255" value="50"
           oninput="document.getElementById('brightVal').textContent=this.value">
    <button onclick="sendNeopixel()">Apply</button>
    <button class="stop" onclick="sendNeopixelOff()">Off</button>
  </div>

  <!-- Motor 1 -->
  <div class="card">
    <h2>&#9632; Motor 1</h2>
    <label>Speed / Direction <span class="val" id="m1val">0</span></label>
    <input type="range" id="motor1" min="-255" max="255" value="0"
           oninput="document.getElementById('m1val').textContent=this.value">
    <button onclick="sendMotor(1)">Set</button>
    <button class="stop" onclick="stopMotor(1)">Stop</button>
  </div>

  <!-- Motor 2 -->
  <div class="card">
    <h2>&#9632; Motor 2</h2>
    <label>Speed / Direction <span class="val" id="m2val">0</span></label>
    <input type="range" id="motor2" min="-255" max="255" value="0"
           oninput="document.getElementById('m2val').textContent=this.value">
    <button onclick="sendMotor(2)">Set</button>
    <button class="stop" onclick="stopMotor(2)">Stop</button>
  </div>

  <!-- Servo 1 -->
  <div class="card">
    <h2>&#9632; Servo 1</h2>
    <label>Angle <span class="val" id="s1val">90</span>°</label>
    <input type="range" id="servo1" min="0" max="180" value="90"
           oninput="document.getElementById('s1val').textContent=this.value">
    <button onclick="sendServo(1)">Set</button>
  </div>

  <!-- Servo 2 -->
  <div class="card">
    <h2>&#9632; Servo 2</h2>
    <label>Angle <span class="val" id="s2val">90</span>°</label>
    <input type="range" id="servo2" min="0" max="180" value="90"
           oninput="document.getElementById('s2val').textContent=this.value">
    <button onclick="sendServo(2)">Set</button>
  </div>

  <!-- I2C scan -->
  <div class="card">
    <h2>&#9632; I2C Scanner</h2>
    <div id="i2cResult" style="font-size:.75rem;color:var(--muted);min-height:40px">Press scan…</div>
    <button onclick="scanI2C()">Scan</button>
  </div>

</div>

<div id="log"></div>

<script>
function log(msg) {
  const el = document.getElementById('log');
  el.innerHTML = '[' + new Date().toLocaleTimeString() + '] ' + msg + '<br>' + el.innerHTML;
}
function hexToRgb(hex) {
  const r = parseInt(hex.slice(1,3),16), g = parseInt(hex.slice(3,5),16), b = parseInt(hex.slice(5,7),16);
  return {r,g,b};
}
function sendNeopixel() {
  const c = hexToRgb(document.getElementById('npColor').value);
  const br = document.getElementById('brightness').value;
  fetch(`/neopixel?r=${c.r}&g=${c.g}&b=${c.b}&brightness=${br}`)
    .then(r=>r.text()).then(t=>log('NeoPixel: '+t));
}
function sendNeopixelOff() {
  fetch('/neopixel?r=0&g=0&b=0&brightness=0').then(r=>r.text()).then(t=>log('NeoPixel off'));
}
function sendMotor(n) {
  const speed = document.getElementById('motor'+n).value;
  fetch(`/motor?n=${n}&speed=${speed}`).then(r=>r.text()).then(t=>log('Motor '+n+': '+t));
}
function stopMotor(n) {
  document.getElementById('motor'+n).value = 0;
  document.getElementById('m'+n+'val').textContent = '0';
  fetch(`/motor?n=${n}&speed=0`).then(r=>r.text()).then(t=>log('Motor '+n+' stopped'));
}
function sendServo(n) {
  const angle = document.getElementById('servo'+n).value;
  fetch(`/servo?n=${n}&angle=${angle}`).then(r=>r.text()).then(t=>log('Servo '+n+': '+t));
}
function scanI2C() {
  fetch('/i2c').then(r=>r.json()).then(data => {
    const el = document.getElementById('i2cResult');
    el.innerHTML = data.devices.length
      ? data.devices.map(a=>`<span style="color:var(--accent)">0x${a.toString(16).padStart(2,'0').toUpperCase()}</span>`).join(', ')
      : '<span style="color:var(--accent2)">No devices found</span>';
    log('I2C scan done: ' + data.devices.length + ' device(s)');
  });
}
function pollStatus() {
  fetch('/status').then(r=>r.json()).then(d => {
    document.getElementById('t1').textContent = d.touch1 ? '●ACTIVE' : '○idle';
    document.getElementById('t1').style.color = d.touch1 ? 'var(--accent)' : 'var(--muted)';
    document.getElementById('t2').textContent = d.touch2 ? '●ACTIVE' : '○idle';
    document.getElementById('t2').style.color = d.touch2 ? 'var(--accent)' : 'var(--muted)';
    document.getElementById('ip').textContent = d.ip;
  }).catch(()=>{});
}
setInterval(pollStatus, 1000);
pollStatus();
</script>
</body>
</html>
)rawliteral";

// ── Route handlers ────────────────────────────────────────────────────────────

void handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void handleNeopixel() {
    npR          = (uint8_t)server.arg("r").toInt();
    npG          = (uint8_t)server.arg("g").toInt();
    npB          = (uint8_t)server.arg("b").toInt();
    npBrightness = (uint8_t)server.arg("brightness").toInt();
    updateNeopixel();
    server.send(200, "text/plain",
        "R=" + String(npR) + " G=" + String(npG) + " B=" + String(npB) +
        " BR=" + String(npBrightness));
}

void handleMotor() {
    int n     = server.arg("n").toInt();
    int speed = server.arg("speed").toInt();
    if (n == 1) {
        motor1Speed = speed;
        setMotor(MOTOR_1_CH_A, MOTOR_1_CH_B, speed);
    } else if (n == 2) {
        motor2Speed = speed;
        setMotor(MOTOR_2_CH_A, MOTOR_2_CH_B, speed);
    }
    server.send(200, "text/plain", "Motor " + String(n) + " speed=" + String(speed));
}

void handleServo() {
    int n     = server.arg("n").toInt();
    int angle = server.arg("angle").toInt();
    if (n == 1) { servo1Angle = angle; setServo(SERVO_CH_1, angle); }
    if (n == 2) { servo2Angle = angle; setServo(SERVO_CH_2, angle); }
    server.send(200, "text/plain", "Servo " + String(n) + " angle=" + String(angle));
}

void handleStatus() {
    // ESP32S3 touch: touchRead returns capacitance; touched = value > threshold
    bool t1 = touchRead(TOUCH_PIN_1) > TOUCH_THRESHOLD;
    bool t2 = touchRead(TOUCH_PIN_2) > TOUCH_THRESHOLD;
    String ip = WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
    String json = "{\"touch1\":" + String(t1 ? "true" : "false") +
                  ",\"touch2\":" + String(t2 ? "true" : "false") +
                  ",\"motor1\":" + String(motor1Speed) +
                  ",\"motor2\":" + String(motor2Speed) +
                  ",\"servo1\":" + String(servo1Angle) +
                  ",\"servo2\":" + String(servo2Angle) +
                  ",\"ip\":\"" + ip + "\"}";
    server.send(200, "application/json", json);
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
    json += "]}";
    server.send(200, "application/json", json);
}

void handleNotFound() {
    server.send(404, "text/plain", "Not found");
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== XIAO ESP32S3 Robot Boot ===");

    // ── NeoPixel
    strip.begin();
    strip.setBrightness(npBrightness);
    strip.setPixelColor(0, strip.Color(255, 255, 255));  // dim green = booting
    strip.show();

    // ── Motor PWM channels
    ledcSetup(MOTOR_1_CH_A, MOTOR_FREQ, MOTOR_RES);
    ledcAttachPin(MOTOR_1_IN1, MOTOR_1_CH_A);
    ledcSetup(MOTOR_1_CH_B, MOTOR_FREQ, MOTOR_RES);
    ledcAttachPin(MOTOR_1_IN2, MOTOR_1_CH_B);
    ledcSetup(MOTOR_2_CH_A, MOTOR_FREQ, MOTOR_RES);
    ledcAttachPin(MOTOR_2_IN1, MOTOR_2_CH_A);
    ledcSetup(MOTOR_2_CH_B, MOTOR_FREQ, MOTOR_RES);
    ledcAttachPin(MOTOR_2_IN2, MOTOR_2_CH_B);

    // ── Servo PWM channels
    ledcSetup(SERVO_CH_1, SERVO_FREQ, SERVO_RES);
    ledcAttachPin(SERVO_PIN_1, SERVO_CH_1);
    ledcSetup(SERVO_CH_2, SERVO_FREQ, SERVO_RES);
    ledcAttachPin(SERVO_PIN_2, SERVO_CH_2);
    setServo(SERVO_CH_1, 90);
    setServo(SERVO_CH_2, 90);

    // ── I2C
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // ── WiFi
#if STA_MODE
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500); Serial.print('.'); retries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\nSTA failed, falling back to AP mode");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASSWORD);
    }
#else
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);
    Serial.println("AP started: " + String(AP_SSID));
    Serial.println("IP: " + WiFi.softAPIP().toString());
#endif

    // ── Routes
    server.on("/",        HTTP_GET, handleRoot);
    server.on("/neopixel",HTTP_GET, handleNeopixel);
    server.on("/motor",   HTTP_GET, handleMotor);
    server.on("/servo",   HTTP_GET, handleServo);
    server.on("/status",  HTTP_GET, handleStatus);
    server.on("/i2c",     HTTP_GET, handleI2CScan);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");

    // Ready: blue pixel
    strip.setPixelColor(0, strip.Color(0, 0, 40));
    strip.show();
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
    server.handleClient();
    delay(2);
}
