// Copyright 2019 Marc-Antoine Ruel. All rights reserved.
// Use of this source code is governed under the Apache License, Version 2.0
// that can be found in the LICENSE file.

// Left:
// - TX (GPIO1) Idles High
// - RX (GPIO3) Idles High           ; Button LED (Doesn't work when logging is enabled)
// - D1 (GPIO5) Idles High           ; Button Monitor Up
// - D2 (GPIO4) Idles High           ; Actuator Monitor Down
// - D3 (GPIO0) Idles High           ; Button Monitor Down
// - D4 (GPIO2) LED Output           ; Low when LED on
// - GND
// - 5V

// Right:
// - RST button
// - A0 void
// - D0 (GPIO16) Idles Float (or Low); Button Seat Down
// - D5 (GPIO14) Idles High          ; Actuator Seat Up
// - D6 (GPIO12) Idles High          ; Actuator Seat Down
// - D7 (GPIO13) Idles High          ; Actuator Monitor Up
// - D8 (GPIO15) Idles Low           ; Button Seat Up
// - 3v3

// Pad layout
//
// DIN-8P DS-8-101
// The official pin order is 61425378 starting top and going clockwise.
// Order looking at the female connector facing top
// 4 Blue  Seat Up
// 6 Green Seat Down
// 7 Gray  Monitor Up
// 8 Brown Monitor Down
// GND Black

#include <Arduino.h>
#include <Homie.h>
#include <ESPAsyncWebServer.h>

#include "actuator.h"
#include "nodes.h"

// Enable to start the web server. Uses more memory.
#define USE_WEB_SERVER 1
// Do not use the LED for now.
#define USE_LED 0
#if defined(LOG_SERIAL)
#define USE_LED 0
#endif

const int BUTTON_SEAT_UP = D8;        // GPIO15; Idles Low
const int BUTTON_SEAT_DOWN = D0;      // GPIO16; Idles Low with INPUT_PULLDOWN_16 (Pulse to wake up)
const int BUTTON_MONITOR_UP = D1;     // GPIO5 ; Idles High
const int BUTTON_MONITOR_DOWN = D3;   // GPIO0 ; Pull Up (Boot mode)
const int BUTTON_LED = RX;            // GPIO3 ; Idles High (UART)
const int ACTUATOR_SEAT_UP = D5;      // GPIO14; Idles High
const int ACTUATOR_SEAT_DOWN = D6;    // GPIO12; Idles High
const int ACTUATOR_MONITOR_UP = D7;   // GPIO13; Idles High
const int ACTUATOR_MONITOR_DOWN = D2; // GPIO4 ; Idles High
const int LED_OUT = D4;               // GPIO2 ; Pull Up; Also onboard LED


//
// Homie nodes accessible through MQTT.
//


// Outputs.
ActuatorNode Seat("seat", ACTUATOR_SEAT_UP, true, ACTUATOR_SEAT_DOWN, true, 10000, 10000);
// The delay will have to be adjusted based on the monitor weight.
ActuatorNode Monitors("monitors", ACTUATOR_MONITOR_UP, true, ACTUATOR_MONITOR_DOWN, true, 5300, 3900);
#if USE_LED
PinOutNode LED("led", LED_OUT, true, NULL);
#endif

// Inputs. All of them idles High, so they are active when Low.
const int period = 50;
// We want the monitors buttons to be sticky. We want the monitors to go all the
// way up or all the way down on press, ignoring when they are released.
PinInNode buttonMonitorUp(
    "button_monitor_up",
    [](bool v) { if (v) { Monitors.set(Actuator::UP); } },
    BUTTON_MONITOR_UP,
    true,
    period);
PinInNode buttonMonitorDown(
    "button_monitor_down",
    [](bool v) { if (v) { Monitors.set(Actuator::DOWN); } },
    BUTTON_MONITOR_DOWN,
    true,
    period);
// We want the seat buttons to only take action while they are pressed. That
// would not make sense to only go all the way up or down.
PinInNode buttonSeatUp(
    "button_seat_up",
    [](bool v) {
      if (v) {
        Seat.set(Actuator::UP);
      } else {
        Seat.set(Actuator::STOP);
      }
    },
    BUTTON_SEAT_UP,
    true,
    period);
PinInNode buttonSeatDown(
    "button_seat_down",
    [](bool v) {
      if (v) {
        Seat.set(Actuator::DOWN);
      } else {
        Seat.set(Actuator::STOP);
      }
    },
    BUTTON_SEAT_DOWN,
    true,
    period);
#if USE_LED
// This pin is UART, so it cannot be used when Serial is used.
PinInNode buttonLED(
    "button_led",
    [](bool v) {
      LED.set(v);
    },
    BUTTON_LED,
    true,
    period);
#endif

#if defined(USE_WEB_SERVER)
// Web server to serve our custom MQTT web UI. This is NOT the web server when
// in configuration mode.
// TODO(maruel): There's no way to update the files over OTA at the moment, this
// requires using flash_all.sh.
AsyncWebServer httpSrv(80);
#endif

void onHomieEvent(const HomieEvent& event) {
	// See
  // https://homieiot.github.io/homie-esp8266/docs/3.0.0/advanced-usage/events/
  switch(event.type) {
    case HomieEventType::OTA_STARTED:
    case HomieEventType::ABOUT_TO_RESET:
      // Make sure to stop the actuators on OTA.
      Seat.set(Actuator::STOP);
      Monitors.set(Actuator::STOP);
      break;
    case HomieEventType::MQTT_READY:
      // Broadcast the state of every node.
      Monitors.init();
      buttonMonitorUp.init();
      buttonMonitorDown.init();
      Seat.init();
      buttonSeatUp.init();
      buttonSeatDown.init();
#if USE_LED
      LED.init();
      buttonLED.init();
      // Reset the actual LEDs.
      LED.set(buttonLED.get());
#endif
      break;
    default:
      break;
  }
}

void setup() {
#if defined(LOG_SERIAL)
  Serial.begin(115200);
  // Increase debug output to maximum level:
  Serial.setDebugOutput(true);
#else
  // Do not initialize the serial port. Remove all debug output:
  Serial.setDebugOutput(false);
  Homie.disableLogging();
#endif

  // TODO(maruel): Disable and emulate the feedback ourselves so we can reset it
  // when it makes sense.
  //Homie.disableLedFeedback();

  // There's no pin available.
  Homie.disableResetTrigger();
  // Setup. The GIT_REV variable is defined in platform.ini
  Homie_setFirmware("emperor", GIT_REV);
  Homie_setBrand("Emperor");

#if defined(LOG_SERIAL)
  Serial.println();
  Serial.println("Version: " GIT_REV);
#endif
  Homie.onEvent(onHomieEvent);
  Homie.setup();

#if defined(USE_WEB_SERVER)
  if (Homie.isConfigured()) {
    httpSrv.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String url("/index.html?device=");
        const HomieInternals::ConfigStruct& cfg = Homie.getConfiguration();
        url += urlencode(cfg.deviceId);
        url += "&host=";
        url += urlencode(cfg.mqtt.server.host);
        // For now, assume the websocket port number is the normal TCP socket
        // +1.
        url += "&port=";
        url += (cfg.mqtt.server.port+1);
        if (cfg.mqtt.auth) {
          url += "&user=";
          url += urlencode(cfg.mqtt.username);
          url += "&password=";
          url += urlencode(cfg.mqtt.password);
        }
        request->redirect(url);
    });
    httpSrv.serveStatic("/", SPIFFS, "/html/").setCacheControl("public; max-age=600");
    httpSrv.begin();
  }
#endif

  // Make sure only one of the actuator runs at a time.
  Seat.link(Monitors);
}

// Only run the loop functions at most once per millisecond.
unsigned long lastLoop;

void loop() {
  unsigned long time = millis();
  if (time != lastLoop) {
    lastLoop = time;
    buttonMonitorUp.update();
    buttonMonitorDown.update();
    Monitors.update();
    buttonSeatUp.update();
    buttonSeatDown.update();
    Seat.update();
#if USE_LED
    buttonLED.update();
    LED.update();
#endif
    Homie.loop();
  }
}
