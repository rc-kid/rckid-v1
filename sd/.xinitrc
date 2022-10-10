#!/bin/bash
xset s off         # don't activate screensaver
xset -dpms         # disable DPMS (Energy Star) features.
xset s noblank     # don't blank the video device
cd /home/pi/rcboy && sudo build/rcboy/rpi/rcboy
