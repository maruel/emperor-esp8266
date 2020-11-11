#!/usr/bin/env python3
# Copyright 2020 Marc-Antoine Ruel. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Prints out build flags."""

import os
import sys

import calculate_version

ver = calculate_version.calculate_version()
# Automatically calculated firmware version.
sys.stdout.write(' -DGIT_REV=\\"%s\\"' % ver)

# Enable serial logging:
if False:
  sys.stdout.write(" -DLOG_SERIAL=1")

# Enables MQTT over SSL:
# https://homieiot.github.io/homie-esp8266/docs/3.0.0/advanced-usage/compiler-flags/
if False:
  sys.stdout.write(" -DASYNC_TCP_SSL_ENABLED=1")
  sys.stdout.write(" -DPIO_FRAMEWORK_ARDUINO_LWIP2_HIGHER_BANDWIDTH")

# Add whatever variable set, makes things simpler:
if os.environ.get("ESP8266_FLAGS"):
  sys.stdout.write(" " + os.environ["ESP8266_FLAGS"])
