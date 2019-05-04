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
// - D0 (GPIO16) In; pulse signal to RST to wake up from wifi

// Left:
// - TX (GPIO1) Idles High
// - RX (GPIO3) Idles High; Button Seat Up
// - D1 (GPIO5) Idles High; Button Seat Down
// - D2 (GPIO4) Idles High; Button Monitor Up
// - D3 (GPIO0) Idles High; Button Monitor Down
// - D4 (GPIO2) LED Output; Low when LED on
// - GND
// - 5V

// Right:
// - RST button
// - A0 void
// - D0 (GPIO16) Idles High; Button LED
// - D5 (GPIO14) Idles High; Actuator Seat Down 
// - D6 (GPIO12) Idles High; Actuator Monitor Up
// - D7 (GPIO13) Idles High; Actuator Monitor Down
// - D8 (GPIO15) Idles Low ; Actuator Seat Up
// - 3v3


#include <Arduino.h>
#include <Homie.h>
#include <ESPAsyncWebServer.h>

#include "nodes.h"

// Enable to start the web server. Uses more memory.
#define USE_WEB_SERVER 1

// Enable debugging.
//#define LOG_SERIAL

const int BUTTON_SEAT_UP = RX;      // GPIO3; Idles High (UART)
const int BUTTON_SEAT_DOWN = D1;    // GPIO5; Idles High
const int BUTTON_MONITOR_UP = D2;   // GPIO4; Idles High
const int BUTTON_MONITOR_DOWN = D3; // GPIO0; Idles High (Boot mode) (Pull Up)
const int BUTTON_LED = D0;          // GPIO16; Idles Low (Pulse to wake up)
const int MOTOR_SEAT_UP = D5;       // GPIO14; Idles High
const int MOTOR_SEAT_DOWN = D6;     // GPIO12; Idles High
const int MOTOR_MONITOR_UP = D7;    // GPIO13; Idles High
const int MOTOR_MONITOR_DOWN = D8;  // GPIO15; Pull Down
const int LED_OUT = D4;             // GPIO2 (Pull Up; Also onboard LED)

// Homie node to control a bidirectional actuator.
//
// Unlike a normal motor, the actuator can only run each way for a certain
// amount of time, as it eventually is fully extended or retracted.
// 
// Each relay output can idle on different values, depending on the GPIO used.
class Actuator : public HomieNode {
public:
  enum Direction {
    STOP = 0,
    UP = 1,
    DOWN = 2,
  };

  static const char *dirToStr(Direction d) {
    switch (d) {
    case STOP:
      return "STOP";
    case UP:
      return "UP";
    case DOWN:
      return "DOWN";
    default:
      return "<INVALID DIRECTION>";
    }
  }

  Actuator(const char *name, int left, bool idleleft, int right, bool idleright)
      : HomieNode(name, "motor"), idleleft_(idleleft), idleright_(idleright),
        left_(left, idleleft), right_(right, idleright) {
    advertise("direction")
        .settable([&](const HomieRange &range, const String &value) {
          if (value.equalsIgnoreCase("STOP")) {
            motor(STOP);
            return true;
          }
          if (value.equalsIgnoreCase("UP")) {
            motor(UP);
            return true;
          }
          if (value.equalsIgnoreCase("DOWN")) {
            motor(DOWN);
            return true;
          }
          Homie.getLogger() << getId() << ": Bad value: " << value << endl;
          return true;
        });
    setProperty("direction").send("STOP");
  }

  void motor(Direction d) {
    Homie.getLogger() << dirToStr(d) << endl;
    switch (d) {
    default:
    case STOP:
      set(idleleft_, idleright_);
      break;
    case UP:
      // If we were not in STOP, stops instead. This is safer.
      if (left_.get() != right_.get()) {
        set(idleleft_, idleright_);
      } else {
        set(!idleleft_, idleright_);
      }
      break;
    case DOWN:
      // If we were not in STOP, stops instead. This is safer.
      if (left_.get() != right_.get()) {
        set(idleleft_, idleright_);
      } else {
        set(idleleft_, !idleright_);
      }
      break;
    }
  }

private:
  void set(bool v_left, bool v_right) {
    left_.set(v_left);
    right_.set(v_right);
  }

  const bool idleleft_;
  const bool idleright_;
  PinOut left_;
  PinOut right_;
};

//
// Homie nodes accessible through MQTT.
//

// Outputs.
PinOutNode LED("led", LED_OUT, true);
Actuator Seat("seat", MOTOR_SEAT_UP, true, MOTOR_SEAT_DOWN, true);
Actuator Monitors("monitors", MOTOR_MONITOR_UP, true, MOTOR_MONITOR_DOWN, false);

// Inputs. All of them idles High, so they are active when Low.
PinInNode buttonMonitorUp(
    "button_monitor_up",
    [](bool v) {
      if (v) {
        Monitors.motor(Actuator::STOP);
        return;
      }
      Monitors.motor(Actuator::UP);
    },
    BUTTON_MONITOR_UP,
    true);
PinInNode buttonMonitorDown(
    "button_monitor_down",
    [](bool v) {
      if (v) {
        Monitors.motor(Actuator::STOP);
        return;
      }
      Monitors.motor(Actuator::DOWN);
    },
    BUTTON_MONITOR_DOWN,
    true);
PinInNode buttonSeatUp(
    "button_seat_up",
    [](bool v) {
      if (v) {
        Monitors.motor(Actuator::STOP);
        return;
      }
      Seat.motor(Actuator::UP);
    },
    BUTTON_SEAT_UP,
    true);
PinInNode buttonSeatDown(
    "button_seat_down",
    [](bool v) {
      if (v) {
        Monitors.motor(Actuator::STOP);
        return;
      }
      Seat.motor(Actuator::DOWN);
    },
    BUTTON_SEAT_DOWN,
    true);
PinInNode buttonLED(
    "button_led",
    [](bool v) {
      LED.set(v);
    },
    BUTTON_LED,
    false);


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
      Seat.motor(Actuator::STOP);
      Monitors.motor(Actuator::STOP);
      break;
    case HomieEventType::MQTT_READY:
      // Reset the actual LEDs.
      LED.set(buttonLED.get());
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
  // Setup.
  Homie_setFirmware("emperor", "1.0.0");
  Homie_setBrand("Emperor");

#if defined(LOG_SERIAL)
  Serial.println();
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
        // TODO(maruel): The websocket port number != cfg.mqtt.server.port.
        url += "&port=9001";
        if (cfg.mqtt.auth) {
          url += "&user=";
          url += urlencode(cfg.mqtt.username);
          url += "&pass=";
          url += urlencode(cfg.mqtt.password);
        }
        request->redirect(url);
    });
    httpSrv.serveStatic("/", SPIFFS, "/html/").setCacheControl("public; max-age=600");
    httpSrv.begin();
  }
#endif
}

void loop() {
  Homie.loop();
  buttonMonitorUp.update();
  buttonMonitorDown.update();
  buttonSeatUp.update();
  buttonSeatDown.update();
  buttonLED.update();
}
