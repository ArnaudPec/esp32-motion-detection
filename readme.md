# ESP32 SigFox Motion dectector/ Weather sensor

Basic SigFox esp32 firmware using:

* ESP32 Thing
* SigFox TD1208
* DHT22 Temperature/humidity sensor
* PIR sensor

Designed to be powered on battery, the esp32 is put in deep sleep mode when in
idle state.

Due to SigFox limitation of 140 messages per day, messages are sent every 12min
at maximum. Hence we can define 2 wakeups modes:

* On interruptions sent by the PIR detector, if no message was sent in the past
    24 minutes, send the payload.

* Scheduled, every 24 minutes and send the payload.

That way the limitation is never exceeded.

We use *AT commands* to communicate with the SigFox module and send messages.
The payload structure depends on remote dashboard configuration (actoboard in
our case). Here we have the following format:

    Temp (integer 8bits) / Humidity (integer 8bits) / Detection(integer 2bit)

which results in sending the following *msg* string:

    sprintf(msg, "AT$SS=%d %d %1.2d\r", temp, hum, detec);

A small lib has been written to read data from the DTH22.

## Pinouts


| ESP32 Thing   | TD1208        |
| ------------- | ------------- |
| TX pin 4      | RX            |
| RX pin 5      | TX            |
| VCC 3V3       | VCC 3V3       |
| GND           | GND           |

DHT connected on pin 26.

PIR detector connected on interrupt pin 25.

## Usage

    make menuconfig

    make
    make flash
