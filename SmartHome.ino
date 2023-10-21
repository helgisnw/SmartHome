#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "HUSKYLENS.h"
#include <Servo.h>
#include <DHT.h>
#include <DS1302.h>
#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
#include <avr/power.h>
#endif
#define PIN 4
#define NUMPIXELS 2
#define DHTPIN 9
#define DHTTYPE DHT11

LiquidCrystal_I2C lcd(0x27, 16, 2);
HUSKYLENS huskylens;
Servo myservo;
DHT dht(DHTPIN, DHTTYPE);
DS1302 rtc(5, 6, 7); // RST/CE, DAT/IO, CLK/SCLK
Time t;
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

const int IR_SENSOR = 12;
const int SERVO_PIN = 13;
const int PIEZO_PIN = 11;
const int TOUCH_SENSOR = 10;
const int RELAY_PIN = 8;
const int SOUND_PIN = A0;
const int FLAME_PIN = 17;

unsigned long previousTime = 0;
unsigned long faceCheckTime;
int closeAngle = 160;
int openAngle = 70;
boolean doorState = 0;
boolean fanState = 0;
boolean ledState = 0;

byte  SpecialChar0[8] = {
  B00100,
  B01010,
  B01010,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110
};

byte  SpecialChar1[8] = {
  B00100,
  B00100,
  B01010,
  B01010,
  B10001,
  B11111,
  B11111,
  B01110
};

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  pinMode(IR_SENSOR, INPUT);
  huskylens.begin(Wire);
  myservo.attach(SERVO_PIN);
  myservo.write(closeAngle);
  pinMode(PIEZO_PIN, OUTPUT);
  pinMode(TOUCH_SENSOR, INPUT);
  lcd.createChar(0, SpecialChar0);
  lcd.createChar(1, SpecialChar1);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  rtc.halt(false);
  rtc.writeProtect(false);
  //rtc.setDOW(WEDNESDAY);
  //rtc.setTime(21, 4, 0);  // 시, 분, 초
  //rtc.setDate(01, 04, 2023); // 일, 월, 년
  pixels.begin();
  pixels.setBrightness(255);
  pixels.clear();
  pixels.show();
  pinMode(FLAME_PIN, INPUT);
  delay(500);
  myservo.detach();
}

void loop() {
  Serial.println(analogRead(SOUND_PIN));
  unsigned long currentTime = millis();
  if ((unsigned long)(currentTime - previousTime) >= 1000) {
    previousTime = currentTime;
    lcdPrint_DHT11();
    printDateTime();
  }
  int t = dht. readTemperature();
  if (t >= 30 && fanState == 0) {
    fanState = 1;
  }
  if (t <= 26 && fanState == 1) {
    fanState = 0;
  }
  if (fanState == 1) digitalWrite(RELAY_PIN, LOW);

  if (fanState == 0) digitalWrite(RELAY_PIN, HIGH);

  if (digitalRead(TOUCH_SENSOR) == 1 && doorState == 0) {
    myservo.attach(SERVO_PIN);
    myservo.write(openAngle);
    delay(500);
    myservo.detach();
    doorState = 1;
  }
  if (digitalRead(TOUCH_SENSOR) == 1 && doorState == 1) {
    myservo.attach(SERVO_PIN);
    myservo.write(closeAngle);
    delay(500);
    myservo.detach();
    doorState = 0;
  }

  if (analogRead(SOUND_PIN) > 200) {
    delay(500);
    ledState = 1 - ledState;
  }
  if (ledState == 1) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(255, 255, 255));
      pixels.show();
    }
  }
  if (ledState == 0) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.clear();
      pixels.show();
    }
  }

  if (digitalRead(FLAME_PIN) == 0) {
    digitalWrite(PIEZO_PIN, HIGH);
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(255, 0, 0));
      pixels.show();
    }
  }
  if (digitalRead(FLAME_PIN) == 1) {
    digitalWrite(PIEZO_PIN, LOW);
  }

  if (digitalRead(IR_SENSOR) == 0 && doorState == 0) {
    delay(500);
    if (digitalRead(IR_SENSOR) == 0 && doorState == 0) {
      face_check();
      faceCheckTime = millis();
      while (millis() - faceCheckTime <= 3000) {
        if (huskylens.request()) {
          if (huskylens.isLearned()) {
            if (huskylens.available()) {
              HUSKYLENSResult result = huskylens.read();
              faceCheckTime = millis();
              while (millis() - faceCheckTime <= 3000) {
                if (result.ID >= 1) {
                  check_ok();
                  myservo.attach(SERVO_PIN);
                  myservo.write(openAngle);
                  delay(1000);
                  myservo.detach();
                  doorState = 1;
                }
                else {
                  check_fail();
                  digitalWrite(PIEZO_PIN, HIGH);
                  delay(3000);
                  digitalWrite(PIEZO_PIN, LOW);
                }
              }
            }
          }
        }
      }
      lcd.clear();
    }
  }
}

void face_check() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Welcome Home!");
  lcd.setCursor(2, 1);
  lcd.print("Face Check!");
}
void check_ok() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("Face Check OK!");
  lcd.setCursor(3, 1);
  lcd.print("Door Open");
}
void check_fail() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Face Check Fail!");
  lcd.setCursor(2, 1);
  lcd.print("Door close");
}
void lcdPrint_DHT11() {
  int h = dht. readHumidity();
  int t = dht. readTemperature();
  char str_h[5];
  char str_t[5];

  lcd.setCursor(11, 0);
  lcd.write(0);
  lcd.print(" ");
  sprintf(str_t, "%2d", t);
  lcd.print(str_t);
  lcd.print("C" );

  lcd.setCursor(11, 1);
  lcd.write(1);
  lcd.print(" ");
  sprintf(str_h, "%2d", h);
  lcd.print(str_h);
  lcd.print("%" );
}
void printDateTime() {
  t = rtc.getTime();
  char str_y[20];
  sprintf(str_y, "%4d/%02d/%02d", t.year, t.mon, t.date);
  lcd.setCursor(0, 0);
  lcd.print(str_y);
  char str_h[20];
  sprintf(str_h, "%02d:%02d:%02d", t.hour, t.min, t.sec);
  lcd.setCursor(1, 1);
  lcd.print(str_h);
}