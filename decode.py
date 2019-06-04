#!/usr/bin/env python3
import sys
import __future__

HEADER_BOOT =  0x00
HEADER_UPDATE = 0x01
HEADER_BUTTON_CLICK = 0x02
HEADER_BUTTON_HOLD  = 0x03

header_lut = {
    HEADER_BOOT: 'BOOT',
    HEADER_UPDATE: 'UPDATE',
    HEADER_BUTTON_CLICK: 'BUTTON_CLICK',
    HEADER_BUTTON_HOLD: 'BUTTON_HOLD'
}


def decode(data):
    if len(data) != 12:
        raise Exception("Bad data length, 12 characters expected")

    header = int(data[0:2], 16)

    temperature = int(data[2:6], 16) if data[2:6] != 'ffff' else None

    if temperature:
        if temperature > 32768:
            temperature -= 65536
        temperature /= 10.0

    return {
        "header": header_lut[header],
        "temperature": temperature,
        "humidity": int(data[6:8], 16) / 2.0 if data[6:8] != 'ff' else None,
        "pressure": int(data[8:12], 16) * 2 if data[8:12] != 'ffff' else None,
    }


def pprint(data):
    print('Header :', data['header'])
    print('Temperature :', data['temperature'])
    print('Humidity :', data['humidity'])
    print('Pressure :', data['pressure'])


if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] in ('help', '-h', '--help'):
        print("usage: python3 decode.py [data]")
        exit(1)

    data = decode(sys.argv[1])
    pprint(data)
