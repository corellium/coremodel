# CoreModel Examples

The CoreModel examples provide a skeleton for a device to be attached to a VMs virtual interfaces. There are independently compiled examples for the CoreModel API usage for list, UART, I2C, SPI, CAN, GPIO, and USBH. Not every VM type currently supports CoreModel API, the IMX93, IMX8, and RPI4B can be used to test connecting to the virtual interfaces.
On boot of the VM the port number to use with CoreModel is provided by `model-gw:`.


## List

The list example shows how to properly use the list APIs and enumerate all of the available virtual interfaces on a VM. The list example can be run be command line by providing the `<ip:port>`.

```
./coremodel-list 10.10.0.3:1900
```

Below is the enumeration of an IMX93 VM interfaces. For interfaces like UART it will only return UARTs that do not have devices on them.

```
----------------------------------------
|  index  |  Type  |  name  |  number  |
----------------------------------------
     0       gpio    iomuxc      108
     1       uart    lpuart2     1
     2       uart    lpuart3     1
     3       uart    lpuart5     1
     4       uart    lpuart6     1
     5       uart    lpuart7     1
     6       spi     lpspi0      2
     7       spi     lpspi1      2
     8       spi     lpspi2      2
     9       spi     lpspi3      3
    10       spi     lpspi4      2
    11       spi     lpspi5      2
    12       spi     lpspi6      2
    13       spi     lpspi7      2
    14       i2c     lpi2c0      128
    15       i2c     lpi2c1      128
    16       i2c     lpi2c2      128
    17       i2c     lpi2c3      128
    18       i2c     lpi2c4      128
    19       i2c     lpi2c5      128
    20       i2c     lpi2c6      128
    21       i2c     lpi2c7      128
    22       can     can0        1
    23       can     can1        1

```

## UART

The UART example shows how to properly attach to a virtual UART interface with the necessary functions to provide RX/TX between the VM and attached device. To attach to a UART `<ip:port>` and `<name>` of the UART interface needs to be provided.

```
./coremodel-uart 10.10.0.3:1900 lpuart3
```

The example will print anything that has been sent from the VM to `stdout`.

## I2C

The I2C example provides an I2C dummy device that will attach to a virtual I2C interface. The I2C address 0x42 is statically defined in the example. If attaching to an address that is already used it will overwrite the device at that address. Attaching the dummy device to the I2C address provide `<ip:port>` and `<name>` of the I2C interface.

```
./coremodel-i2c 10.10.0.3:1900 lpi2c0
```

The example will print anything that has been sent from the VM to `stdout`. The VM will be provided dummy data on reads.

## SPI

The SPI example shows the basic way to connect to a virtual SPI bus and attaches a dummy device. The chip select is statically assigned to index 0. Attaching the dummy device to the SPI address provide `<ip:port>` and `<name>` of the SPI interface.

```
./coremodel-spi 10.10.0.3:1900 lpspi0
```

The example will print anything that has been sent from the VM to `stdout`. The VM will be provided dummy data on reads.

## CAN

The CAN example shows the basic way to connect to a virtual CAN bus and attaches a dummy device. Attaching the dummy device to the CAN address provide `<ip:port>` and `<name>` of the CAN interface.

```
./coremodel-can 10.10.0.3:1900 can0
```

The example will print anything that has been sent from the VM to `stdout`. The VM will be provided only `<ctrl>` of `0x8ac7ffff00` on reads.

## GPIO

The GPIO example shows how to attach to the virtual GPIO interface and monitor GPIO pins. To attach and monitor GPIO pins provide `<ip:port>` and `<name>` of the GPIO interface followed by the `<index>` of the GPIO pins to be monitored. 

```
./coremodel-gpio 10.10.0.3:1900 iomuxc 0 7 8 16
```

The `coremodel-gpio` will print the voltage values in millivolts to `stdout` when the values change. The GPIO pins 7 is red, 8 is green, and 16 is blue LEDs of the IMX93.

```
GPIO[0] = 3300 mV
GPIO[7] = 0 mV
GPIO[8] = 0 mV
GPIO[16] = 0 mV
``` 

## USBH

The USBH example shows how to attach to the virtual USBH interface. To attach USBH dummy provide `<ip:port>` and `<name>` of the USBH interface followed by the `<port>` of the USBH bus.

```
./coremodel-usbh 10.10.0.3:1900 xhci 0
```

The example will print the data sent to the device on `USB_TKN_OUT`, `USB_TKN_IN`, and `USB_TKN_SETUP` to `stdout`. The IMX93 does not have an exposed USBH to CoreModel the following is from RPI4B.

```
RESET
XFR 00 EP0 SETUP [8]: 80 06 00 01 00 00 40 00 -> 8
XFR 00 EP0 IN [64] -> 12 01 00 02 00 00 00 08 6b 1d 04 01 01 01 01 02 00 01
XFR 00 EP0 OUT [0]: -> 0
RESET
RESET
XFR 03 EP0 SETUP [8]: 80 06 00 01 00 00 12 00 -> 8
XFR 03 EP0 IN [18] -> 12 01 00 02 00 00 00 08 6b 1d 04 01 01 01 01 02 00 01
XFR 03 EP0 OUT [0]: -> 0
XFR 03 EP0 SETUP [8]: 80 06 00 06 00 00 0a 00 -> -2
XFR 03 EP0 IN [10] -> 12 01 00 02 00 00 00 08 6b 1d
XFR 03 EP0 OUT [0]: -> 0
XFR 03 EP0 SETUP [8]: 80 06 00 06 00 00 0a 00 -> -2
XFR 03 EP0 IN [10] -> 12 01 00 02 00 00 00 08 6b 1d
XFR 03 EP0 OUT [0]: -> 0
XFR 03 EP0 SETUP [8]: 80 06 00 06 00 00 0a 00 -> -2
XFR 03 EP0 IN [10] -> 12 01 00 02 00 00 00 08 6b 1d
XFR 03 EP0 OUT [0]: -> 0
XFR 03 EP0 SETUP [8]: 80 06 00 02 00 00 09 00 -> 8
XFR 03 EP0 IN [9] -> 09 02 22 00 01 01 03 a0 00
XFR 03 EP0 OUT [0]: -> 0
XFR 03 EP0 SETUP [8]: 80 06 00 02 00 00 22 00 -> 8
XFR 03 EP0 IN [34] -> 09 02 22 00 01 01 03 a0 00 09 04 00 00 01 03 01 01 04 09 21 11 01 21 01 22 3f 00 07 05 81 03 08 00 02
XFR 03 EP0 OUT [0]: -> 0
XFR 03 EP0 SETUP [8]: 80 06 00 03 00 00 ff 00 -> 8
XFR 03 EP0 IN [255] -> 04 03 09 04
XFR 03 EP0 OUT [0]: -> 0
XFR 03 EP0 SETUP [8]: 80 06 02 03 09 04 ff 00 -> 8
XFR 03 EP0 IN [255] -> 12 03 4b 00 65 00 79 00 62 00 6f 00 61 00 72 00 64 00
XFR 03 EP0 OUT [0]: -> 0
XFR 03 EP0 SETUP [8]: 80 06 01 03 09 04 ff 00 -> 8
XFR 03 EP0 IN [255] -> 14 03 43 00 6f 00 72 00 65 00 6c 00 6c 00 69 00 75 00 6d 00
XFR 03 EP0 OUT [0]: -> 0
XFR 03 EP0 SETUP [8]: 00 09 01 00 00 00 00 00 -> 8
XFR 03 EP0 IN [0] -> 0
XFR 03 EP0 SETUP [8]: 80 06 03 03 09 04 ff 00 -> 8
XFR 03 EP0 IN [255] -> 12 03 4b 00 65 00 79 00 62 00 6f 00 61 00 72 00 64 00
XFR 03 EP0 OUT [0]: -> 0
XFR 03 EP0 SETUP [8]: 80 06 04 03 09 04 ff 00 -> 8
XFR 03 EP0 IN [255] -> 16 03 48 00 49 00 44 00 20 00 44 00 65 00 76 00 69 00 63 00 65 00
XFR 03 EP0 OUT [0]: -> 0
XFR 03 EP0 SETUP [8]: 21 0a 00 00 00 00 00 00 -> 8
XFR 03 EP0 IN [0] -> -2
XFR 03 EP0 SETUP [8]: 81 06 00 22 00 00 3f 00 -> 8
XFR 03 EP0 IN [63] -> 05 01 09 06 a1 01 05 07 19 e0 29 e7 15 00 25 01 75 01 95 08 81 02 95 01 75 08 81 01 95 05 75 01 05 08 19 01 29 05 91 02 95 01 75 03 91 01 95 06 75 08 15 00 25 65 05 07 19 00 29 65 81 00 c0
XFR 03 EP0 OUT [0]: -> 0
XFR 03 EP0 SETUP [8]: 21 09 00 02 00 00 01 00 -> 8
XFR 03 EP0 OUT [1]: 00 -> 1
XFR 03 EP1 IN [8] -> -1
XFR 03 EP0 IN [0] -> -2

```