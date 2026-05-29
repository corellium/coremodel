# DS3231 i2c Connected Realtime Clock Example

The DS3231 examples provide a basic implementation of the Analog Devices [DS3231 Extremely Accurate I²C-Integrated RTC/TCXO/Crystal](https://www.analog.com/en/products/ds3231.html) which is commonly used in the Raspberry Pi and Arduino ecosystems making it particularly easy to test. This example is based on [Datasheet Rev 10 3/2015](https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf). This example is intended as an example of how the CoreModel interface can be used to extend a Corellium Virtual Device. No functionality beyond what is demonstrated in the example usage below should be expected to be implemented. No suitability for any other application is guaranteed.

## Notable Limitations

* The example only implements the I2C interface.
* The square wave and interrupt outputs are not implemented.
* The time is always reported as the host system time, no attempt has been make to emulate low power or battery operation.
* Alarms are only calculated when the device is activated see below.

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

#### Run the DS3231 Example

In another terminal on your host run DS3231 example built above:

```bash
./i2c-ds3231 127.0.0.1:1900 lpi2c0
```

#### Test Single Byte Read Operations using i2cget

In the i.MX93 console check that a device registers can be accessed using the `i2cget` command.

```bash
root@imx93evk:~# i2cget -y 0 0x42 0x0f
0x88
```

#### Test Multiple 1 Byte Read Operations using i2cdump

In the i.MX93 console check that the devices registers can be accessed using the `i2cdump` command.

```bash
root@imx93evk:~# i2cdump -r 0x00-0x13 -y 0 0x42 b
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f    0123456789abcdef
00: 33 24 16 06 13 86 25 00 00 00 00 00 00 00 1c 88    3$????%.......??
10: 00 19 40 35                                        .?@5            

```

The argument `-r 0x00-0x12` indicates that the command is to probe the range of addresses 0x00 to 0x12 within the device on the i2c bus. `-y` disables interactive mode so that you don't have to type Y to continue. `0` indicates that the device is connected to i2c bus 0 as selected by the `lpi2c0` argument to the example. `0x42` indicates the device id on the selected i2c bus which is defined as an argument to `coremodel_attach_i2c` near the end of i2c-ds3231.c. The `b` indicates byte access mode. You may note that the console where the example is running reports that each read is proceeded by a write of the address and wonder if multi-byte reads can be accomplished.

#### Test Multiple 2 Byte Read Operations using i2cdump

```bash
root@imx93evk:~# i2cdump -r 0x00-0x13 -y 0 0x42 W
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f    0123456789abcdef
00: 33 24 16 06 13 86 25 00 00 00 00 00 00 00 1c 88    3$????%.......??
10: 00 19 40 35                                        .?@5            

```

The trailing `W` indicates that 16 bit word access mode should be used. The output indicates that the example implements both auto-increment addressing to enable multi-byte reads and properly handles wrap around. This is important as in `W` mode the range must start with an even address and end with an odd address. The DS3231 has an odd number of registers so the final address wraps around to the first address rather than reading NULL.

#### Testing Multi-Byte Read Operation using i2Cdump

```bash
root@imx93evk:~# i2cdump -r 0x00-0x13 -y 0 0x42 i
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f    0123456789abcdef
00: 34 31 16 06 13 86 25 00 00 00 00 00 00 00 1c 88    41????%.......??
10: 00 19 40 34       
```

The trailing `i` indicates that a full i2c block should be transmitted. Notice that the debugging output suggests that an i2c block is about twice the size of the register bank.

#### Putting Semantic Meaning to the Data

Format the data:

```bash
i2cdump -r 0x00-0x02 -y 0 0x42 b | tail -1 | sed -e 's/.*: \(..\) \(..\) \(..\).*/The current time is \3:\2:\1/'
i2cdump -r 0x04-0x06 -y 0 0x42 b | tail -1 | sed -e 's/.*: *\(..\) \(..\) \(..\).*/The current date is 20\3 \2 \1/' #ignore the bonus 8 in the month number

```

#### Testing Writes using i2cset

```bash
i2cset -y 0 0x42 0x00 0x00 # write one byte
i2cset -y 0 0x42 0x00 0x01 0x02 i # write all the bytes

```

#### Testing the Alarms

```bash
root@imx93evk:~# i2cget -y 0 0x42 0x0f
0x88
root@imx93evk:~# i2cset -y 0 0x42 0x07 0x80 0x80 0x80 0x80 i
root@imx93evk:~# i2cget -y 0 0x42 0x0f
0x89
```

Notice the low order bit corresponding to Alarm 1 active is now set.
