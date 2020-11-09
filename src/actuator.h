// Copyright 2020 Marc-Antoine Ruel. All rights reserved.
// Use of this source code is governed under the Apache License, Version 2.0
// that can be found in the LICENSE file.

#ifndef ACTUATOR_H__
#define ACTUATOR_H__
#pragma once

#include <Homie.hpp>

#include "pins_esp8266.h"

class Actuator {
public:
  enum Direction {
    STOP = 0,
    UP = 1,
    DOWN = 2,
  };

  static const char *dirToStr(Direction d) {
    switch (d) {
    case STOP:
      return "stop";
    case UP:
      return "up";
    case DOWN:
      return "down";
    default:
      return "<invalid direction>";
    }
  }

  Actuator(int left, bool idleleft, int right, bool idleright, int delayup, int delaydown)
    : delayup_(delayup),
      delaydown_(delaydown),
      left_(left, idleleft),
      right_(right, idleright),
      dir_(STOP),
      stopat_(0) {
  }

  Direction get() {
    return dir_;
  }

  // set sets the new direction. Returns the actual direction choosen.
  Direction set(Direction d) {
    if (dir_ == STOP) {
      switch (d) {
        case STOP:
          // Do nothing when receiving a STOP while we were in STOP.
          return d;
        case UP:
          _set_relays(true, false);
          stopat_ = (millis() + delayup_) & 0xFFFFFFFF;
          break;
        case DOWN:
          _set_relays(false, true);
          stopat_ = (millis() + delaydown_) & 0xFFFFFFFF;
          break;
      }
      if (stopat_ == 0) {
        stopat_ = 1;
      }
      dir_ = d;
    } else {
      // On any other confusing message, like repeated UP, stop for safety.
      _set_relays(false, false);
      dir_ = STOP;
      stopat_ = 0;
    }
    return dir_;
  }

  // update updates the actuator state based on time. It shall be called inside
  // loop().
  bool update() {
    if (stopat_ != 0 && millis() >= stopat_) {
      set(STOP);
      return true;
    }
    return false;
  }

private:
  void _set_relays(bool left, bool right) {
    left_.set(left);
    right_.set(right);
  }

  const unsigned long delayup_;
  const unsigned long delaydown_;
  PinOut left_;
  PinOut right_;
  Direction dir_;
  unsigned long stopat_;

  DISALLOW_COPY_AND_ASSIGN(Actuator);
};

// Homie node to control a bidirectional actuator.
//
// Unlike a normal motor, the actuator can only run each way for a certain
// amount of time, as it eventually is fully extended or retracted.
//
// See https://en.wikipedia.org/wiki/Actuator
//
// Each relay output can idle on different values, depending on the GPIO used.
class ActuatorNode : public HomieNode {
public:
  ActuatorNode(const char *name, int left, bool idleleft, int right, bool idleright, int delayup, int delaydown)
      : HomieNode(name, name, "actuator"),
        actuator_(left, idleleft, right, idleright, delayup, delaydown) {
    advertise("direction")
        .settable([&](const HomieRange &range, const String &value) {
          return _from_mqtt(value);
        });
    // datatype = enum
    // format = "stop,up,down"
  }

  // init initializes the state, including both the LED and the MQTT topic.
  void init() {
    set(Actuator::STOP);
  }

  // set is the high level function to set the direction and updates the MQTT
  // topic if needed.
  Actuator::Direction set(Actuator::Direction d) {
    Actuator::Direction actual = actuator_.set(d);
    _updateLED(actual);
    _to_mqtt(actual);
    return actual;
  }

  // update gets the current value from the underlying actuator, and updates the
  // MQTT topic if needed.
  bool update() {
    if (actuator_.update()) {
      Actuator::Direction actual = actuator_.get();
      _updateLED(actual);
      _to_mqtt(actual);
      return true;
    }
    return false;
  }

private:
  // _from_mqtt is called when an incoming MQTT message is received.
  bool _from_mqtt(const String &value) {
    Homie.getLogger() << getId() << "._from_mqtt(" << value << ")" << endl;
    // If we get an action and we were not idle, go idle. This is to
    // prevent quick back and forth, which would be harsh on the actuator.
    // Better be safe than sorry.
    if (actuator_.get() == Actuator::STOP) {
      if (value.equals("up")) {
        set(Actuator::UP);
        return true;
      }
      if (value.equals("down")) {
        set(Actuator::DOWN);
        return true;
      }
      // Ignore bad values and reset to stop. So sending garbage still
      // stops the actuator.
      if (!value.equals("stop")) {
        Homie.getLogger() << "  bad value" << endl;
      }
    } else if (!value.equals("stop")) {
      Homie.getLogger() << "  value ignored due to actuator not being stopped" << endl;
    }
    set(Actuator::STOP);
    return true;
  }

  // _to_mqtt is to be called when we need to change the value.
  void _to_mqtt(Actuator::Direction d) {
    const char * v;
    switch (d) {
    case Actuator::UP:
      v = "up";
      break;
    case Actuator::DOWN:
      v = "down";
      break;
    case Actuator::STOP:
      v = "stop";
      break;
    }
    setProperty("direction").send(v);
  }

  // _updateLED updates the LED.
  void _updateLED(Actuator::Direction d) {
    if (d != Actuator::STOP) {
      HomieInternals::Interface::get().getBlinker().start(0.3);
    } else {
      HomieInternals::Interface::get().getBlinker().stop();
    }
  }

  Actuator actuator_;

  DISALLOW_COPY_AND_ASSIGN(ActuatorNode);
};

#endif
