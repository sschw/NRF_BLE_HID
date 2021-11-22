# Bluetooth: nRF Keyboard HID Device


## Overview

Exposes a BLE HID Service and sends button presses to the bonded central.

After 5 minutes, the device will enter SYSTEM_OFF which reduces the power consumption to below 0.3 uA ([see here](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52832.ps.v1.1%2Fpmu.html&cp=2_1_0_16_0_0_2&anchor=unique_498092668))


## Building and Running

This project is based on the zephyr samples/bluetooth/peripheral_hids project.

