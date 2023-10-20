// https://github.com/plapointe6/EspMQTTClient
#include <EspMQTTClient.h>
// https://github.com/FastLED/FastLED
#include <FastLED.h>
// https://github.com/contrem/arduino-timer
#include <arduino-timer.h>
// https://arduinojson.org/v6/how-to/use-arduinojson-with-pubsubclient/
#include <ArduinoJson.h>
#include "FastLED_additions.h"

#define NUM_LEDS 60
#define DATA_PIN 4
CRGB leds[NUM_LEDS];

int currentSecond = 0;
int currentMinute = 0;
int currentHour = 0;
int remainingTime = 0;
uintptr_t countdown;
const uint16_t secondColor = 200;
const uint16_t minuteColor = 100;
const uint16_t hourColor = 5;
auto timer = timer_create_default();

bool hideAnimations = true;

EspMQTTClient client(
  "Dumas-2-4",
  "6gKE4@NbDMv.",
  "192.168.0.200",    // "192.168.12.189",  // MQTT Broker server ip
  "mqtt",          // Can be omitted if not needed
  "mqtt",  // "lkjEkljaiOO89421lkj",   // Can be omitted if not needed
  "ClockTimer",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

void onConnectionEstablished() {
  

  // number of seconds to set on the clock
  client.subscribe("clock/setTimer", [](const String &payload) {
    clearLights();
    if (payload == "complete") {
      complete();
    } else if (payload == "cancelled") {
      reset();
      FastLED.show();
    } else {
      StaticJsonDocument<256> doc;
      deserializeJson(doc, payload);
      const int seconds = doc["totalSeconds"];
      Serial.println(seconds);
      start(seconds);
    }
  });
}

void clearLights() {
  if (countdown != NULL) {
    timer.cancel(countdown);
    countdown = NULL;
  }
  hideAnimations = true;
  reset();
}

void setLights(uint16_t light, uint16_t color, bool all) {
  if (light < 0) {
    return;
  }

  if (all) {
    for (uint16_t i = 0; i < light; i++) {
      leds[i] = CHSV(color, 255, 255);
    }
  } else {
    leds[light] = CHSV(color, 255, 255);
  }
}

void setLeds() {
  reset();

  setLights(currentMinute, minuteColor, true);
  setLights(currentHour, hourColor, true);
  if (currentHour > currentMinute) {
    setLights(currentMinute, minuteColor, false);
  }
  setLights(currentSecond, secondColor, remainingTime < 60);
  FastLED.show();
}

void reset() {
  for (uint16_t i; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
}

void start(int timerSeconds) {
  remainingTime = timerSeconds;
  setCurrentTimes(timerSeconds);
  FastLED.setBrightness(255);
  setLeds();
  delay(30);
  countdown = timer.every(1000, handleTick);
}

void setCurrentTimes(int timerSeconds) {
// 1 min = 60 sec
// 1 hour = 60 min  
  currentSecond = timerSeconds % 60;
  currentMinute = timerSeconds / 60;
  currentHour = currentMinute / 60;
  currentMinute = currentMinute % 60;
}

bool handleTick(void *) {
  remainingTime -= 1;
  setCurrentTimes(remainingTime);
  setLeds();
  if (remainingTime == 0) {
    complete();
  }
  return remainingTime != 0;
}
void complete() {
  timer.cancel(countdown);
  reset();
  Serial.println("animate");
  runAnimation();
}

bool checkIn(void *) {
  client.publish("clock/status", "OK");
  return true;
}


void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  client.enableDebuggingMessages();                                           // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater();                                              // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA();                                                         // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true

  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);

  timer.every(36000, checkIn);
}

void loop() {
  timer.tick();
  client.loop();
  if (hideAnimations == false) {
    animationCycle();
  }
}

int animationRuntime;
void runAnimation() {
  animationRuntime = 0;
  hideAnimations = false;
  Serial.println("run animation loop");
}
void animationCycle() {
  animationRuntime += 1;
  if (animationRuntime >= 5000) {
    hideAnimations = true;
    fadeOut();
    reset();
    FastLED.show();
  } else {
    colorWheel();
  }
}


////////////////
// ANIMATIONS //
////////////////
void fadeOut() {
  for (uint8_t i = 255; i > 0; i--) {
    FastLED.setBrightness(i);
    FastLED.show();
    delay(10);
  }
}

CRGBPalette16 currentPalette = RainbowHalfStripeColors_p;
void colorWheel() {
  static uint16_t startIndex = 0, phase = 0;
  startIndex -= 120; /* rotation speed */
  phase += 64;       /* swirl speed */
  CRGB *led = leds;
  uint16_t ringIndex = startIndex + sin16(phase * 1024);
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint16_t hue = ringIndex + i * 65535 / NUM_LEDS;
    *led++ = ColorFromPaletteExtended(currentPalette, hue, 255, LINEARBLEND);
  }
  FastLED.show();
}