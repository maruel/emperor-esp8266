# Pretty expensive chair + Pretty cheap computer

Control a [MWELab Emperor 1510](https://www.mwelab.com/) with an ESP8266.

![Emperor 1510](https://raw.githubusercontent.com/wiki/maruel/emperor-esp8266/wellbeing.jpg)

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

A WeMos D1 Mini:

![WeMos D1
mini](https://raw.githubusercontent.com/wiki/maruel/emperor-esp8266/d1_mini_v3.0.0_3_16x9.jpg)


The Web UI:

![Web
UI](https://raw.githubusercontent.com/wiki/maruel/emperor-esp8266/web_ui.png)


## Hardware

### Requirements

- 1x [Emperor 1510](https://www.mwelab.com/en/wheretobuy.html) (>5000$USD)
- 1x [WeMos D1 mini](https://www.wemos.cc/en/latest/d1/d1_mini.html) (4$USD) or any
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
  each) or a blank PCB with holes.
- 13x [Breadboard
  wires](https://www.aliexpress.com/wholesale?SearchText=140+breadboard+wires&SortType=price_asc)
  (1.75$USD) or whatever wire you have around.
- (optional): DIN 8 pin female socket.
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


### Wiring

![Schematics](https://raw.githubusercontent.com/wiki/maruel/emperor-esp8266/schematics.png)

Confirm with [main.cpp](src/main.cpp).


## Software

- Python 3.
- [PlatformIO](http://platformio.org) (optional)
  - `./setup.sh` installs a local version in virtualenv.
- Mosquitto (optional)
  - Only needed if you setup Wifi on the WeMos and want to control the chair
    remotely or log operations.
  - `sudo apt install mosquitto` on Ubuntu or Debian.
  - Run the following to enable Websocket on port 9001 (adapt for other OSes):
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


## Flashing

1. Connect the ESP8266 via USB.
2. Copy `config_sample.json` to `data/homie/config.json` and [edit as
   documented](https://homieiot.github.io/homie-esp8266/docs/2.0.0/configuration/json-configuration-file/):
   - device name and id
   - Wifi SSID and password
   - MQTT server host name, port, user and password (optional)
   - Another option is to use the [manual configuration
     mode](https://homieiot.github.io/homie-esp8266/docs/2.0.0/quickstart/getting-started/#connecting-to-the-ap-and-configuring-the-device)
     but I don't recommend it.
3. Run: `./flash_all.sh` at a bash prompt.

The ESP8266 takes less than a second to boot.

## OTA

Over the air update can be done with:

```
./scripts/ota_update.py \
    --device-id emperor-1510 \
    --host <mqttserver> \
    --user <mqttuser> \
    --password <mqttpassword>
```

## Debugging

- Use mosquotto-clients on linux to confirm the server works.
    ```
    mosquitto_sub -h <host> -p 1883 -u <user> -P <password> -v -t homie/#
    mosquitto_pub -h <host> -p 1883 -u <user> -P <password> -t homie/bar -m baz
    ```
- Use the web interface that is exposed on the ESP 8266.
- Uncomment `build_flags=-D LOG_SERIAL=1` in [platform.ini](platform.ini), flash
  again then run to monitor the serial log over USB:
    ```
    platformio device monitor
    ```

## Upgrade

Homie allows OTA via MQTT:

```
./do.py --log-serial mqtt -H <host>
```

Warning: for an unknown reason, OTA doesn't work if the device was flashed over
serial and not rebooted since then. Make sure to reset the device in-between.


## Steps

This is the exhaustive list, with this you (read: I) shouldn't forget anything:

1. Acquire the hardware.
1. Setup the software.
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
1. Cut the 6x buttons and LED wires or solder a DIN-8 female socket.
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
