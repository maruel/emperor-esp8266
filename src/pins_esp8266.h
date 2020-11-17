// Copyright 2020 Marc-Antoine Ruel. All rights reserved.
// Use of this source code is governed under the Apache License, Version 2.0
// that can be found in the LICENSE file.

// Wrapper classes for ESP-8266 GPIO.
//
// http://arduino.esp8266.com/Arduino/versions/2.0.0/doc/reference.html
// https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/

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
// - RX (GPIO3) Idles High
// - D1 (GPIO5) Idles High
// - D2 (GPIO4) Idles High
// - D3 (GPIO0) Idles High
// - D4 (GPIO2) LED Output
// - GND
// - 5V

// Right:
// - RST button
// - A0 void
// - D0 (GPIO16) Idles Float (or Low)
// - D5 (GPIO14) Idles High
// - D6 (GPIO12) Idles High
// - D7 (GPIO13) Idles High
// - D8 (GPIO15) Idles Low
// - 3v3

#ifndef PINS_ESP8266_H__
#define PINS_ESP8266_H__
#pragma once

#include <Arduino.h>
#include <Bounce2.h>


#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                     \
  TypeName(const TypeName &) = delete;                                         \
  void operator=(const TypeName &) = delete


// Input pin without noise filtering.
//
// It samples the GPIO at every update (which should be called inside loop())
// and that's it.
//
// If idle is true, idles on PULLUP, if false, assumes a pull down. This is
// useful to not cause a "blip" on pins that default to pull high on boot.
class PinInRaw {
public:
  explicit PinInRaw(int pin, bool idle) : pin(pin), idle_(idle) {
    if (pin != A0) {
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
    if (pin == A0) {
      return (analogRead(A0) >= 512)  != idle_;
    }
    return digitalRead(pin) != idle_;
  }

  const bool idle_;
  bool last_;

  DISALLOW_COPY_AND_ASSIGN(PinInRaw);
};

// Debounced input pin.
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

// BounceA0 support debouncing on A0 when used as a digital pin.
class BounceA0 : public Bounce {
protected:
  virtual bool readCurrentState() {
    return analogRead(A0) >= 512;
  }
};

// Debounced input pin for A0.
class PinInDebouncedA0 {
public:
  explicit PinInDebouncedA0(bool idle, int period) : idle_(idle) {
    debouncer_.interval(period);
  }

  // Returns the logical value.
  bool get() {
    return debouncer_.read() != idle_;
  }

  bool update() {
    return debouncer_.update();
  }

  static const int pin = A0;

private:
  BounceA0 debouncer_;
  const bool idle_;

  DISALLOW_COPY_AND_ASSIGN(PinInDebouncedA0);
};

// Output pin.
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

// PWM output pin.
class PinPWM {
public:
  explicit PinPWM(int pin) : pin(pin) {
    pinMode(pin, OUTPUT);
    set(0);
  }

  int set(int v) {
    if (v <= 0) {
      v = 0;
    } else if (v >= PWMRANGE) {
      v = PWMRANGE;
    }
    analogWrite(pin, value_);
    value_ = v;
    return value_;
  }
  int get() { return value_; }

  const int pin;

private:
  int value_;

  DISALLOW_COPY_AND_ASSIGN(PinPWM);
};

// PWM pin meant to be used as a buzzer using the tone() function.
class PinTone {
public:
  explicit PinTone(int pin) : pin(pin) {
    pinMode(pin, OUTPUT);
    set(0, -1);
  }

  // Use -1 for duration for infinite duration.
  int set(int freq, int duration) {
    if (freq <= 0) {
      noTone(pin);
      freq_ = 0;
    } else {
      if (freq >= 10000) {
        freq = 10000;
      }
      tone(pin, freq, duration);
      freq_ = freq;
    }
    return freq_;
  }

  int get() { return freq_; }

  const int pin;

private:
  int freq_;

  DISALLOW_COPY_AND_ASSIGN(PinTone);
};

#endif
