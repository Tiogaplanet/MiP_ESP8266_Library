# MiP ESP8266 Library
![MiP Photo](https://github.com/WowWeeLabs/MiP-BLE-Protocol/blob/master/Images/MiP.png)<br>

This project provides a library for the Arduino IDE and allows users to take control of [WowWee Labs'](https://github.com/WowWeeLabs/)  [MiP](https://wowwee.com/mip) and turn it into a cloud-connected robot.

MiP is a hacker friendly self-balancing robot. WowWee not only provides the [MiP Protocol Specification on GitHub](https://github.com/WowWeeLabs/MiP-BLE-Protocol), they also provide a [4-pin hacker port](https://cdn.sparkfun.com/assets/learn_tutorials/2/8/5/HackingPortAnnotated.png), complete with JST connector, right on the mainboard. This connector makes it easy to connect an external controller such as [D1 mini](https://wiki.wemos.cc/products:d1:d1_mini) or compatible boards and take control of your MiP. Once connected, you can:
* Command the speed and direction of motion for the gravity defying MiP.
* Command the individual control (on, off, blink) of the 4 LED eye segments on the head.
* Take full control of the RGB LED in MiP's chest.
* Command the playback of sound lists using the >100 built-in sounds.
* Use the head mounted IR sensors to read 'radar' distance measurements or detected user hand gestures.
* Detect user claps with the built-in microphone.
* Detect the MiP's current pose via its inertial sensors, the same sensors that make its balancing magic possible.
* And more!

Also, be sure to check out the [D1 mini Pack](https://github.com/Tiogaplanet/MiP_D1-mini-Pack) which conveniently allows you to mount a D1 mini to MiP's battery compartment.

## Acknowledgement
* This library is a port of adamgreen's [MiP_ProMini Pack](https://github.com/adamgreen/MiP_ProMini-Pack).  Here you will find all the  same functionality as his original library but with cloud-connectivity too!<br>
* JoaoLopesF developed the highly valuable [RemoteDebug](https://github.com/JoaoLopesF/RemoteDebug) library which makes debugging MiP wireless and easy.
* Without esp8266's [Arduino](https://github.com/esp8266/Arduino) library, none of this would be possible.

## Installation
1. The esp8266 [Arduino](https://github.com/esp8266/Arduino) library should already be installed prior to using this library.

2. The MiP_ESP8266_Library is intended for use with the Arduino IDE.  Installation is the same as for other libraries.  Download the zip and select `Sketch->Include Library->Add .ZIP Library...`.  Browse to the downloaded zip file and the Arduino IDE will do the rest.

3. The MiP_ESP8266_Library requires JoaoLopesF's [RemoteDebug](https://github.com/JoaoLopesF/RemoteDebug) library.  Install it too using the same process described for installing the MiP_ESP8266_Library.

## Usage
A very thorough guide to using the MiP_ESP8266_Library is provided in the [wiki](https://github.com/Tiogaplanet/MiP_ESP8266_Library/wiki).

## Contributing
This project is intended to make programming MiP easy and fun.  To that end, contributions are highly encouraged!  Please see [CONTRIBUTING.md](https://github.com/Tiogaplanet/MiP_ESP8266_Library/blob/master/CONTRIBUTING.md) for more information.
