#!/usr/bin/env python
# Copyright 2019 Marc-Antoine Ruel. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import argparse
import os
import subprocess
import sys

DIR = os.path.dirname(os.path.abspath(__file__))


def flash_serial(spiffs):
  if spiffs:
    print('')
    print('Building SPIFFS')
    print('This erases all saved settings')
    print('')
    subprocess.check_call(
        ['platformio', 'run', '--target', 'buildfs', '--silent'])

    print('')
    print('Flashing SPIFFS')
    print('This erases all saved settings')
    print('')
    subprocess.check_call(['platformio', 'run', '--target', 'uploadfs'])

  print('')
  print('Flashing code')
  print('')
  subprocess.check_call(['platformio', 'run', '--target', 'upload'])


def do(args):
  print('Building the firmware')
  env = os.environ.copy()
  if args.log_serial:
    env['LOG_SERIAL'] = (
        env.get('LOG_SERIAL', '') + ' -D LOG_SERIAL=1').lstrip()
  subprocess.check_call(['platformio', 'run', '--silent'], env=env)

  # Print the firmware properties.
  firmware = os.path.join('.pioenvs', 'd1_mini', 'firmware.bin')
  subprocess.check_call(
      [
        sys.executable, os.path.join('scripts', 'firmware_parser.py'),
        firmware,
      ])
  print('')

  if args.mode == 'serial':
    flash_serial(args.spiffs)
  else:
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

  print('Congratulations!')
  if args.mode == 'serial':
    #PLATFORMIO="$(which platformio)"
    cmd = ['platformio', 'serialports', 'monitor', '--baud', '115200']
    print('Run the following to monitor the device over the serial port:')
    print('')
    print('  %s' % ' '.join(cmd))
    return subprocess.call(cmd)
  return 0


def main():
  os.chdir(DIR)
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--log-serial', action='store_true',
      help='Enable logging over serial in the firmware')

  subparsers = parser.add_subparsers(
      title='subcommands', description='valid subcommands')

  parser_cmd = subparsers.add_parser(
      'serial', help='flash the firmware over serial')
  parser_cmd.add_argument(
      '--spiffs', action='store_true',
      help='flash the SPIFFS partition too')
  parser_cmd.set_defaults(mode='serial')

  parser_cmd = subparsers.add_parser(
      'mqtt', help='flash the firmware via MQTT with Homie protocol')
  parser_cmd.add_argument(
      '-H', '--host', help='MQTT host', required=True)
  parser_cmd.add_argument(
      '-p', '--port', type=int, default=1883, metavar='1883',
      help='MQTT port')
  parser_cmd.add_argument(
      '-u', '--user', default='homie', metavar='homie', help='MQTT username')
  parser_cmd.add_argument(
      '-P', '--password', default='homie', metavar='homie',
      help='MQTT password')
  parser_cmd.add_argument(
      '-d', '--device', default='emperor',
      metavar='emperor',
      help='Homie device to flash')
  parser_cmd.set_defaults(mode='mqtt')

  args = parser.parse_args()

  # Append platformio tool from Atom's package as the default if it wasn't
  # installed system wide.
  home = os.path.expanduser('~')
  bin_dir = os.path.join(
      home, '.atom', 'packages', 'platformio-ide', 'penv', 'bin')
  if os.path.isdir(bin_dir):
    os.environ['PATH'] += os.pathsep + bin_dir

  try:
    return do(args)
  except subprocess.CalledProcessError as e:
    return e.returncode


if __name__ == '__main__':
  sys.exit(main())
