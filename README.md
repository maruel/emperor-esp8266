# A pretty expensive chair and a pretty cheap computer

Control a [MWELab Emperor 1510](https://www.mwelab.com/) with an ESP8266.

![Emperor 1510](https://www.mwelab.com/en/images/wellbeing.jpg)

*No* internet connectivity is needed, this isn't
[InternetOfShit](https://twitter.com/internetofshit).


## Design

An ESP8266 acts as the controller between the inputs (the buttons on the side)
and the outputs (motors and LEDs).

It also optionally connects to a MQTT server and waits for commands and reports
back actions. It uses the [Homie MQTT
convention](https://github.com/homieiot/homie#convention) so you can control it
via any MQTT enabled automation service.

A similar design can be achieved via a Raspberry Pi or similar micro computer
but they take longer to boot, which can be a small annoyance. Also there's
higher expectations about keeping these systems up to date. On the other hand,
these are powerful enough to only communicate over TLS/SSL.


![WeMos D1
mini](https://wiki.wemos.cc/_media/products:d1:d1_mini_v3.0.0_3_16x9.jpg)


## Steps

This is the exhaustive list, with this you shouldn't forget anything:

1. Acquire the hardware listed below.
1. Setup the software listed below.
1. Flash the WeMos.
1. Put the WeMos on a breadboard or immediately solder 13 wires (5x outputs, 5x
   inputs, 1x ground, 1x 3.3V, 1x 5V).
1. Confirm its operation with a multimeter by toggling inputs and looking at
   outputs reaction.
1. (optional) If setting up MQTT, confirm it works where the WeMos sends updates
   for the button, and the outputs toggle for a few seconds.
1. Open the Emperor 1510 by removing the side panels then the seat.
1. Remove the previous controller in the Emperor 1510.
1. Cut the 4x motor wires.
1. Cut the 6x buttons and LED wires.
1. Connect the 7x wires (GPIOs) to the relay (4x GPIO outputs, 1x ground, 1x
   3.3V, 1x 5V).
1. Connect the 5V USB to main and to the WeMos. This powers up the WeMos.
1. Connect the 6x wires (5x GPIOs, 1x ground) from the boutons to the WeMos.
1. Confirm the buttons via MQTT if possible.
1. Disconnect the WeMos power.
1. Connect the 4x motor wires to the HL-54S relays on the middle pin.
1. Connect the 8x 12V wires from the power source to the HL-54S relays.
1. Connect the seat 12V wires to power source.
1. Connect the 12V power supply to main.
1. Confirm seat operation.
1. Reconnect the WeMos to restart it.
1. TODO(maruel): Connect the LEDs; may require adapter.
1. Confirm the motors operation via buttons.
1. (optional) Confirm the motors operation via MQTT.
1. (optional) Replace the breadboard with soldered board.
  1. Confirm the motors operation via buttons.
1. Attach wires with tie-wraps to keep the inside tidy.
1. Confirm the motors operation via buttons.
1. Put back the seat then the side panels.
1. Feel very proud of yourself.


## Hardware

### Requirements

- 1x [Emperor 1510](https://www.mwelab.com/en/wheretobuy.html) (>5000$USD)
- 1x [WeMos D1 mini](https://wiki.wemos.cc/products:d1:d1_mini) (4$USD) or any
  ESP8266 based board. One with a micro USB port is recommended.
  - A Raspberry Pi can be used.
- 1x 12V 10A power supply (25$USD)
- 1x 5V USB charger 1A
- 1x 5.5mm connector for 12V.
- 30cm of #18 wire for 12V 5A.
- 2x Twist-on wire connector.
- 1x USB wire.
- 2x 120V power plugs, or a way to solder them together to connect to the
	Emperor's main.
- 4x Relays coupled with octocouplers, e.g. HL-54S. (10$USD)
- 1x Mini breadboard
  [SYD-170](https://www.aliexpress.com/wholesale?SearchText=syb-170) (0.38$USD
  each).
- 13x [Breadboard
  wires](https://www.aliexpress.com/wholesale?SearchText=140+breadboard+wires&SortType=price_asc)
  (1.75$USD) or whatever wire you have around.
- TODO(maruel): Something to drive the system's LEDs via the Computer, like a
  74AHCT125.
- (optional) 1x Computer to run Mosquitto; a Raspberry Pi or your workstation
  will do just fine.
  - The system can run without internet.
- Few tie-wraps.


### Power ⚡

The system operates at 4 different voltages:

- 120V main @ 5A
  - Sound system.
  - Input for 12V and 5V.
  - Used for internal wiring to power the monitors.
- 12V power supply @ 10A
	- The Emperor 1510 motors need 12V at 5A. The more the safer.
  - Seat heat and fan.
- 5V via USB @ 1A
  - To power the WeMos D1 Mini board.
  - Relays board.
- 3.3V via DC-DC converter on WeMos D1 Mini @ 100mA
	- That's the level at which the GPIO run.


### GPIOs

Nearly all the connectors of the WeMos are used.

- Outputs: 5
  - 2x 2 motors
  - 1x LED driver + 5V.
- Inputs: 5
  - 1x light
  - 2x 2 motors


### Wiring

Quick summary:

1. 4x GPIO Computer -> HL-54S
2. 1x Power 5V (from Computer) -> HL-54S
3. 5x Power 12V -> HL-54S
4. 4x Power HL-54S -> Moteurs
5. 5x GPIO Buttons -> Computer
6. 1x (TODO: needs driver) Computer -> LED


## Software


### [PlatformIO](http://platformio.org)

##### CLI

This is faster, if you plan to use your own text editor.

Run `pip install --user --upgrade platformio`


#### IDE

1. Install [Atom](https://atom.io).
2. Open Atom. In _Settings_, _Install_, type _platformio_ in the box and choose
   _platformio-ide_.
3. Click 'Rebuild' on the little box on the very bottom right.


### Mosquitto

This is optional. Only needed if you setup Wifi on the WeMos and want to control
the chair remotely or log operations.

1. On Ubuntu 16.04+ or Debian Jessie, run: `sudo apt install mosquitto`. On
   other OSes get it from https://mosquitto.org/download.
2. Run the following to enable Websocket on port 9001 (adapt for other OSes):

```
cat << 'EOF' | sudo tee /etc/mosquitto/conf.d/ws.conf
listener 1883
protocol mqtt

listener 9001
protocol websockets
EOF
sudo mosquitto_passwd /etc/mosquitto/passwd homie
sudo systemctl restart mosquitto
```

**⚠:** anyone on the local network and see and inject commands. The ESP8266 is
not powerful enough to do TLS/SSL reliably.


### Flashing

1. Connect an ESP8266 via USB.
2. (optional) Copy `config_sample.json` to `data/homie/config.json` and [edit as
   documented](https://homieiot.github.io/homie-esp8266/docs/2.0.0/configuration/json-configuration-file/):
   - device name and id
   - Wifi SSID and password
   - MQTT server host name, port, user and password
   - This is not required, as you can do it manually via the web UI as explained
     below.
3. Run: `./flash_all.sh` at a bash prompt.

The ESP8266 takes less than a second to boot.


### Manual device configuration

If `data/homie/config.json` settings didn't work out, the blue LED will be solid
blue as the ESP8266 will boot in configuration mode. Configure it manually with
the web UI:

1. Connect to the Wifi `emperor-xxx`. The password is the `xxx` portion of the
   SSID.
2. Open a Web browser and navigate to http://192.168.123.1.
3. Follow the instructions.
   - Configure the wifi to use
   - Configure the MQTT server to use
   - Use a friendly name like "Emperor" and a device name like "emperor".
   - Enable OTA.
4. At this point the LED will go slow blink, rapid blink then will turn off.

See [Homie
documentation](https://homieiot.github.io/homie-esp8266/docs/2.0.0/quickstart/getting-started/#connecting-to-the-ap-and-configuring-the-device)
for more details.


## Debugging

- Use mosquotto-clients on linux to confirm the server works.

    ```
    mosquitto_sub -h <host> -p 1883 -u <user> -P <password> -v -t homie/#
    mosquitto_pub -h <host> -p 1883 -u <user> -P <password> -t homie/bar -m baz
    ```

- Use the web interface that is exposed on the ESP 8266.


## Upgrade

Homie allows OTA via MQTT:

```
git clone https://github.com/homieiot/homie-esp8266
python homie-esp8266/scripts/ota_updater/ota_updater.py -l <host> -p 1883 -u <user> -d <password> -i emperor path/to/emperor-esp8266/.pioenvs/d1_mini/firmware.bin
```

Warning: for an unknown reason, OTA doesn't work if the device was flashed over
Serial and not rebooted since then. Make sure to reset the device in-between.


## Contributing

I gladly accept contributions via GitHub pull requests, as long as the author
has signed the Google Contributor License.

Before we can use your code, you must sign the [Google Individual Contributor
License Agreement](https://cla.developers.google.com/about/google-individual)
(CLA), which you can do online. The CLA is necessary mainly because you own the
copyright to your changes, even after your contribution becomes part of our
codebase, so we need your permission to use and distribute your code. We also
need to be sure of various other things—for instance that you'll tell us if you
know that your code infringes on other people's patents. You don't have to sign
the CLA until after you've submitted your code for review and a member has
approved it, but you must do it before we can put your code into our codebase.
Before you start working on a larger contribution, you should get in touch with
us first through the issue tracker with your idea so that we can help out and
possibly guide you. Coordinating up front makes it much easier to avoid
frustration later on.
