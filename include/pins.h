#pragma once

// ============================================================
// Pin mapping for XIAO ESP32S3 Robot Board
// Verified against PCB pin table (D-label → GPIO)
// ============================================================
//
//  D-Label | GPIO  | PCB Component        | Function
//  --------|-------|----------------------|------------------
//  D0      | GPIO1 | Touch Button 1       | Touch input 1
//  D1      | GPIO2 | Touch Button 2       | Touch input 2
//  D2      | GPIO3 | Motor Driver 2 (IN2) | Motor 2 control
//  D3      | GPIO4 | Motor Driver 1 (IN1) | Motor 1 control
//  D4      | GPIO5 | Motor Driver 1 (IN2) | Motor 1 control
//  D5      | GPIO6 | I2C SCL              | I2C clock
//  D6      | GPIO7 | Neopixel             | RGB LED data
//  D7      | GPIO8 | Servo Connector 1    | Servo 1 PWM
//  D8      | GPIO9 | Servo Connector 2    | Servo 2 PWM
//  D9      | GPIO10| Motor Driver 2 (IN1) | Motor 2 control
//  D10     | GPIO11| I2C SDA              | I2C data

// Touch buttons
#define TOUCH_PIN_1     D0   // GPIO1
#define TOUCH_PIN_2     D1   // GPIO2

// Motor Driver (H-bridge, e.g. DRV8833 / TB6612)
#define MOTOR_1_IN1     D3   // GPIO4
#define MOTOR_1_IN2     D4   // GPIO5
#define MOTOR_2_IN1     D9   // GPIO10
#define MOTOR_2_IN2     D2   // GPIO3

// I2C
#define I2C_SDA_PIN     D10  // GPIO11
#define I2C_SCL_PIN     D5   // GPIO6

// Neopixel
#define NEOPIXEL_PIN    D6   // GPIO7
#define NEOPIXELS       1

// Servos
#define SERVO_PIN_1     D7   // GPIO8
#define SERVO_PIN_2     D8   // GPIO9

// PWM channels (ESP32 LEDC)
#define SERVO_CH_1      0
#define SERVO_CH_2      1
#define MOTOR_1_CH_A    2
#define MOTOR_1_CH_B    3
#define MOTOR_2_CH_A    4
#define MOTOR_2_CH_B    5

#define SERVO_FREQ      50
#define SERVO_RES       16
#define SERVO_US_MIN    500
#define SERVO_US_MAX    2500

#define MOTOR_FREQ      1000
#define MOTOR_RES       8    // 0-255

// Touch thresholds (tune per hardware)
#define TOUCH_THRESHOLD 40000  // ESP32S3 uses capacitive; higher = touched
