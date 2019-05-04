// Copyright 2019 Marc-Antoine Ruel. All rights reserved.
// Use of this source code is governed under the Apache License, Version 2.0
// that can be found in the LICENSE file.

#include <Homie.h>

#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                     \
  TypeName(const TypeName &) = delete;                                         \
  void operator=(const TypeName &) = delete

int isBool(const String &v);
int toInt(const String &v, int min, int max);
String urlencode(const String& src);

// Wrapper for an output pin.
class PinOut {
public:
  explicit PinOut(int pin, bool level) : pin(pin) {
    pinMode(pin, OUTPUT);
    set(level);
  }

  void set(bool l) {
    digitalWrite(pin, l ? HIGH : LOW);
    value_ = l;
  }

  bool get() { return value_; }

  const int pin;

private:
  bool value_;

  DISALLOW_COPY_AND_ASSIGN(PinOut);
};

// Wrapper for a PWM output pin.
class PinPWM {
public:
  explicit PinPWM(int pin, int level = 0) : pin(pin) {
    pinMode(pin, OUTPUT);
    set(level);
  }

  int set(int v);
  int get() { return value_; }

  const int pin;

private:
  int value_;

  DISALLOW_COPY_AND_ASSIGN(PinPWM);
};

// Wrapper for a PWM pin meant to be used as a buzzer using the tone() function.
class PinTone {
public:
  explicit PinTone(int pin, int freq = 0) : pin(pin) {
    pinMode(pin, OUTPUT);
    set(freq);
  }

  int set(int freq, int duration = -1);
  int get() { return freq_; }

  const int pin;

private:
  int freq_;

  DISALLOW_COPY_AND_ASSIGN(PinTone);
};

//
// Homie nodes.
//

// Homie node representing an input pin.
//
// onSet is called with true being the non-idle value. So if idle is true, the
// value sent to onSet() are reversed.
//
// Uses a debouncer with a 25ms delay.
class PinInNode : public HomieNode {
public:
  explicit PinInNode(const char *name, void (*onSet)(bool v), int pin,
                     bool idle)
      : HomieNode(name, "input"), onSet_(onSet), idle_(idle) {
    if (idle) {
      debouncer_.attach(pin, INPUT_PULLUP);
    } else {
      debouncer_.attach(pin, INPUT);
    }
    debouncer_.interval(25);
    advertise("on");
    if (!update()) {
      broadcast(debouncer_.read());
    }
  }

  bool update();
  bool get() {
    return debouncer_.read();
  }

private:
  void broadcast(bool level) {
    setProperty("on").send(idle_ != level ? "true" : "false");
    onSet_(idle_ != level);
  }

  void (*const onSet_)(bool v);
  Bounce debouncer_;
  const bool idle_;

  DISALLOW_COPY_AND_ASSIGN(PinInNode);
};

// Homie node representing an output pin.
class PinOutNode : public HomieNode {
public:
  explicit PinOutNode(const char *name, int pin, bool level,
                      void (*onSet)(bool v) = NULL)
      : HomieNode(name, "output"), onSet_(onSet), pin_(pin, level) {
    advertise("on").settable([&](const HomieRange &range, const String &value) {
      return this->_onPropSet(value);
    });
    set(level);
  }

  void set(bool level) {
    pin_.set(level);
    setProperty("on").send(level ? "true" : "false");
  }

  bool get() {
    return pin_.get();
  }

private:
  bool _onPropSet(const String &value);

  void (*const onSet_)(bool v);
  PinOut pin_;

  DISALLOW_COPY_AND_ASSIGN(PinOutNode);
};

// Homie node representing a PWM output.
class PinPWMNode : public HomieNode {
public:
  explicit PinPWMNode(const char *name, int pin, int level = 0,
                      void (*onSet)(int v) = NULL)
      : HomieNode(name, "pwm"), onSet_(onSet), pin_(pin, level) {
    advertise("pwm").settable(
        [&](const HomieRange &range, const String &value) {
          return this->_onPropSet(value);
        });
    set(level);
  }

  void set(int level) {
    setProperty("pwm").send(String(pin_.set(level)));
  }

  int get() {
    return pin_.get();
  }

private:
  bool _onPropSet(const String &value);

  void (*const onSet_)(int v);
  PinPWM pin_;

  DISALLOW_COPY_AND_ASSIGN(PinPWMNode);
};

// Homie node representing a buzzer output.
class PinToneNode : public HomieNode {
public:
  explicit PinToneNode(const char *name, int pin, int freq = 0,
                      void (*onSet)(int v) = NULL)
      : HomieNode(name, "freq"), onSet_(onSet), pin_(pin, freq) {
    advertise("freq").settable(
        [&](const HomieRange &range, const String &value) {
          return this->_onPropSet(value);
        });
    set(freq);
  }

  void set(int freq) {
    setProperty("freq").send(String(pin_.set(freq)));
  }

  int get() {
    return pin_.get();
  }

private:
  bool _onPropSet(const String &value);

  void (*const onSet_)(int v);
  PinTone pin_;

  DISALLOW_COPY_AND_ASSIGN(PinToneNode);
};
