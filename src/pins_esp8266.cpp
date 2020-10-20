// Copyright 2020 Marc-Antoine Ruel. All rights reserved.
// Use of this source code is governed under the Apache License, Version 2.0
// that can be found in the LICENSE file.

#include "pins_esp8266.h"

int PinPWM::set(int v) {
  if (v <= 0) {
    v = 0;
  } else if (v >= PWMRANGE) {
    v = PWMRANGE;
  }
  analogWrite(pin, value_);
  value_ = v;
  return value_;
}

int PinTone::set(int freq, int duration) {
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

