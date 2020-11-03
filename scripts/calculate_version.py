#!/usr/bin/env python3
# Copyright 2020 Marc-Antoine Ruel. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Calculates a version based on the current git checkout.

Can be imported, to call calculate_version() directly.
"""

import argparse
import getpass
import os
import string
import subprocess
import sys


def git(cmd):
  env = os.environ.copy()
  env['LANG'] = 'en_US.UTF-8'
  return subprocess.check_output(['git'] + cmd, env=env).rstrip().decode('utf-8')


def is_pristine(mergebase):
  """Returns True if there's any local change."""
  head = git(['rev-parse', 'HEAD'])
  if head != mergebase:
    return False
  return not (
      git(['diff', '--ignore-submodules=none', mergebase]) or
      git(['diff', '--ignore-submodules', '--cached', mergebase]) or
      git(['status', '-s', '--porcelain=v2']))


def get_pseudo_version(cur, upstream):
  """Counts the number of commits since the oldest root, and returns the merge
  base found in the upstream reference.
  """
  mergebase = git(['merge-base', cur, upstream])
  # This doesn't scale well to repositories with very large (millions) of
  # commits, but for reasonable repositories it's fine to enumerate every single
  # time.
  history = git(['rev-list', '--topo-order', '--parents', '--reverse', mergebase]).splitlines()
  key = mergebase + ' '
  for i, line in enumerate(history):
    if line.startswith(key):
      return i, mergebase
  return 'Unknown', mergebase


def calculate_version(tag):
  pseudo_revision, mergebase = get_pseudo_version('HEAD', 'origin/master')
  pristine = is_pristine(mergebase)
  if not pristine and not tag:
    branch = git(['rev-parse', '--abbrev-ref', 'HEAD']).strip()
    if branch != 'HEAD':
      tag = branch
  version = '%s-%s' % (pseudo_revision, mergebase[:7])
  # 25 characters is already too much.
  #if not pristine:
  #  version += '-' + getpass.getuser()
  #if tag:
  #  version += '-' + tag
  return version


def main():
  parser = argparse.ArgumentParser(description=sys.modules[__name__].__doc__)
  parser.add_argument(
      '-t', '--tag', help='Tag to attach to a tainted version')
  args = parser.parse_args()
  os.chdir(git(['rev-parse', '--show-toplevel']))
  print(calculate_version(args.tag))
  return 0


if __name__ == '__main__':
  sys.exit(main())
