#!/bin/bash
# Copyright 2019 Marc-Antoine Ruel. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

# Update both the code and data partitions.

set -eu

cd "$(dirname $0)"

./setup.sh

source .venv/bin/activate

if [ ! -f data/homie/config.json ]; then
  echo "Create data/homie/config.json file first."
  echo "See config_example.json as a example."
  exit 1
fi

# Manually erase the file, since the cache is not updated properly on file
# change.
# TODO(maruel): Doesn't seem like it's enough.
if [ -f .pio/build/d1_mini/spiffs.bin ]; then
  rm .pio/build/d1_mini/spiffs.bin
fi

echo "- Building code"
pio run -s
echo "- Building data"
pio run --target buildfs -s
echo "- Flashing data (SPIFFS)"
echo "  This erases all saved settings"
pio run --target uploadfs -s

echo "- Flashing code"
pio run --target upload -s

echo ""
echo "Flashed:"
./scripts/firmware_parser.py

echo ""
echo "Congratulations!"
echo "If you want to use debugging, enable it in platform.ini,"
echo "then you can monitor the device over the serial port:"
echo "  source .venv/bin/activate"
echo "  pio device monitor"
