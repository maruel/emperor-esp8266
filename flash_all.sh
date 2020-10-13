#!/bin/bash
# Copyright 2019 Marc-Antoine Ruel. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

set -eu

cd "$(dirname $0)"

./install_platformio.sh

source .venv/bin/activate
PLATFORMIO="$(which platformio)"

if [ ! -f data/homie/config.json ]; then
  echo "Create data/homie/config.json file first."
  echo "See config_example.json as a example."
  exit 1
fi

echo "Before uploading, make sure it builds"
echo ""
platformio run


platformio run --target buildfs
echo ""
echo "Flashing SPIFFS"
echo "This erases all saved settings"
echo ""
platformio run --target uploadfs

# TODO(maruel): Script for OTA
# http://homieiot.github.io/homie-esp8266/docs/develop/others/ota-configuration-updates/
# https://github.com/homieiot/homie-esp8266/blob/develop/scripts/ota_updater/ota_updater.py

echo ""
echo "Flashing code"
platformio run --target upload

echo ""
echo "Congratulations!"
echo "If you want to use debugging, enable it in platform.ini,"
echo "then you can monitor the device over the serial port:"
echo "  $PLATFORMIO device monitor --baud 115200"
