// Copyright 2019 Marc-Antoine Ruel. All rights reserved.
// Use of this source code is governed under the Apache License, Version 2.0
// that can be found in the LICENSE file.

// This file uses WeMos pin numbers but they are not special, just defines to
// the actual GPIO number.

// https://github.com/espressif/esptool/wiki/ESP8266-Boot-Mode-Selection
// https://wiki.wemos.cc/products:d1:d1_mini#documentation
//
// Interfere with boot:
// - RST -> button
// - D3 (GPIO0) HIGH normal, LOW flash via UART; 10kOhm Pull Up; On startup 26MHz for 50ms
// - TX (GPIO1) Out; On startup binary output for 20ms
// - D4 (GPIO2) Out; Pulled HIGH; Drives on board LED; 10kOhm Pull Up; On startup 600ms low with 20ms of 25kHz
// - RX (GPIO3) In
// - D8 (GPIO15) In; LOW normal; HIGH boot to SDIO; 10kOhm Pull Down; On startup 200ms at 0.7V
// - D0 (GPIO16) In; pulse signal to RST to wake up from wifi; Float; Pull Down with INPUT_PULLDOWN_16

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


#include <Arduino.h>
#include <Homie.h>
#include <ESPAsyncWebServer.h>

#include "actuator.h"
#include "nodes.h"

// Enable to start the web server. Uses more memory.
#define USE_WEB_SERVER 1

// Enable debugging. Preferably use "./do.py --log-serial" instead.
//#define LOG_SERIAL 1


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
// Stop after 10 seconds.
const int runTime = 10000;
PinOutNode LED("led", LED_OUT, true, NULL);
ActuatorNode Seat("seat", ACTUATOR_SEAT_UP, true, ACTUATOR_SEAT_DOWN, true, runTime);
ActuatorNode Monitors("monitors", ACTUATOR_MONITOR_UP, true, ACTUATOR_MONITOR_DOWN, true, runTime);


// Inputs. All of them idles High, so they are active when Low.
const int period = 50;
PinInNode buttonMonitorUp(
    "button_monitor_up",
    [](bool v) {
      if (!v) {
        Monitors.set(Actuator::STOP);
        return;
      }
      Monitors.set(Actuator::UP);
    },
    BUTTON_MONITOR_UP,
    true,
    period);
PinInNode buttonMonitorDown(
    "button_monitor_down",
    [](bool v) {
      if (!v) {
        Monitors.set(Actuator::STOP);
        return;
      }
      Monitors.set(Actuator::DOWN);
    },
    BUTTON_MONITOR_DOWN,
    true,
    period);
PinInNode buttonSeatUp(
    "button_seat_up",
    [](bool v) {
      if (!v) {
        Seat.set(Actuator::STOP);
        return;
      }
      Seat.set(Actuator::UP);
    },
    BUTTON_SEAT_UP,
    false,
    period);
PinInNode buttonSeatDown(
    "button_seat_down",
    [](bool v) {
      if (!v) {
        Seat.set(Actuator::STOP);
        return;
      }
      Seat.set(Actuator::DOWN);
    },
    BUTTON_SEAT_DOWN,
    false,
    period);
#if !defined(LOG_SERIAL)
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
// Web server to serve the MQTT web UI. This is NOT the web server when in
// configuration mode.
AsyncWebServer httpSrv(80);
#endif

void onHomieEvent(const HomieEvent& event) {
	// See
  // https://homieiot.github.io/homie-esp8266/docs/2.0.0/advanced-usage/events/
  switch(event.type) {
    case HomieEventType::OTA_STARTED:
    case HomieEventType::ABOUT_TO_RESET:
      // Make sure to stop the actuators on OTA.
      Seat.set(Actuator::STOP);
      Monitors.set(Actuator::STOP);
      break;
    case HomieEventType::MQTT_READY:
      // Broadcast the state of every node.
      LED.init();
      Seat.init();
      Monitors.init();
      buttonMonitorUp.init();
      buttonMonitorDown.init();
      buttonSeatUp.init();
      buttonSeatDown.init();
#if !defined(LOG_SERIAL)
      buttonLED.init();
#endif
      // Reset the actual LEDs.
      // LED.set(buttonLED.get());
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
  Serial.println("LED button disabled because of logging over serial");
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
}

// Only run the loop functions at most once per millisecond.
unsigned long lastLoop;

void loop() {
  unsigned long time = millis();
  if (time != lastLoop) {
    lastLoop = time;
    buttonMonitorUp.update();
    buttonMonitorDown.update();
    buttonSeatUp.update();
    buttonSeatDown.update();
#if !defined(LOG_SERIAL)
    buttonLED.update();
#endif
    Monitors.update();
    Seat.update();
    Homie.loop();
  }
}
