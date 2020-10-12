#!/bin/bash
# Copyright 2020 Marc-Antoine Ruel. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

set -eu

cd "$(dirname $0)"

PLATFORMIO="$(which platformio || true)"
if [ "$PLATFORMIO" != "" ]; then
  # Already in PATH, skip.
  exit 0
fi

if [ ! -d .venv ]; then
  mkdir .venv
fi

if [ ! -f ./.venv/bin/activate ]; then
  VIRTUALENV="$(which virtualenv || true)"
  if [ "$VIRTUALENV" = "" ]; then
    echo ""
    echo "Get virtualenv"
    echo ""
    curl https://bootstrap.pypa.io/virtualenv.pyz > .venv/virtualenv.pyz
    echo ""
    echo "Create virtualenv"
    echo ""
    python3 .venv/virtualenv.pyz .venv
    rm .venv/virtualenv.pyz
  else
    echo ""
    echo "Create virtualenv"
    echo ""
    $VIRTUALENV .venv
  fi
fi

echo ""
echo "Activating virtualenv"
echo ""
source ./.venv/bin/activate

PLATFORMIO="$(which platformio || true)"
if [ "$PLATFORMIO" != "" ]; then
  # Already in PATH, skip.
  exit 0
fi
echo ""
echo "Get platformio"
echo ""
pip install -U platformio

echo "Congratulations!"
echo "Everything is inside ./.venv/"
echo "To access platformio, run:"
echo "  source ./.venv/bin/activate"
