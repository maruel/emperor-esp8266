// Copyright 2016 Marc-Antoine Ruel. All rights reserved.
// Use of this source code is governed under the Apache License, Version 2.0
// that can be found in the LICENSE file.

'use strict';

var client;
var device;

function getParameterByName(name) {
  var regex = new RegExp('[?&]' + name.replace(/[\[\]]/g, '\\$&') + '(=([^&#]*)|&|#|$)');
  var results = regex.exec(window.location.href);
  if (!results) return null;
  if (!results[2]) return '';
  return decodeURIComponent(results[2].replace(/\+/g, ' '));
}

function onLoad() {
  var host = getParameterByName('host');
  var port = getParameterByName('port');
  device = getParameterByName('device');
  if (!host || !port) {
    document.getElementById('state').innerText = 'Please provide query arguments "host" and "port" to the MQTT server';
    return;
  }
  // https://www.eclipse.org/paho/files/jsdoc/symbols/Paho.MQTT.Client.html
  client = new Paho.MQTT.Client(host, Number(port), '/', 'emperor-esp8266-web');
  client.onConnectionLost = onConnectionLost;
  client.onMessageArrived = onMessageArrived;
  connect();
}

function connect() {
  var user = getParameterByName('user');
  var opts = {onSuccess: onConnect, onFailure: onFailure};
  if (user) {
    opts.userName = user;
    var password = getParameterByName('password');
    if (password) {
      opts.password = password;
    }
  }
  if (getParameterByName('useSSL')) {
    opts.useSSL = true;
  }
  client.connect(opts);
}

function onConnect() {
  document.getElementById('state').innerText = 'Connected to ' + client.host + ':' + client.port;
  if (getParameterByName('all')) {
    client.subscribe('#');
  } else {
    client.subscribe('homie/+/$localip');
    client.subscribe('homie/+/+/on');
    client.subscribe('homie/+/+/pwm');
    client.subscribe('homie/+/+/freq');
    //client.subscribe('homie/+/$implementation/config');
  }
}

function onConnectionLost(responseObject) {
  if (responseObject.errorCode !== 0) {
    document.getElementById('state').innerText = 'Connection lost: ' + responseObject.errorMessage;
    window.setTimeout(connect, 1000);
  }
}

function onFailure(context) {
  document.getElementById('state').innerText = 'Connection failure(' + context.errorCode + '): ' + context.errorMessage;
  window.setTimeout(connect, 1000);
}

function set_monitors(v) {
  sendDevMsg('monitors/direction/set', v);
}

function set_seat(v) {
  sendDevMsg('seat/direction/set', v);
}

function onLED() {
  sendDevMsg('led/on/set', document.getElementById('LED').checked.toString());
}

function sendDevMsg(topic, payload) {
  // TODO(maruel): Set a warning otherwise.
  if (device) {
    sendMsg('homie/' + device + '/' + topic, payload);
  }
}

function sendMsg(topic, payload) {
  var message = new Paho.MQTT.Message(payload);
  message.destinationName = topic;
  client.send(message);
  console.log('Sent: ' + topic + ': ' + payload);
}

function onMessageArrived(message) {
  // TODO(maruel): As new devices are discovered, change UI to control multiple
  // chairs.
  // TODO(maruel): React to the messages.
  document.getElementById('messages').innerText += message.destinationName + ': ' + message.payloadString + '\n';
}
