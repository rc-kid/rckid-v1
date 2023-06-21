# RCKid Remote Protocol

A simple and extensible protocol that allows the RCKid to use its onboard NRF radio to remotely control various devices. The RCKid acts as a _remote_ that can control an arbitrary number of _devices_. Each device has a number of _channels_ into which all the device's functionality has to be encoded. The remote protocol supports pairing, querying devices and their channel capabilities and so on. 

> Note that the protocol is very simple and its main purpose is *not* security. 

## Channels

A client device is described by a list of channels. Each channel has a specific type and consists of the following distinct parts:

- _control_, which is information the remote communicates regularly to the device that alters the state of the channel (such as motor speed)
- _feedback_, which is information the device communicates to the remote, such as current consumption
- _config_, which consists of channel configuration that is not expected to be changed once the remote and the device are paired, such as the motor's current limit

Each part of the channel can be empty, depending on the channel type. Channels are identified by numbers from `1` to `255` inclusive. The following channel types are supported:

### Motor channel

The motor channel provides control for single DC brushed motor attached to an H bridge. It consists



### CustomIO

A configurable channel that can fit multiple purposes. Corresponds to a single wire and various digital/analog functions it may have, namely:

Mode              | Control   | Feedback | Config
------------------|-----------|----------| -------------
Digital Out       | 0/1       |          | 00
Digital In        |           | 0/1      | 01
Analog Out (DAC)  | 0 - 255   |          | 02
Analog In (ADC)   |           | 0 - 255  | 03
PWM               | 0 - 255   |          | 04
Servo             | 0 - 65535 |          | 05 iiii xxxx
Tone              | 0 - 65535 |          | 06

### ToneEffect



### RGB Strip

The RGB strip channel is designed to configure and control a strip of RGB colors such as neopixels. The config part of the strip is the index of the first RGB channel specifying the color and the length of the strip. The length of the strip should not exceed the number of RGB colors available. The is no feedback and the channel control is the state of the strip, which can be one of:

ID   | Meaning
-----|-------------------
`00` | Manual control - the strip will display exactly the colors specified in the RGB channels
`..` | Effect - the number specifies which effect to use

### RGB Color

Single RGB color. Its control cosists of the three RGB bytes. No feedback or config is possible. The purpose of the RGB color channel is to simply hold memory for one neopixel. The device itself and the RGBStrip channel's config the determine which of these colors will be used. 

## Communication




## Commands

By default the remote sends the device channel control information and the device sends back the channel feedback info when there is change. Instead of sending directly the channel data, the remote may choose to send one of the predfined commands instead:

`00 00` - Heartbeat

`00 01 xx` - Read feedback of channel `xx`

`00 02 xx` - Read control of channel `xx`


    