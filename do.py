#!/usr/bin/env python
# Copyright 2019 Marc-Antoine Ruel. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import argparse
import os
import subprocess
import sys


def flash_serial(spiffs):
  if spiffs:
    print('')
    print('Building SPIFFS')
    print('This erases all saved settings')
    print('')
    subprocess.check_call(['platformio', 'run', '--target', 'buildfs'])

    print('')
    print('Flashing SPIFFS')
    print('This erases all saved settings')
    print('')
    subprocess.check_call(['platformio', 'run', '--target', 'uploadfs'])

  print('')
  print('Flashing code')
  print('')
  subprocess.check_call(['platformio', 'run', '--target', 'upload'])


def main():
  parser = argparse.ArgumentParser()
  # TODO(maruel): Make it two subcommands.
  parser.add_argument(
      '--serial', action='store_true',
      help='flash the firmware over serial')
  parser.add_argument(
      '--spiffs', action='store_true',
      help='flash the SPIFFS partition too')
  parser.add_argument(
      '--ota-mqtt', action='store_true',
      help='flash the firmware via MQTT with Homie protocol')
  parser.add_argument(
      '-H', '--host', help='MQTT host')
  parser.add_argument(
      '-P', '--port', type=int, default=1883, help='MQTT port')
  parser.add_argument(
      '-u', '--user', default='homie', help='MQTT username')
  parser.add_argument(
      '-p', '--password', default='homie', help='MQTT password')
  parser.add_argument(
      '-d', '--device', default='emperor', help='Homie device to flash')
  args = parser.parse_args()

  # Append platformio tool from Atom's package as the default if it wasn't
  # installed system wide.
  home = os.path.expanduser('~')
  bin_dir = os.path.join(
      home, '.atom', 'packages', 'platformio-ide', 'penv', 'bin')
  if os.path.isdir(bin_dir):
    os.environ['PATH'] += os.pathsep + bin_dir

  print('Building the firmware')
  print('')
  subprocess.check_call(['platformio', 'run'])
  print('')

  # Print the firmware properties.
  firmware = os.path.join('.pioenvs', 'd1_mini', 'firmware.bin')
  subprocess.check_call(
      [
        sys.executable, os.path.join('scripts', 'firmware_parser.py'),
        firmware,
      ])
  print('')

  if args.serial:
    flash_serial(args.spiffs)
  elif args.host and args.device:
    # http://homieiot.github.io/homie-esp8266/docs/develop/others/ota-configuration-updates/
    subprocess.check_call(
        [
          sys.executable, os.path.join('scripts', 'ota_updater.py'),
          '-l', args.host,
          '-p', str(args.port),
          '-u', args.user,
          '-d', args.password,
          '-t', 'homie',
          '-i', args.device,
          firmware,       
        ])
  else:
    parser.error('provide either --serial or --device')

  print('Congratulations!')
  if args.serial:
    #PLATFORMIO="$(which platformio)"
    print('Run the following to monitor the device over the serial port:')
    print('')
    print('  platformio serialports monitor --baud 115200')
  return 0


if __name__ == '__main__':
  sys.exit(main())
