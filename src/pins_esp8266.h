// Copyright 2020 Marc-Antoine Ruel. All rights reserved.
// Use of this source code is governed under the Apache License, Version 2.0
// that can be found in the LICENSE file.

// Wrapper classes for ESP-8266 GPIO.

#include <Arduino.h>
#include <Bounce2.h>


#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                     \
  TypeName(const TypeName &) = delete;                                         \
  void operator=(const TypeName &) = delete


// Wrapper for an input pin without noise filtering.
//
// It samples the GPIO at every update (which should be called inside loop())
// and that's it.
//
// If idle is true, idles on PULLUP, if false, assumes a pull down. This is
// useful to not cause a "blip" on pins that default to pull high on boot.
class PinInRaw {
public:
  explicit PinInRaw(int pin, bool idle) : pin(pin), idle_(idle) {
    if (idle) {
      // Assert not D0.
      pinMode(pin, INPUT_PULLUP);
    } else {
      // GPIO16 is a bit one-off.
      if (pin == D0) {
        pinMode(pin, INPUT_PULLDOWN_16);
      } else {
        pinMode(pin, INPUT);
      }
    }
    last_ = raw_get();
  }

  // Returns the logical value.
  bool get() {
    return last_;
  }

  bool update() {
    bool cur = raw_get();
    if (cur != last_) {
      last_ = cur;
      return true;
    }
    return false;
  }

  const int pin;

private:
  bool raw_get() {
    return digitalRead(pin) != idle_;
  }

  const bool idle_;
  bool last_;

  DISALLOW_COPY_AND_ASSIGN(PinInRaw);
};

// Wrapper for a debounced input pin.
//
// It samples the GPIO at every update (which should be called inside loop())
// and wait for at least <period> ms before reacting.
//
// If idle is true, idles on PULLUP, if false, assumes a pull down. This is
// useful to not cause a "blip" on pins that default to pull high on boot.
class PinInDebounced {
public:
  explicit PinInDebounced(int pin, bool idle, int period) : pin(pin), idle_(idle) {
    debouncer_.interval(period);
    if (idle) {
      debouncer_.attach(pin, INPUT_PULLUP);
    } else {
      // GPIO16 is a bit one-off.
      if (pin == D0) {
        debouncer_.attach(pin, INPUT_PULLDOWN_16);
      } else {
        debouncer_.attach(pin, INPUT);
      }
    }
  }

  // Returns the logical value.
  bool get() {
    return debouncer_.read() != idle_;
  }

  bool update() {
    return debouncer_.update();
  }

  const int pin;

private:
  Bounce debouncer_;
  const bool idle_;

  DISALLOW_COPY_AND_ASSIGN(PinInDebounced);
};

// Wrapper for an output pin.
//
// If idle is true, the values are reversed. This is useful to not cause a
// "blip" on pins that default to pull high on boot.
class PinOut {
public:
  explicit PinOut(int pin, bool idle) : pin(pin), idle_(idle) {
    pinMode(pin, OUTPUT);
    set(false);
  }

  // Sets the logical value.
  void set(bool l) {
    // Enabling this here is very noisy, but useful when deep down into
    // debugging. Hence it's commented out by default but feel free to
    // temporarily enable it if you are having a hard time.
    //Homie.getLogger() << pin << ".set(" << l << ")" << endl;
    digitalWrite(pin, l != idle_ ? HIGH : LOW);
    value_ = l;
  }

  // Returns the logical value.
  bool get() { return value_; }

  const int pin;

private:
  bool value_;
  const bool idle_;

  DISALLOW_COPY_AND_ASSIGN(PinOut);
};

// Wrapper for a PWM output pin.
class PinPWM {
public:
  explicit PinPWM(int pin) : pin(pin) {
    pinMode(pin, OUTPUT);
    set(0);
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
  explicit PinTone(int pin) : pin(pin) {
    pinMode(pin, OUTPUT);
    set(0, -1);
  }

  // Use -1 for duration for infinite duration.
  int set(int freq, int duration);
  int get() { return freq_; }

  const int pin;

private:
  int freq_;

  DISALLOW_COPY_AND_ASSIGN(PinTone);
};
