#!/usr/bin/env python3
# Copyright 2020 Marc-Antoine Ruel. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Updates a homie device firmware via MQTT."""

# See
# https://homieiot.github.io/homie-esp8266/docs/3.0.0/others/ota-configuration-updates/
# and
# https://pypi.org/project/paho-mqtt/

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
  firmware = attr.ib(type=bytes)

  # Automatically calculated:
  md5 = attr.ib(type=str, init=False)

  # From the device:
  published = attr.ib(type=bool, default=False)
  ota_enabled = attr.ib(type=bool, default=False)
  old_md5 = attr.ib(type=str, default='')

  def __attrs_post_init__(self):
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
    logging.info('Connecting to mqtt broker %s on port %s', host, port)
    c.connect(host, port, 60)
    # Blocking call that processes network traffic, dispatches callbacks and
    # handles reconnecting.
    c.loop_forever()

  @property
  def topic(self):
    return self.base_topic + self.device_id

  def _subscribe(self, client, topic):
    t = self.topic + '/' + topic
    logging.debug('subscribe(%s)', t)
    client.subscribe(t)

  def _publish(self, client, topic, payload):
    t = self.topic + '/' + topic
    logging.debug('publish(%s, %d bytes)', t, len(payload))
    resp = client.publish(t, payload)
    logging.debug('-> sending %s', resp.mid)

  def _on_connect(self, client, flags, rc):
    """On CONNACK."""
    if rc != 0:
      logging.warning('Connection Failed with result code %s', rc)
      client.disconnect()
    self._subscribe(client, '$state')
    logging.info('Waiting for device %s to come online...', self.device_id)

  def _on_message(self, client, msg):
    """On a PUBLISH message."""
    payload = msg.payload.decode('utf-8')
    assert msg.topic.startswith(self.topic + '/')
    topic = msg.topic[len(self.topic)+1:]
    logging.debug('on_message(%s, %s)', topic, payload)

    if topic == '$implementation/ota/status':
      status = int(payload.split()[0])
      if self.published:
        if status == 206: # In progress
          # State in progress, print progress bar.
          progress, total = [int(x) for x in payload.split()[1].split('/')]
          bar_width = 30
          bar = int(bar_width*(progress/total))
          print('\r[', '+'*bar, ' '*(bar_width-bar), '] ', payload.split()[1], end='', sep='')
          if progress == total:
              print('')
          sys.stdout.flush()
        elif status == 304: # Not modified
          logging.info(
              'Device firmware already up to date with md5 checksum: %s',
              self.md5)
          client.disconnect()
        elif status == 403: # Forbidden
          logging.error('Device ota disabled, aborting...')
          client.disconnect()

    elif topic == '$fw/checksum':
      if self.published:
        if payload == self.md5:
          logging.info('Device back online. Update Successful!')
        else:
          logging.error(
              'Expecting checksum %s, got %s, update failed!', self.md5,
              payload)
        # We're done!
        client.disconnect()
      else:
        if payload != self.md5:
          logging.info('Received current firmware checksum: %s', payload)
          self.old_md5 = payload
        else:
          logging.info(
              'Device firmware already up to date with md5 checksum: %s',
              payload)
          client.disconnect()

    elif topic == '$implementation/ota/enabled':
      if payload == 'true':
        self.ota_enabled = True
      else:
        logging.error('Device ota disabled, aborting...')
        client.disconnect()

    elif topic == '$state':
      if payload == 'lost':
        logging.error('Device is offline.')
        client.disconnect()
        return
      if payload == 'init':
        logging.info('Device is initializing. Waiting')
        return
      # Subscribing in on_connect() means that if we lose the connection and
      # reconnect then subscriptions will be renewed.
      self._subscribe(client, '$implementation/ota/status')
      self._subscribe(client, '$implementation/ota/enabled')
      self._subscribe(client, '$fw/#')
      # Wait for device info to come in and invoke the on_message callback where
      # update will continue.
      logging.debug('Waiting for device info...')

    if (not self.published and self.ota_enabled and
        self.old_md5 and self.md5 != self.old_md5):
      #logging.debug('Publishing new firmware with checksum %s', self.md5)
      topic = '$implementation/ota/firmware/{}'.format(self.md5)
      self._publish(client, topic, self.firmware)
      logging.debug('Done, waiting for device to react')
      self.published = True


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
  group = parser.add_argument_group('MQTT connection')
  group.add_argument(
      '--host',
      metavar='127.0.0.1',
      default='127.0.0.1',
      help='host name or ip address of the MQTT broker')
  group.add_argument(
      '--port', type=int,
      metavar=1883,
      default=1883,
      help='MQTT broker port')
  group.add_argument(
      '--username',
      help='username used to authenticate with the MQTT broker')
  group.add_argument(
      '--password',
      help='password used to authenticate with the MQTT broker')
  group.add_argument(
      '--tls-cacert', metavar='CERT',
      help='CA certificate bundle used to validate TLS connections. If set, TLS will be enabled on the broker connection')

  # Ensures base topic always ends with a '/'.
  parser.add_argument(
      '--base-topic',
      type=lambda s: s if s.endswith('/') else s + '/',
      metavar='homie/',
      default='homie/',
      help='base topic of the homie devices')
  parser.add_argument(
      '--device-id', metavar='DEV', required=True, help='homie device id')
  parser.add_argument(
      '--firmware', required=False,
      metavar=firmware,
      default=firmware,
      help='Path to firmware.bin')
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
  # TODO(maruel): Other useful things:
  # '$implementation/reset' = 'true'
  # '$implementation/config/set'
  return 0


if __name__ == '__main__':
  sys.exit(main())

