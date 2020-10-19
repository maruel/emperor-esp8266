#!/usr/bin/env python3
# Copyright 2020 Marc-Antoine Ruel. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Prints out details about a homie firmware."""

import argparse
import os
import re
import sys

THIS_DIR = os.path.dirname(os.path.abspath(__file__))


class Sad(Exception):
  pass


def extract_metadata(data):
  re_homie = re.compile(b'\x25\x48\x4f\x4d\x49\x45\x5f\x45\x53\x50\x38\x32\x36\x36\x5f\x46\x57\x25')
  re_name = re.compile(b'\xbf\x84\xe4\x13\x54(.+)\x93\x44\x6b\xa7\x75')
  re_version = re.compile(b'\x6a\x3f\x3e\x0e\xe1(.+)\xb0\x30\x48\xd4\x1a')
  re_brand = re.compile(b'\xfb\x2a\xf5\x68\xc0(.+)\x6e\x2f\x0f\xeb\x2d')

  match_name = re_name.search(data)
  match_version = re_version.search(data)
  if not re_homie.search(data) or not match_name or not match_version:
    raise Sad('Not a valid Homie firmware')
  match_brand = re_brand.search(data)
  name = match_name.group(1).decode('utf-8')
  version = match_version.group(1).decode('utf-8')
  brand = match_brand.group(1).decode('utf-8') if match_brand else 'unset (default is Homie)'
  return name, version, brand


def main():
  firmware = os.path.join(os.path.dirname(THIS_DIR), '.pio', 'build', 'd1_mini', 'firmware.bin')
  parser = argparse.ArgumentParser(description=sys.modules[__name__].__doc__)
  parser.add_argument(
      '--firmware', type=argparse.FileType('rb'), required=False,
      metavar=firmware,
      default=firmware,
      help='Path to firmware.bin')
  args = parser.parse_args()
  data = args.firmware.read()
  args.firmware.close()
  try:
    name, version, brand = extract_metadata(data)
  except Sad as e:
    print('Error: {0}.'.format(e.args[0]), file=sys.stderr)
    return 1
  print('Name: {0}'.format(name))
  print('Version: {0}'.format(version))
  print('Brand: {0}'.format(brand))
  return 0


if __name__ == '__main__':
  sys.exit(main())
