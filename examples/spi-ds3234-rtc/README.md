# DS3234 SPI Connected Realtime Clock Example

The CR3020 examples provide a basic implementation of the Analog Devices [DS3234 Extremely Accurate SPI Bus RTC with Integrated Crystal and SRAM](https://www.analog.com/en/products/ds3234.html) which is a SPI  device similar to the DS3231 realtime clock and thus easy to test via comparison to the DS3231 example. This example is based on [Datasheet Rev 4; 3/15](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3234.pdf). This example is intended as an example of how the CoreModel interface can be used to extend a Corellium Virtual Device. No functionality beyond what is demonstrated in the example usage below should be expected to be implemented. No suitability for any other application is guaranteed.

This example was primarily tested with `spidev_test` on Linux 5.15 on i.MX93. `spidev_test` deasserts CS after every word while the DS3234 specification says that it support both one byte and multi-byte read/write transactions consisting of an address byte and one or multiple data bytes and does not address the case of a true single byte transaction. We work around this by use of the `-b` parameter to `spidev_test` which enables transactions of 8, 16, 24, or 32 bits. For transaction of 2, 3, or 4 bytes the example operates as per the specification returning 0x00 on all write transactions and as the first byte of read transactions. One byte read transactions return data in the first byte of the response despite this being logically impossible as it simplifies testing. One byte write transactions are ignored.

## Notable Limitations

* High Impedance is modeled as 0x00
* The example only implements the SPI interface.
* The square wave and interrupt outputs are not implemented.
* The time is always reported as the host system time, no attempt has been make to emulate low power or battery operation.
* Alarms are only calculated when the device is activated see below.
* The reset function is not implemented.
* The spec if two sequential reads from address 0x19 should access sequential bytes or the same bytes from the SRAM. The example chooses sequential access.
* `hwclock -f /dev/rtc1` access does not work as interrupts are not implemented.

### Alarm Implementation

Alarms in this example differ from alarms as defined in the specification. According to the specification alarms should be asserted whenever the alarm condition is satisfied even if reporting is suppressed at the instant of the condition and will remain asserted until cleared. The example only asserts if the condition is satisfied when the example is accessed via I2C interface.

Additionally the specification states that alarm conditions are defined by the table:

| DY/D̅T̅ | A1M4 | A1M3 | A1M2 | A1M1 | Alarm Rate |
| --- | --- | --- | --- | -- | -- |
| X | 1 | 1 | 1 | 1 | Alarm once per second |
| X | 1 | 1 | 1 | 0 | Alarm when seconds match |
| X | 1 | 1 | 0 | 0 | Alarm when minutes and seconds match |
| X | 1 | 0 | 0 | 0 | Alarm when hours, minutes and seconds match |
| 0 | 0 | 0 | 0 | 0 | Alarm when date, hours, minutes, and seconds match |
| 1 | 0 | 0 | 0 | 0 | Alarm when day, hours, minutes, and seconds match |

and then states that "Configurations now listed in the table will results in illogical operation." As "illogical operation" is poorly defined the code instead implements the table:

| DY/D̅T̅ | A1M4 | A1M3 | A1M2 | A1M1 | Alarm Rate |
| --- | --- | --- | --- | -- | -- |
| X | X | X | X | 1 | Alarm once per second |
| X | X | X | 1 | 0 | Alarm when seconds match |
| X | X | 1 | 0 | 0 | Alarm when minutes and seconds match |
| X | 1 | 0 | 0 | 0 | Alarm when hours, minutes and seconds match |
| 1 | 0 | 0 | 0 | 0 | Alarm when day, hours, minutes, and seconds match |
| 0 | 0 | 0 | 0 | 0 | Alarm when date, hours, minutes, and seconds match |

Alarm 2 is similarly implemented.

## Notes on DTBs and SPI addressing on the i.MX 93

Note that the firmware for most boards disables all SPI controllers not actively in use. Further enabling SPI controllers and configuring SPI drivers is rather opaque and google mostly finds frustration rather than answers. Thus a long discussion before the step by step instructions.

According to the [i.MX93 Applications Processor Reference Manual, Rev. 6, 2025-07-01 section 61.6](https://www.nxp.com/webapp/Download?colCode=IMX93RM) the i.MX 93 has 8 Low Power SPI (LPSPI) controllers located in memory at

```bash
61.6.1.1 LPSPI memory map
LPSPI1 base address: 4436_0000h
LPSPI2 base address: 4437_0000h
LPSPI3 base address: 4255_0000h
LPSPI4 base address: 4256_0000h
LPSPI5 base address: 426F_0000h
LPSPI6 base address: 4270_0000h
LPSPI7 base address: 4271_0000h
LPSPI8 base address: 4272_0000h
```

The coremodel interface uses the same order but uses 0 based numbering for the names: `lpspi0`, `lpspi1`, ... ,`lpspi7`. Note that while Linux also uses 0 based numbers it sorts by the address where the controller is enabled into memory. This means that if all controllers are enabled in the DTB CoreModel `lpspi0` will correspond to Linux `/dev/spidev6.0`, while if only the first 4 controllers are enabled in the DTB it will be `/dev/spidev2.0`. This can be confirmed, in the 4 controller case, with

```bash
root@imx93evk:~# ls -l /sys/bus/spi/devices/
total 0
lrwxrwxrwx 1 root root 0 Aug  7 21:06 spi0.0 -> ../../../devices/platform/soc@0/42000000.bus/42550000.spi/spi_master/spi0/spi0.0
lrwxrwxrwx 1 root root 0 Aug  7 21:06 spi1.0 -> ../../../devices/platform/soc@0/42000000.bus/42560000.spi/spi_master/spi1/spi1.0
lrwxrwxrwx 1 root root 0 Aug  7 21:06 spi2.0 -> ../../../devices/platform/soc@0/44000000.bus/44360000.spi/spi_master/spi2/spi2.0
lrwxrwxrwx 1 root root 0 Aug  7 21:06 spi3.0 -> ../../../devices/platform/soc@0/44000000.bus/44370000.spi/spi_master/spi3/spi3.0
```

As editing DTBs can be a challenge a fully configured DTB is included in this directory. It was created by downloading the default i.MX93 firmware `imx-image-full-imx93evk-5.12.52-2.1.0.coreimage` and copying the `devicetree` to a host with `dtc` installed. `dtc` was then used to extract the device tree from binary to text format using `dtc ^Cdtb devicetree > devicetree.dts`. The following patch was then applied:

```bash
379c379,387
< 				status = "disabled";
---
> 				status = "okay";
> 				spidev@0 {
> 					reg = <0>;
> 					compatible = "rohm,dh2228fv";
> 					spi-max-frequency = <4000000>;
> 					#address-cells = <1>;
> 					#size-cells = <0>;
> 					status = "okay";
> 				};
392c400,408
< 				status = "disabled";
---
> 				status = "okay";
> 				spidev@0 {
> 					reg = <0>;
> 					compatible = "maxim,ds3234";
> 					spi-max-frequency = <4000000>;
> 					#address-cells = <1>;
> 					#size-cells = <0>;
> 					status = "okay";
> 				};
```

before recompiling with `dtc -Odtb devicetree.dts > devicetree`. The resulting file is included in this directory. Note the before /dev entries:

```bash
root@imx93evk:~# ls /dev/rtc* /dev/spi*
ls: cannot access '/dev/spi*': No such file or directory
 /dev/rtc   /dev/rtc0
```

and after:

```bash
root@imx93evk:~# ls /dev/rtc* /dev/spi*
/dev/rtc  /dev/rtc0  /dev/rtc1	/dev/spidev0.0
```

Note that two SPI controllers have been enabled by changing from status `disabled` to `okay` and that two spi devices have been added one per controller. The first uses the `compatible = "rohm,dh2228fv"` which appears to be the internet standard opinion on the best way to enable `spidev_test` while the second identifies the chip being modeled to enable testing with the standard kernel driver.

## Testing Functionality

### Build

```bash
% make
```

### Testing with an i.MX93

Create a an instance of the i.MX93 using the default firmware `imx-image-full-imx93evk-5.12.52-2.1.0.coreimage`. Before enabling the device click on the "advanced boot options" checkbox. Or after creating the device visit the "Settings" page of the interface and then select the "Device Tree". From there upload the device tree from this directory and either "Save", or "Save & restart" to boot the i.MX93 with the updated DTB.

#### Start CoreModel Port Forwarding

Navigate to the connect tab to find the quick connect ssh command. Using the same user and proxy information at a prompt use SSH to forward the CoreModel port:

```bash
% ssh [user at proxy] -L 1900:10.11.1.4:1900 -N             
```

#### Run the DS3234 Example

In another terminal on your host run the DS3234 example built above:

```bash
./spi-ds3234 127.0.0.1:1900 lpspi0
```

Note that chipselect is defined in the source code while the spi bus is a command line argument.

#### Start testing

In the i.MX93 console exercise basic SPI testing using the `spidev_test` command.

```bash
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v      
spi mode: 0x0
bits per word: 8
max speed: 500000 Hz (500 kHz)
TX | FF FF FF FF FF FF 40 00 00 00 00 95 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF F0 0D  |......@.........................|
RX | 30 30 30 30 30 30 30 41 41 41 41 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 30 00  |0000000AAAA00000000000000000000.|
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 16
spi mode: 0x0
bits per word: 16
max speed: 500000 Hz (500 kHz)
TX | FF FF FF FF FF FF 40 00 00 00 00 95 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF F0 0D  |......@.........................|
RX | 30 31 30 31 30 31 30 31 00 48 00 48 30 31 30 31 30 31 30 31 30 31 30 31 30 31 30 31 30 31 30 31  |01010101.H.H01010101010101010101|
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 24
spi mode: 0x0
bits per word: 24
max speed: 500000 Hz (500 kHz)
can't send spi message: Connection timed out
Aborted
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | FF FF FF FF FF FF 40 00 00 00 00 95 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF F0 0D  |......@.........................|
RX | 30 31 32 33 30 31 32 33 00 56 58 09 30 31 32 33 30 31 32 33 30 31 32 33 30 31 32 33 30 31 32 33  |01230123.VX.01230123012301230123|
```

Notice the error when sending data that is not an even multiple of the transmit byte count.

On the spi-ds3234 terminal you will see the output:

```bash
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] 40
TX [1] 30
CS Asserted
RX [1] 00
Seconds [00-59] 41
Read
TX [1] 41
CS Asserted
RX [1] 00
Seconds [00-59] 41
Read
TX [1] 41
CS Asserted
RX [1] 00
Seconds [00-59] 41
Read
TX [1] 41
CS Asserted
RX [1] 00
Seconds [00-59] 41
Read
TX [1] 41
CS Asserted
RX [1] 95
Write
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] ff
TX [1] 30
CS Asserted
RX [1] f0
TX [1] 30
CS Asserted
RX [1] 0d
Alarm 2 Date [0-31] 00 Disabled
Read
TX [1] 00
CS Asserted
RX [2] ff ff
TX [2] 30 31
CS Asserted
RX [2] ff ff
TX [2] 30 31
CS Asserted
RX [2] ff ff
TX [2] 30 31
CS Asserted
RX [2] 40 00
TX [2] 30 31
CS Asserted
RX [2] 00 00
Seconds [00-59] 48
Read
TX [2] 00 48
CS Asserted
RX [2] 00 95
Seconds [00-59] 48
Read
TX [2] 00 48
CS Asserted
RX [2] ff ff
TX [2] 30 31
CS Asserted
RX [2] ff ff
TX [2] 30 31
CS Asserted
RX [2] ff ff
TX [2] 30 31
CS Asserted
RX [2] ff ff
TX [2] 30 31
CS Asserted
RX [2] ff ff
TX [2] 30 31
CS Asserted
RX [2] ff ff
TX [2] 30 31
CS Asserted
RX [2] ff ff
TX [2] 30 31
CS Asserted
RX [2] ff ff
TX [2] 30 31
CS Asserted
RX [2] ff ff
TX [2] 30 31
CS Asserted
RX [2] f0 0d
TX [2] 30 31
CS Asserted
RX [3] ff ff ff
TX [3] 30 31 32
RX [3] ff ff 40
TX [3] 30 31 32
RX [3] 00 00 00
Seconds [00-59] 51
Minutes [00-59] 58
Read
TX [3] 00 51 58
RX [3] ff ff ff
TX [3] 30 31 32
RX [3] ff ff ff
TX [3] 30 31 32
RX [3] ff ff ff
TX [3] 30 31 32
RX [3] ff ff ff
TX [3] 30 31 32
RX [3] ff ff f0
TX [3] 30 31 32
RX [4] ff ff ff ff
TX [4] 30 31 32 33
CS Asserted
RX [4] ff ff 40 00
TX [4] 30 31 32 33
CS Asserted
RX [4] 00 00 00 95
Seconds [00-59] 56
Minutes [00-59] 58
Hours [00-23] 09
Read
TX [4] 00 56 58 09
CS Asserted
RX [4] ff ff ff ff
TX [4] 30 31 32 33
CS Asserted
RX [4] ff ff ff ff
TX [4] 30 31 32 33
CS Asserted
RX [4] ff ff ff ff
TX [4] 30 31 32 33
CS Asserted
RX [4] ff ff ff ff
TX [4] 30 31 32 33
CS Asserted
RX [4] ff ff f0 0d
TX [4] 30 31 32 33
```

#### Check the Time

In the i.MX93 console request the time using the `spidev_test` command.

```bash
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x00\x00\x00\x00"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 00 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
RX | 00 23 17 10 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |.#..|
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x03\x00\x00\x00"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 03 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
RX | 00 02 11 88 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x06\x00\x00\x00"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 06 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
RX | 00 25 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |.%..|
```

In the ds3234 terminal you will see output similar to:

```bash
RX [4] 00 00 00 00
Seconds [00-59] 23
Minutes [00-59] 17
Hours [00-23] 10
Read
TX [4] 00 23 17 10
CS Asserted
RX [4] 03 00 00 00
Day of Week [1-7] 02
Day of Month [1-31] 11
Month [1-12] 08 + [Century] 01 = 88
Read
TX [4] 00 02 11 88
CS Asserted
RX [4] 06 00 00 00
Year since 1900 [0-99] 25
Alarm 1 Seconds [00-59] 00 Disabled
Alarm 1 Minutes [00-59] 00 Disabled
Read
TX [4] 00 25 00 00
```

#### Test reading and writing an alarm

In the i.MX93 console read the value of Alarm 1, write a new vale, and read it back using the `spidev_test` command.

```bash
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x07\x00\x00\x00"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 07 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
RX | 00 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x87\x01\x20\x30"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 87 01 20 30 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |.. 0|
RX | 00 01 20 30 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |.. 0|
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x07\x00\x00\x00"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 07 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
RX | 00 01 20 30 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |.. 0|
```

In the spi-ds3234 terminal you will see output similar to:

```bash
CS Asserted
RX [4] 07 00 00 00
Alarm 1 Seconds [00-59] 00 Disabled
Alarm 1 Minutes [00-59] 00 Disabled
Alarm 1 Hours [00-23] 00 Disabled
Read
TX [4] 00 00 00 00
CS Asserted
RX [4] 87 01 20 30
Setting Alarm 1 Seconds [00-59] 01 Disabled
Setting Alarm 1 Minutes [00-59] 20 Disabled
Setting Alarm 1 Hours [00-23] 20 Disabled
Write
TX [4] 00 01 20 30
CS Asserted
RX [4] 07 00 00 00
Alarm 1 Seconds [00-59] 01 Disabled
Alarm 1 Minutes [00-59] 20 Disabled
Alarm 1 Hours [00-23] 20 Disabled
Read
TX [4] 00 01 20 30
```

#### Test reading and writing the SRAM

In the i.MX93 console read the value of Alarm 1, write a new vale, and read it back using the `spidev_test` command.

```bash
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x18\x00\x00\x00"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 18 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
RX | 00 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x98\x07\x10\x20"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 98 07 10 20 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |... |
RX | 00 07 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x19\x00\x00\x00"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 19 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
RX | 00 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x98\x00\x00\x00"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 98 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
RX | 00 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x99\x01\x02\x03"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 99 01 02 03 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
RX | 00 01 02 03 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x98\x00\x00\x00"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 98 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
RX | 00 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
root@imx93evk:~# spidev_test -D /dev/spidev0.0 -v -b 32 -p "\x19\x00\x00\x00"
spi mode: 0x0
bits per word: 32
max speed: 500000 Hz (500 kHz)
TX | 19 00 00 00 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
RX | 00 01 02 03 __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __ __  |....|
```

In the spi-ds3234 terminal you will see output similar to:

```bash
CS Asserted
RX [4] 18 00 00 00
SRAM Address = 00
Read
TX [4] 00 00 00 00
CS Asserted
RX [4] 98 07 10 20
SRAM Address = 07
Write
TX [4] 00 07 00 00
CS Asserted
RX [4] 19 00 00 00
SRAM Data[07] = 00
SRAM Data[08] = 00
SRAM Data[09] = 00
Read
TX [4] 00 00 00 00
CS Asserted
RX [4] 98 00 00 00
SRAM Address = 00
Write
TX [4] 00 00 00 00
CS Asserted
RX [4] 99 01 02 03
SRAM Data[00] = 01
SRAM Data[01] = 02
SRAM Data[02] = 03
Write
TX [4] 00 01 02 03
CS Asserted
RX [4] 98 00 00 00
SRAM Address = 00
Write
TX [4] 00 00 00 00
CS Asserted
RX [4] 19 00 00 00
SRAM Data[00] = 01
SRAM Data[01] = 02
SRAM Data[02] = 03
Read
TX [4] 00 01 02 03
```

Note that it is unclear from the spec if two sequential reads from address 0x19 should access sequential bytes or the same bytes from the SRAM.
