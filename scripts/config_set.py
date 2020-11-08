#!/usr/bin/env python3
# Copyright 2020 Marc-Antoine Ruel. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

"""Updates a homie device config via MQTT."""

# See
# https://homieiot.github.io/homie-esp8266/docs/3.0.0/others/ota-configuration-updates/

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
class Setter(object):
  # Required arguments:
  base_topic = attr.ib(type=str)
  device_id = attr.ib(type=str)

  config_sent = attr.ib(type=bool, default=False)

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
    client.publish(t, payload)

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
    if topic != '$state':
      assert False, topic
    if payload == 'lost':
      if not self.reboot_sent:
        logging.error('Device is offline, can\'t reboot')
        client.disconnect()
      else:
        logging.info('Device disconnected. Waiting')
    elif payload == 'init':
      logging.info('Device is initializing. Waiting')
    elif payload == 'ready':
      if not self.reboot_sent:
        self._publish(client, '$implementation/reboot', 'true')
        self.reboot_sent = True
      else:
        logging.info('Device rebooted. Done')
        client.disconnect()
    else:
      assert False, payload


def get_mqtt_client(host, port, username, password, ca_cert):
  """Returns a MQTT client."""
  client = mqtt.Client()
  if username and password:
    client.username_pw_set(username, password)
  if ca_cert is not None:
    client.tls_set(ca_certs=ca_cert)
  return client


def main():
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
  args = parser.parse_args()
  logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)
  c = get_mqtt_client(
      args.host, args.port, args.username, args.password, args.tls_cacert)
  s = Setter(base_topic=args.base_topic, device_id=args.device_id)
  s.setup_and_connect(c, args.host, args.port)
  return 0


if __name__ == '__main__':
  sys.exit(main())

