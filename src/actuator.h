// Copyright 2020 Marc-Antoine Ruel. All rights reserved.
// Use of this source code is governed under the Apache License, Version 2.0
// that can be found in the LICENSE file.

#ifndef ACTUATOR_H__
#define ACTUATOR_H__
#pragma once

#include <Homie.hpp>

#include "pins_esp8266.h"

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

  ActuatorNode(const char *name, int left, bool idleleft, int right, bool idleright)
      : HomieNode(name, name, "actuator"),
        left_(left, idleleft), right_(right, idleright) {
    advertise("direction")
        .settable([&](const HomieRange &range, const String &value) {
          return _from_mqtt(value);
        });
    // datatype = enum
    // format = "stop,up,down"
  }

  void init() {
    setProperty("direction").send("stop");
  }

  void set(Direction d) {
    Homie.getLogger() << getId() << ".set(" << dirToStr(d) << ")" << endl;
    if (left_.get() == right_.get()) {
      if (d == UP) {
        set_relays(true, false);
        setProperty("direction").send("up");
        return;
      }
      if (d == DOWN) {
        set_relays(false, true);
        setProperty("direction").send("down");
        return;
      }
    } else if (d != STOP) {
      Homie.getLogger() << "  value ignored due to different pin values" << endl;
    }
    set_relays(false, false);
    setProperty("direction").send("stop");
  }

private:
  void set_relays(bool left, bool right) {
    Homie.getLogger() << getId() << ".set_relays(" << left << ", " << right << ")" << endl;
    left_.set(left);
    right_.set(right);
  }

  bool _from_mqtt(const String &value) {
    Homie.getLogger() << getId() << "._from_mqtt(" << value << ")" << endl;
    // If we get an action and we were not idle, go idle. This is to
    // prevent quick back and forth, which would be harsh on the actuator.
    // Better be safe than sorry.
    if (left_.get() == right_.get()) {
      if (value.equals("up")) {
        set_relays(true, false);
        setProperty("direction").send(value);
        return true;
      }
      if (value.equals("down")) {
        set_relays(false, true);
        setProperty("direction").send(value);
        return true;
      }
      // Ignore bad values and reset to stop. So sending garbagge still
      // stops the actuator.
      if (!value.equals("stop")) {
        Homie.getLogger() << "  bad value" << endl;
      }
    } else if (!value.equals("stop")) {
      Homie.getLogger() << "  value ignored due to different pin values" << endl;
    }
    set_relays(false, false);
    setProperty("direction").send("stop");
    return true;
  }

  PinOut left_;
  PinOut right_;
};

#endif
