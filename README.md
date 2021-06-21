# DUTCHess
## _Device Under Test (DUT) Control Harness_
DUTCHess is a prototype device to enable you to work with your networking/electronics lab equipment remotely.

Based on the MIMXRT1020 Evaluation Kit, DUTCHess provides the following features:
 - DUT serial.
 - DUT Power switching.
 - TFTP server.
 - EEPROM update over TFTP.
 - Remote temperature sensing.
 - Configurable networking.
 - Access via telnet, USB serial, or web GUI.

## Table of contents
* [Technologies](#technologies)
  * [Hardware](#hardware)
  * [Software](#software)
* [Getting Started](#getting-started)
* [Features](#features)

## Technologies
### Hardware
DUTCHess is based on the NXP MIMXRT1020 Evaluation Kit (EVK) which has a slew of useful hardware features.  The most notable of these for this project is the networking capability, enabling DUTCHess to provide a remote endpoint from which to connect to a DUT.  Hardware required is as follows:
 - NXP MIMXRT1020 EVK
<img src="https://www.nxp.com/assets/images/en/block-diagrams/MIMXRT-1020-EVKBD2.jpg" width="600">
 - PCT2075 Arduino Shield
 <img src="https://www.nxp.com/assets/images/en/dev-board-image/PCT2075DP-ARD-ISO.jpg" width="600">
 - UART to RS232 Level Shifter Board (DE-9 Connector)
 <img src="https://imgaz2.staticbg.com/thumb/large/oaupload/ser1/banggood/images/DF/AB/3acd3e6d-b33b-4f00-b051-ab958f8b9c63.JPG.webp" width="600">
 - Isolated Relay Board
 <img src="https://www.elecrow.com/pub/media/catalog/product/cache/f8158826193ba5faa8b862a9bd1eb9e9/1/4/14005719500_1.jpg" width="600">

The full prototype setup is pictured below:

<img src="https://raw.githubusercontent.com/Civica-EE/DUTCHess/main/docs/prototype.png" width="600">

### Software
DUTCHess is written in C and relies on [Zephyr](https://www.zephyrproject.org/) to provide a Realtime OS (RTOS) under-the-hood.  Using Zephyr allows DUTCHess to be extremely modular and extensible; it is also easy to contribute to Zephyr as we have already proven by adding the SD card support for the MIMXRT1020 EVK which this project makes use of.

## Getting Started
In order to begin using DUTCHess you first need to follow the Zephyr [Getting Started](https://docs.zephyrproject.org/latest/getting_started/index.html) guide to download Zephyr and get the build tools.  We use a Linux development environment; YMMV on other platforms.

Use the "Hello World" example in the Zephyr documentation to get you up and running; attach the board via USB to your machine and ensure that this works before proceeding.  You will need to get the J-Link tools mentioned on [this page](https://docs.zephyrproject.org/latest/boards/arm/mimxrt1020_evk/doc/index.html) to flash the board.

After following that guide you can clone this repository, build, and flash the DUTCHess board:
```
git clone git@github.com:Civica-EE/DUTCHess.git
cd DUTCHess/
cmake -B build/ && ZEPHYR_BASE=~/zephyrproject/zephyr/ make -C build flash
```
## Features
### Configuration Interface
DUTCHess can be configured in two ways:
 - CLI
 - Web GUI

Certain features are only accessible by particular methods these will be noted for each feature in their own section.

#### CLI
DUTCHess provides a CLI which is accessible over telnet (port 23) and USB serial.

The CLI has several options available which can be viewed with the `help` command:
```
Available commands:
  clear     :Clear screen.
  date      :Date commands
  device    :Device commands
  dutchess  :DUTCHess commands
  eeprom    :EEPROM shell commands
  fs        :File system commands
  help      :Prints the help message.
  history   :Command history.
  i2c       :I2C commands
  kernel    :Kernel commands
  log       :Commands for controlling logger
  net       :Networking commands
  resize    :Console gets terminal screen size or assumes default in case the
             readout fails. It must be executed after each terminal width change
             to ensure correct text display.
  sensor    :Sensor commands
  shell     :Useful, not Unix-like shell commands.
```

##### Network Configuration
The `net` and `dutchess ip` command trees can be used to check and modify the network configuration.

```
uart:~$ net help
net - Networking commands
Subcommands:
  allocs     :Print network memory allocations.
  arp        :Print information about IPv4 ARP cache.
  capture    :Configure network packet capture.
  conn       :Print information about network connections.
  dns        :Show how DNS is configured.
  events     :Monitor network management events.
  gptp       :Print information about gPTP support.
  iface      :Print information about network interfaces.
  ipv6       :Print information about IPv6 specific information and
              configuration.
  mem        :Print information about network memory usage.
  nbr        :Print neighbor information.
  ping       :Ping a network host.
  pkt        :net_pkt information.
  ppp        :PPP information.
  resume     :Resume a network interface
  route      :Show network route.
  stacks     :Show network stacks information.
  stats      :Show network statistics.
  suspend    :Suspend a network interface
  tcp        :Connect/send/close TCP connection.
  udp        :Send/recv UDP packet
  virtual    :Show virtual network interfaces.
  vlan       :Show VLAN information.
  websocket  :Print information about WebSocket connections.
```

```
uart:~$ dutchess ip help
ip - IP command.
Subcommands:
  address  :IP address command.
  netmask  :IP netmask command.
  gateway  :IP gateway command.
```

Configuration set in the `dutchess ip` command tree will persist across reboots.  When a DUTCHess board is first started, these commands should be used via USB serial to set the network configuration of the board so that it can be used remotely.

Currently only IPv4 configuration is supported.  DHCP will be added in a future version.

DUTCHess will respond to ping, so you can verify your configuration by pinging DUTCHess before proceeding beyond the network configuration.

##### Temperature
The temperature can be read using the `dutchess temperature` command:
```
uart:~$ dutchess temperature 
Temperature: 29.875 C
```
This is useful if your DUT is temperature sensitive; you can check that the ambient temperature is within acceptable limits remotely.

##### Power
The relay on the DUTCHess can be toggled using the `dutchess relay` command:
```
uart:~$ dutchess relay on
Power ON

uart:~$ dutchess relay off
Power OFF
```
The high voltage side of the relay can be connected to the DUT to allow the power to be controlled remotely e.g. if a DUT is stuck in a boot loop.

#### Web GUI
DUTCHess provides an HTTP web GUI (port 80). This can be accessed via a browser at `http://<dutchess_ip>`

##### Power
The relay on the DUTCHess can be switched via the web GUI in much the same way as on the CLI.  Accessing `http://<dutchess_ip>/on` will turn on the relay and accessing `http://<dutchess_ip>/off` will turn it off.  This can be useful for scripts which need to automatically turn on/off the power to a DUT.

### Terminal Server
DUTCHess provides a terminal server accessible via telnet.  Different ports provide different baudrates, parity, etc.

Currently the following are hard-coded, but this configuration is planned to be added to the configuration interfaces in the future:

 - Port 21500
   - 115200 8N1
 - Port 25700
   - 57600 8N1

Telnet to the above ports connects to the UART on the D0 and D1 pins of the Arduino header on the MIMXRT1020 EVK.

### TFTP Server
DUTCHess provides a TFTP server implementation for writing images to an I2C attached EEPROM.  Often a DUT will have an I2C EEPROM for a bootloader or RCW; by attaching the I2C lines to the DUTCHess we can remotely update these.

```
$ tftp 192.0.2.7
tftp> bin
tftp> put arm.rcw eeprom/
```

If no EEPROM is attached, then the server will report an error.

## License
MIT

