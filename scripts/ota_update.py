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

import paho.mqtt.client as mqtt

THIS_DIR = os.path.dirname(os.path.abspath(__file__))


def _on_connect(client, userdata, flags, rc):
  """On CONNACK."""
  if rc != 0:
    logging.warning('Connection Failed with result code {}'.format(rc))
    client.disconnect()
  else:
    logging.debug('Connected with result code {}'.format(rc))
  client.subscribe('{base_topic}{device_id}/$online'.format(**userdata))
  logging.debug('Waiting for device to come online...')


def _on_message(client, userdata, msg):
  """On a PUBLISH message."""
  payload = msg.payload.decode('utf-8')
  if msg.topic.endswith('$implementation/ota/status'):
    status = int(payload.split()[0])
    if userdata.get('published'):
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
        logging.info('Device firmware already up to date with md5 checksum: {}'.format(userdata.get('md5')))
        client.disconnect()
      elif status == 403: # Forbidden
        logging.error('Device ota disabled, aborting...')
        client.disconnect()

  elif msg.topic.endswith('$fw/checksum'):
    if userdata.get('published'):
      if payload == userdata.get('md5'):
        logging.info('Device back online. Update Successful!')
      else:
        logging.error('Expecting checksum {}, got {}, update failed!'.format(userdata.get('md5'), payload))
      # We're done!
      client.disconnect()
    else:
      if payload != userdata.get('md5'):
        # Save old md5 for comparison with new firmware.
        userdata.update({'old_md5': payload})
      else:
        logging.info('Device firmware already up to date with md5 checksum: {}'.format(payload))
        client.disconnect()

  elif msg.topic.endswith('ota/enabled'):
    if payload == 'true':
      userdata.update({'ota_enabled': True})
    else:
      logging.error('Device ota disabled, aborting...')
      client.disconnect()

  elif msg.topic.endswith('$online'):
    if payload == 'false':
      return
    # Calculate firmware md5.
    firmware_md5 = hashlib.md5(userdata['firmware']).hexdigest()
    userdata.update({'md5': firmware_md5})
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe('{base_topic}{device_id}/$implementation/ota/status'.format(**userdata))
    client.subscribe('{base_topic}{device_id}/$implementation/ota/enabled'.format(**userdata))
    client.subscribe('{base_topic}{device_id}/$fw/#'.format(**userdata))
    # Wait for device info to come in and invoke the on_message callback where
    # update will continue.
    logging.debug('Waiting for device info...')

  if (not userdata.get('published')
      and userdata.get('ota_enabled')
      and 'old_md5' in userdata
      and userdata.get('md5') != userdata.get('old_md5')):
    # Push the firmware binary.
    userdata.update({'published': True})
    topic = '{base_topic}{device_id}/$implementation/ota/firmware/{md5}'.format(**userdata)
    logging.debug('Publishing new firmware with checksum {}'.format(userdata.get('md5')))
    client.publish(topic, userdata['firmware'])


def do_ota(host, port, username, password, ca_cert, base_topic, device_id, firmware):
  client = mqtt.Client()
  client.on_connect = _on_connect
  client.on_message = _on_message
  if username and password:
    client.username_pw_set(username, password)
  if ca_cert is not None:
    client.tls_set(ca_certs=ca_cert)
  # Save data to be used in the callbacks.
  client.user_data_set(
      {
        'base_topic': base_topic,
        'device_id': device_id,
        'firmware': firmware,
      })
  logging.info('Connecting to mqtt broker {} on port {}'.format(host, port))
  client.connect(host, port, 60)
  # Blocking call that processes network traffic, dispatches callbacks and handles reconnecting.
  client.loop_forever()


def _base_topic_arg(s):
  """Ensures base topic always ends with a '/'."""
  s = str(s)
  return s if s.endswith('/') else s + '/'


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
  parser.add_argument(
      '--base-topic', type=_base_topic_arg,
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
  parser.add_argument(
      '--no-build', action='store_true', help='Do not build first')
  args = parser.parse_args()
  logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)
  if not args.no_build:
    subprocess.check_call(['pio', 'run'], cwd=os.path.dirname(THIS_DIR))
  with open(args.firmware, 'rb') as f:
    data = f.read()
  do_ota(args.host, args.port, args.username, args.password, args.tls_cacert,
      args.base_topic, args.device_id, data)
  return 0


if __name__ == '__main__':
  sys.exit(main())

