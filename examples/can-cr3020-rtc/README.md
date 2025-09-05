# CR3020 CAN Connected Realtime Clock Example

The CR3020 examples provide a basic implementation of the ifm effector [Real-time clock for CAN systems CR3020](https://www.ifm.com/us/en/product/CR3020?srsltid=AfmBOoqsSruHZrLVSjpKorOlhGbFJHD5bzMR2MzVLWwBelv40R0sv91H) which is a CANopen device superficially similar to DS3231 based realtime clocks and thus easy to test via comparison to the DS3231 example. This example is based on [Programming manual (supplement) CR3020 Hardware version: from AE](https://media.ifm.com/CIP/mediadelivery/asset/5f44d9c7fef2074fac50663b7dfd56f9/11595171_00_GB.pdf?contentdisposition=inline&_gl=1*vnam4v*_gcl_au*MTU4ODUzODYxNi4xNzUzMzkxMzI4*_ga*MTUxOTkxMzQyLjE3NTMzOTEzMjY.*_ga_5EX9WGPNX8*czE3NTM0NjM4OTkkbzMkZzEkdDE3NTM0NjM5NTUkajQkbDAkaDI4NDU1NjU0NA..*_fplc*a0F3TjR6U1NCWUJKWnZQSldZcUd1WjJkRFk2U2ZFWWF3Q1c3WU9uYThodGowOU14RXdQRmNXMnlnT29NSWFuWksyVmlZMEhTZXFwQVl3dEtKVHNuOXJzTEt0cDFLcTBySUR5UEJqcWs5cyUyQmxualVxSlBOTjVyMlElMkZMRlRaZyUzRCUzRA..). This example is intended as an example of how the CoreModel interface can be used to extend a Corellium Virtual Device. No functionality beyond what is demonstrated in the example usage below should be expected to be implemented. No suitability for any other application is guaranteed.

## Notable Limitations

* The example has not been tested with the CODESYS driver, only with `cansend` and `candump` from the can-utils package
* The example uses BCD as this is common in RTC chips and is easier to read when using `cansend` and `candump`
* The example only implements the CAN-FD interface
* The time set function is ack'd but otherwise ignored
* The time is always reported as the host system time, no attempt has been make to emulate low power or battery operation.
* The max time to alarm is 6 days not 7 as stated in the manual there are only 7 days in the week
* The manual does not define how alarms are to be signaled, thus not implemented
* The node ID can be set by sending the same value twice, otherwise ignored
* Baud rate is ignored.

## Example Usage

### Build

```bash
% make
```

### Testing with an i.MX93

Start an instance of the i.MX93 using the default firmware `imx-image-full-imx93evk-5.12.52-2.1.0.coreimage`.

#### Start CoreModel Port Forwarding

Navigate to the connect tab to find the quick connect ssh command. Using the same user and proxy information at a prompt use SSH to forward the CoreModel port:

```bash
% ssh [user at proxy] -L 1900:10.11.1.4:1900 -N             
```

#### Initialize the CAN Interface and Test Simple Echo using cansend

In an i.MX93 console or terminal enable the CAN bus and start `candump` to monitor traffic.

```bash
root@imx93evk:~# sudo ip link set can0 up type can bitrate 50000
root@imx93evk:~# candump can0
```

#### Run the CR3020 Example

In another terminal on your host run the CR3020 example built above (note CAN bus names do not line up):

```bash
./can-cr3020 127.0.0.1:1900 can1
```

#### Start testing

In a second i.MX93 console or terminal basic CAN traffic to the example using the `cansend` command.

```bash
root@imx93evk:~# cansend can0 003#110033005500
root@imx93evk:~# cansend can0 003#R
root@imx93evk:~# cansend can0 003fb873#0000
root@imx93evk:~# cansend can0 003fb873#R```
```

On the can-cr3020 terminal you will see the output:

```bash
ID 003 RTR 0 [00000033ffff8006 0000000000000000] 6, 1100_3300_5500
ID 003 RTR 1 [0000003bffff8000 0000000000000000]
ID 003fb873 RTR 0 [000000ffb8730002 0000000000000000] 2, 0000
ID 003fb873 RTR 1 [000000ffb8738000 0000000000000000]
```

On the i.MX terminal showing `candump` you will see:

```bash
  can0  003   [6]  11 00 33 00 55 00
  can0  003   [0]  remote request
  can0  003FB873   [2]  00 00
  can0  003FB873   [0]  remote request
```

#### Initialize the RTC and Check the Time

In the sending i.MX93 terminal initialize the RTC and request the time using the `cansend` command.

```bash
root@imx93evk:~# cansend can0 7ff#
root@imx93evk:~# cansend can0 313#
root@imx93evk:~# cansend can0 313#112233445566
root@imx93evk:~# cansend can0 213#
root@imx93evk:~# cansend can0 413#
root@imx93evk:~# cansend can0 513#
root@imx93evk:~# cansend can0 513#00112233445566
root@imx93evk:~# cansend can0 413#
```

The address 7ff is the CANopen broadcast address, in this case the RTC responds with its Node ID and what baud rate it is operating at. The second command requests the time command `0x200` from node `0x13`.

In the can-cr3040 terminal you will see output similar to:

```bash
ID 7ff RTR 0 [00007ff3ffff8000 0000000000000000]
Initialization received.
 -> 0
ID 313 RTR 0 [00003133ffff8000 0000000000000000]
Incorrect argument count 00 for RTC Set command.
ID 313 RTR 0 [00003133ffff8006 0000000000000000] 6, 1122_3344_5566
RTC Set.
Ignoring request to set Day 11 Month 22 Year 33 Hour 44 Minute 55 DOW 66
Day of Month [1-31] 28
Month [1-12] 07
Year since 1900 [0-99] 25
Hours [00-23] 16
Minutes [00-59] 50
Day of Week [1-7] 02
 -> 0
ID 213 RTR 0 [00002133ffff8000 0000000000000000]
RTC Request.
Day of Month [1-31] 28
Month [1-12] 07
Year since 1900 [0-99] 25
Hours [00-23] 16
Minutes [00-59] 50
Seconds [00-59] 33
Day of Week [1-7] 02
Battery State Sufficient
 -> 0
ID 413 RTR 0 [00004133ffff8000 0000000000000000]
Request Time to Alarm.
Days until alarm [0-6] 06
Hours until alarm [00-23] 07
Minutes until alarm [00-59] 00
NodeID 13
Baud Rate to 500 kBd
Enabled is 00
 -> 0
ID 513 RTR 0 [00005133ffff8000 0000000000000000]
Incorrect argument count 00 for Alarm Set command.
ID 513 RTR 0 [00005133ffff8007 0000000000000000] 7, 0011_2233_4455_66
Setting Alarm.
Days until alarm [0-6] 00
Hours until alarm [00-23] 10
Minutes until alarm [00-59] 00
Setting Baud Rate to 500 kBd
Setting Enabled to 66
 -> 0
ID 413 RTR 0 [00004133ffff8000 0000000000000000]
Request Time to Alarm.
Days until alarm [0-6] 00
Hours until alarm [00-23] 17
Minutes until alarm [00-59] 00
NodeID 13
Baud Rate to 500 kBd
Enabled is 00
 -> 0
```

In the `candump` terminal you will see output similar to:

```bash
  can0  7FF   [0] 
  can0  7FE   [2]  13 02
  can0  313   [0] 
  can0  313   [6]  11 22 33 44 55 66
  can0  393   [6]  28 07 25 16 50 02
  can0  213   [0] 
  can0  193   [8]  28 07 25 16 50 33 02 00
  can0  413   [0] 
  can0  193   [8]  06 04 00 13 13 02 00 00
  can0  513   [0] 
  can0  513   [7]  00 11 22 33 44 55 66
  can0  493   [7]  00 56 00 13 13 02 00
  can0  413   [0] 
  can0  193   [8]  00 22 00 13 13 02 00 00
```

#### Test the cansend command

Note this functionality requires release 7.6 or later.

In a second local terminal test basic CAN traffic to the example using the `can-cansend` command.

```bash
./can-cansend 127.0.0.1:1900 can1 065#
./can-cansend 127.0.0.1:1900 can1 065#002233
./can-cansend 127.0.0.1:1900 can1 065#abcdef01
./can-cansend 127.0.0.1:1900 can1 065#0022334
```

In the can-cr3040 terminal you will see output similar to:

```bash
ID 065 RTR 0 [0000065000000000 0000000000000000]
ID 065 RTR 0 [0000065000000003 0000000000000000] 3, 0022_33
ID 065 RTR 0 [0000065000000004 0000000000000000] 4, abcd_ef01
ID 065 RTR 0 [0000065000000003 0000000000000000] 3, 0022_33
```

In the `candump` terminal you will see output similar to:

```bash
  can0  065   [0] 
  can0  065   [3]  00 22 33
  can0  065   [4]  AB CD EF 01
  can0  065   [3]  00 22 33
```

Notice that when using an odd number of characters for the data the final character is silently dropped.
