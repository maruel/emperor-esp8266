#!/bin/bash
# Copyright 2020 Marc-Antoine Ruel. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

set -eu

cd "$(dirname $0)"

echo -n " -DGIT_REV="'\"'"$(./calculate_version.py)"'\"'

# Enable serial logging:
#echo -n " -DLOG_SERIAL=1"

# Enables MQTT over SSL:
# https://homieiot.github.io/homie-esp8266/docs/3.0.0/advanced-usage/compiler-flags/
#echo " -DASYNC_TCP_SSL_ENABLED=1"
#echo " -DPIO_FRAMEWORK_ARDUINO_LWIP2_HIGHER_BANDWIDTH"

echo ""
