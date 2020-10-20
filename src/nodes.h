// Copyright 2019 Marc-Antoine Ruel. All rights reserved.
// Use of this source code is governed under the Apache License, Version 2.0
// that can be found in the LICENSE file.

// Homie nodes.
//
// See https://homieiot.github.io/specification/spec-core-develop/ for the
// MQTT convention.

#ifndef NODES_H__
#define NODES_H__
#pragma once

#include "pins_esp8266.h"

#include <Homie.h>


#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                     \
  TypeName(const TypeName &) = delete;                                         \
  void operator=(const TypeName &) = delete

int isBool(const String &v);
int toInt(const String &v, int min, int max);
String urlencode(const String& src);


// Homie node representing an input pin. It is read only.
//
// onSet is called with true being the non-idle value. So if idle is true, the
// value sent to onSet() are reversed.
class PinInNode : public HomieNode {
public:
  explicit PinInNode(const char *name, void (*onSet)(bool v), int pin,
                     bool idle, int period)
      : HomieNode(name, name, "input"), onSet_(onSet), pin_(pin, idle, period) {
    advertise("on");
    // datatype = "boolean"
  }

  void init() {
    broadcast();
  }

  // Returns the logical value of the pin.
  bool get() {
    return pin_.get();
  }

  // Must be called at every loop.
  bool update() {
    if (!pin_.update()) {
      return false;
    }
    broadcast();
    return true;
  }

private:
  void broadcast() {
    bool level = pin_.get();
    const char* value = level ? "true" : "false";
    Homie.getLogger() << getId() << ".broadcast(" << value << ")" << endl;
    setProperty("on").send(value);
    onSet_(level);
  }

  void (*const onSet_)(bool v);
  PinInDebounced pin_;

  DISALLOW_COPY_AND_ASSIGN(PinInNode);
};

// Homie node representing an output pin.
//
// If idle is true, acts in reverse. This is important for pins that are pull
// high, thus default to high upon boot which lasts ~600ms. This is most of the
// pins.
class PinOutNode : public HomieNode {
public:
  explicit PinOutNode(const char *name, int pin, bool idle,
                      void (*onSet)(bool v))
      : HomieNode(name, name, "output"), onSet_(onSet), pin_(pin, idle) {
    advertise("on").settable(
        [&](const HomieRange &range, const String &value) {
          return _from_mqtt(value);
        });
    // datatype = "boolean"
  }

  void init() {
    setProperty("on").send("false");
  }

  // Overiddes the value and broadcast it.
  void set(bool level) {
    pin_.set(level);
    const char* value = level ? "true" : "false";
    Homie.getLogger() << getId() << ".set(" << value << ")" << endl;
    setProperty("on").send(value);
  }

  bool get() {
    return pin_.get();
  }

private:
  bool _from_mqtt(const String &value);

  void (*const onSet_)(bool v);
  PinOut pin_;

  DISALLOW_COPY_AND_ASSIGN(PinOutNode);
};

// Homie node representing a PWM output.
//
// For most pins idle should be true since most pins have a pull up.
class PinPWMNode : public HomieNode {
public:
  explicit PinPWMNode(const char *name, int pin,
                      void (*onSet)(int v))
      : HomieNode(name, name, "pwm"), onSet_(onSet), pin_(pin) {
    advertise("pwm").settable(
        [&](const HomieRange &range, const String &value) {
          return _from_mqtt(value);
        });
    // datatype = "integer"
    // format = 0:PWMRANGE
    // or
    // datatype = "float"
    // format = 0:100
    // unit: %
  }

  void init() {
    setProperty("pwm").send("0");
  }

  void set(int level) {
    String value(pin_.set(level));
    Homie.getLogger() << getId() << ".set(" << value << ")" << endl;
    setProperty("pwm").send(value);
  }

  int get() {
    return pin_.get();
  }

private:
  bool _from_mqtt(const String &value);

  void (*const onSet_)(int v);
  PinPWM pin_;

  DISALLOW_COPY_AND_ASSIGN(PinPWMNode);
};

// Homie node representing a buzzer output.
class PinToneNode : public HomieNode {
public:
  explicit PinToneNode(const char *name, int pin,
                      void (*onSet)(int v))
      : HomieNode(name, name, "freq"), onSet_(onSet), pin_(pin) {
    advertise("freq").settable(
        [&](const HomieRange &range, const String &value) {
          return _from_mqtt(value);
        });
    // datatype = "integer"
    // format = 0:20000
    // unit = Hz
  }

  void init() {
    setProperty("freq").send("0");
  }

  void set(int freq) {
    String value(pin_.set(freq, -1));
    Homie.getLogger() << getId() << ".set(" << value << ")" << endl;
    setProperty("freq").send(value);
  }

  int get() {
    return pin_.get();
  }

private:
  bool _from_mqtt(const String &value);

  void (*const onSet_)(int v);
  PinTone pin_;

  DISALLOW_COPY_AND_ASSIGN(PinToneNode);
};

#endif
