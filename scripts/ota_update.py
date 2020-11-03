#!/usr/bin/env python3
# Copyright 2020 Marc-Antoine Ruel. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Updates a homie device via MQTT."""

import argparse
import hashlib
import logging
import os
import subprocess
import sys

import attr
import paho.mqtt.client as mqtt

THIS_DIR = os.path.dirname(os.path.abspath(__file__))


@attr.s
class Updater(object):
  # Required arguments:
  base_topic = attr.ib(type=str)
  device_id = attr.ib(type=str)
  firmware = attr.ib(type=str)

  # Automatically calculated:
  md5 = attr.ib(type=str, init=False)

  # From the device:
  published = attr.ib(type=bool, default=False)
  ota_enabled = attr.ib(type=bool, default=False)
  old_md5 = attr.ib(type=str, default='')

  def __attr_post_init__(self):
    """Calculate firmware md5."""
    self.md5 = hashlib.md5(self.firmware).hexdigest()

  def setup_and_connect(self, c, host, port):
    """Setups and start the process."""
    # Convert hooks to methods:
    c.user_data_set(self)
    c.on_connect = lambda client, userdata, flags, rc: userdata._on_connect(
        client, flags, rc)
    c.on_message = lambda client, userdata, msg: userdata._on_message(
        client, msg)
    logging.info('Connecting to mqtt broker {} on port {}'.format(host, port))
    c.connect(host, port, 60)
    # Blocking call that processes network traffic, dispatches callbacks and
    # handles reconnecting.
    c.loop_forever()

  @property
  def topic(self):
    return self.base_topic + self.device_id

  def _on_connect(self, client, flags, rc):
    """On CONNACK."""
    if rc != 0:
      logging.warning('Connection Failed with result code {}'.format(rc))
      client.disconnect()
    logging.debug('Subscribing to {}/$online'.format(self.topic))
    client.subscribe('{}/$online'.format(self.topic))
    logging.info(
        'Waiting for device {} to come online...'.format(self.device_id))

  def _on_message(self, client, msg):
    """On a PUBLISH message."""
    payload = msg.payload.decode('utf-8')
    if msg.topic.endswith('$implementation/ota/status'):
      status = int(payload.split()[0])
      if self.published:
        if status == 206: # In progress
          # State in progress, print progress bar.
          progress, total = [int(x) for x in payload.split()[1].split('/')]
          bar_width = 30
          bar = int(bar_width*(progress/total))
          logging.info('\r[', '+'*bar, ' '*(bar_width-bar), '] ', payload.split()[1], end='', sep='')
          if progress == total:
              print()
          sys.stdout.flush()
        elif status == 304: # Not modified
          logging.info(
              'Device firmware already up to date with md5 checksum: {}'.format(
                self.md5))
          client.disconnect()
        elif status == 403: # Forbidden
          logging.error('Device ota disabled, aborting...')
          client.disconnect()

    elif msg.topic.endswith('$fw/checksum'):
      if self.published:
        if payload == self.md5:
          logging.info('Device back online. Update Successful!')
        else:
          logging.error(
              'Expecting checksum {}, got {}, update failed!'.format(
                self.md5, payload))
        # We're done!
        client.disconnect()
      else:
        if payload != self.md5:
          # Save old md5 for comparison with new firmware.
          self.old_md5 = payload
        else:
          logging.info(
              'Device firmware already up to date with md5 checksum: {}'.format(
                payload))
          client.disconnect()

    elif msg.topic.endswith('ota/enabled'):
      if payload == 'true':
        self.ota_enabled = True
      else:
        logging.error('Device ota disabled, aborting...')
        client.disconnect()

    elif msg.topic.endswith('$online'):
      if payload == 'false':
        logging.error('Device is offline.')
        return
      # Subscribing in on_connect() means that if we lose the connection and
      # reconnect then subscriptions will be renewed.
      client.subscribe('{}/$implementation/ota/status'.format(self.topic))
      client.subscribe('{}/$implementation/ota/enabled'.format(self.topic))
      client.subscribe('{}/$fw/#'.format(self.topic))
      # Wait for device info to come in and invoke the on_message callback where
      # update will continue.
      logging.debug('Waiting for device info...')

    if (not self.published and self.ota_enabled and
        self.old_md5 and self.md5 != self.old_md5):
      logging.debug('Publishing new firmware with checksum {}'.format(self.md5))
      self.published = True
      topic = '{}/$implementation/ota/firmware/{}'.format(self.topic, self.md5)
      client.publish(topic, self.firmware)


def get_mqtt_client(host, port, username, password, ca_cert):
  """Returns a MQTT client."""
  client = mqtt.Client()
  if username and password:
    client.username_pw_set(username, password)
  if ca_cert is not None:
    client.tls_set(ca_certs=ca_cert)
  return client


def main():
  firmware = os.path.join(os.path.dirname(THIS_DIR), '.pio', 'build', 'd1_mini', 'firmware.bin')
  parser = argparse.ArgumentParser(description=sys.modules[__name__].__doc__)
  parser.add_argument('-v', '--verbose', action='store_true', help='Verbose logging')
  parser.add_argument(
      '--host',
      metavar='127.0.0.1',
      default='127.0.0.1',
      help='host name or ip address of the MQTT broker')
  parser.add_argument(
      '--port', type=int,
      metavar=1883,
      default=1883,
      help='MQTT broker port')
  parser.add_argument(
      '--username',
      help='username used to authenticate with the MQTT broker')
  parser.add_argument(
      '--password',
      help='password used to authenticate with the MQTT broker')
  # Ensures base topic always ends with a '/'.
  parser.add_argument(
      '--base-topic',
      type=lambda s: s if s.endswith('/') else s + '/',
      metavar='homie/',
      default='homie/',
      help='base topic of the homie devices')
  parser.add_argument(
      '--device-id', required=True, help='homie device id')
  parser.add_argument(
      '--firmware', required=False,
      metavar=firmware,
      default=firmware,
      help='Path to firmware.bin')
  parser.add_argument(
      '--tls-cacert',
      help='CA certificate bundle used to validate TLS connections. If set, TLS will be enabled on the broker connection')
  # Behavior:
  parser.add_argument(
      '--no-build', action='store_true', help='Do not build first')
  parser.add_argument(
      '--query-only',
      action='store_true',
      help='Query the version but do not upgrade')
  args = parser.parse_args()
  logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)
  if not args.no_build:
    subprocess.check_call(['pio', 'run'], cwd=os.path.dirname(THIS_DIR))
  with open(args.firmware, 'rb') as f:
    data = f.read()

  c = get_mqtt_client(
      args.host, args.port, args.username, args.password, args.tls_cacert)
  u = Updater(
      base_topic=args.base_topic, device_id=args.device_id, firmware=data)
  u.setup_and_connect(c, args.host, args.port)
  return 0


if __name__ == '__main__':
  sys.exit(main())

