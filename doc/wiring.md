# Wiring

The current board is a classic mini-USB ATmega328P Nano.

## Lens-Side Connections
![Opened Canon XL lens mount and internal wiring](images/canon-xl-lens-mount-wiring.jpg)\
The removed contact assembly is on the right. From top to bottom in this view,
the soldered wires are violet, blue, green, yellow, orange, red and brown.
These colors match the project wiring table below; they are not a universal
Canon color code or a connector numbering convention. Check continuity and
electrical levels on the actual lens before connecting it.

The brown wire is visible inside the lens. Its 4.5 V output is not connected
to the Nano; the photograph does not imply that every visible wire goes to
the controller. Work on the contacts only with all power disconnected and
avoid straining the internal ribbon cables.

## Controller Wiring
![Classic Nano SPI wiring](images/classic-nano-wiring.svg)\
This functional diagram does not show physical Canon connector contact positions.
Its editable English labels support later translation; see the
[illustration notes](images/README.md).

Colors identify the historical harness, not a Canon pin standard. Check the actual harness.

| Wire | Function | Arduino | AVR | RP2040 GPIO |
| --- | --- | --- | --- | --- |
| Violet | Common ground | GND | - | - |
| Blue | MISO, lens to controller | D12 | PB4 | 4 |
| Green | MOSI, controller to lens | D11 | PB3 | 7 |
| Yellow | Clock | D13 | PB5 | 6 |
| Orange | Active-low enable | D10 | PB2 | 5 |
| Red | External lens power, historically about 7/7.4 V | No logic pin | - | - |
| Brown | Lens 4.5 V output | Not connected | - | - |

Mapping source: [Arduino core variant](https://github.com/arduino/ArduinoCore-mbed/blob/4.6.0/variants/NANO_RP2040_CONNECT/variant.cpp).
The [classic Nano](https://docs.arduino.cc/resources/datasheets/A000005-datasheet.pdf)
uses 5 V logic; the
[Nano RP2040 Connect](https://docs.arduino.cc/resources/pinouts/ABX00053-full-pinout.pdf)
uses 3.3 V logic and its GPIO inputs are not 5 V tolerant. The lens SPI levels
have not been measured on this bench; lens supply voltage does not establish
SPI voltage. Measure bus levels before changing boards and use suitable 1 MHz
level translation if necessary. Never apply the lens 4.5/7.4 V rails to GPIO.

## Separate Lens Power
![UBEC regulator used for the lens supply](images/ubec-lens-power-regulator.jpg)\
The initial setup used a regulator identified as UBEC-3A-7.4V, supplied from 12 V,
for the lens power rail reported at about 7/7.4 V. This identifies the original
setup, not a measured specification for every UBEC module or Canon XL lens.

The regulator powers the lens through the red wire. Its return, the lens ground
and Nano GND share a common ground through the violet connection. The Nano
controls the SPI bus; it does not supply lens motor power through a GPIO or
its power pins. Keep the brown 4.5 V output disconnected from the Nano.

Measure the regulator output and verify polarity, regulation and current limiting
before connecting the lens. Lens supply voltage does not establish SPI logic
voltage. Never connect camcorder and Nano SPI outputs to the same lens
simultaneously: they are both masters.

## Transport Configuration

Constants: [Config.hpp](../canon_xl_controller/Config.hpp).

| Setting | Nominal value |
| --- | --- |
| Clock | 1000000 Hz; obtained initialization frequency published in STATUS |
| Format | Mode 3, 8 bits, LSB-first in both directions |
| Period | 20000 us |
| Lead | Enable low, MOSI low, wait 350 us |
| Bytes | 40, target start-to-start 20 us |
| Tail | MOSI low for 52 us, then enable high |
| Idle MOSI | High 4 us after enable high |

These are software requests derived from camcorder captures, not electrical
measurements. Mbed/USB interrupts introduce jitter; GPIO/SPI have overhead.
Interrupts remain enabled. AVR micros has 4 us resolution at 16 MHz. Missed
periods are skipped without catch-up bursts and counted. Checksum is
`(~sum(first 39 bytes)) & 255`; full sum is FF modulo 256. Display numbering
is 1..40, C++ indexing 0..39.
