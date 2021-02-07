# Expensive chair + Cheap computer

Control a [MWELab Emperor 1510](https://www.mwelab.com/) with an ESP8266 running
[ESPHome](https://esphome.io).


## Design

An ESP8266 acts as the controller between the inputs (the buttons on the side)
and the outputs (actuators). It has local automations so it works standalone.

*No* internet connectivity is needed.

It integrates seamlessly in [Home Assistant](https://www.home-assistant.io/) so
automation with other devices, e.g. lights can be done.

![Demo](https://raw.githubusercontent.com/wiki/maruel/emperor-esp8266/demo.gif)

The dashboard looks like this:

![Home Assistant integration](https://raw.githubusercontent.com/wiki/maruel/emperor-esp8266/homeassistant.png)

The physical monitor buttons are sticky; once pressed the actuator goes all the
way up/down. This is particularly useful when getting inside the chair and
pressing the button to move the monitors down, without having to hold the
button.

The seat buttons are not sticky; the actuator only runs while the button is
pressed.

### Wiring

![Schematics](https://raw.githubusercontent.com/wiki/maruel/emperor-esp8266/schematics_v2.png)

### The chair

![Emperor 1510](https://raw.githubusercontent.com/wiki/maruel/emperor-esp8266/wellbeing.jpg)
