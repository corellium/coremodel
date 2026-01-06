from coremodel import *
import time
from datetime import datetime

class ds3234(CoreModelSpi):
    def __init__(self, busname, cs, devname):
        super().__init__(busname = busname, cs = cs)
        self.devname = devname

        self.system_time = time.time()
        self.index = 0
        self.twelve_hour_flag = 0
        self.Alarm1_Sec = 0
        self.Alarm1_Min = 0
        self.Alarm1_Hours = 0
        self.Alarm1_DayDate = 1
        self.Alarm2_Min = 0
        self.Alarm2_Hours = 0
        self.Alarm2_DayDate = 1
        self.control = 0b00011100
        self.status = 0b10001000
        self.aging = 0
        self.SRAM_Address = 0
        self.SRAM_Data = [0] * 256

    # Convert a binary value into 7 bit BCD
    def format_7bit_bcd(self, binary_value):
        return ((int(binary_value / 10) << 4) + binary_value % 10) & 0x7f

    # Convert a binary value into 5 bit BCD
    def format_6bit_bcd(self, binary_value):
        return self.format_7bit_bcd(binary_value) & 0x3f

    # Convert a binary value into 4 bit BCD
    def format_5bit_bcd(self, binary_value):
        return self.format_7bit_bcd(binary_value) & 0x1f

    # Print Enabled or disabled based on the high bit
    def print_enabled(self, value):
        if (value & 0x80) > 0:
            print("Alarm Enabled")
        else:
            print("Alarm Disabled")

    def cs(self, csel):
        if not csel:
            return

        print("CS Asserted")

        # read the system time
        dt_object = datetime.fromtimestamp(self.system_time)
        old_second = dt_object.second
        old_minute = dt_object.minute
        self.system_time = time.time()
        dt_object = datetime.fromtimestamp(self.system_time)

        # calculating alarm 1
        if (self.Alarm1_Sec & 0x80) > 0: # A1M1 is high
            if old_second != self.format_7bit_bcd(dt_object.second):
                self.status |= 0b1
        elif (self.Alarm1_Min & 0x80) > 0: #A1M2 is high
            if (self.Alarm1_Sec & 0x7f) == self.format_7bit_bcd(dt_object.second):
                self.status |= 0b1
        elif (self.Alarm1_Hours & 0x80) > 0: #A1M3 is high
            if (self.Alarm1_Min & 0x7f) == self.format_7bit_bcd(dt_object.minute):
                self.status |= 0b1
        elif (self.Alarm1_DayDate & 0x80) > 0: #A1M4 is high
            if self.twelve_hour_flag > 0:
                if dt_object.hour > 11:
                    if (self.Alarm1_Hours & 0x7f) == self.format_5bit_bcd(dt_object.hour - 12) + 0b01100000:
                        self.status |= 0b1
                else:
                    if (self.Alarm1_Hours & 0x7f) == self.format_5bit_bcd(dt_object.hour - 12) + 0b01000000:
                        self.status |= 0b1

            else:
                if (self.Alarm1_Hours & 0x7f) == self.format_6bit_bcd(dt_object.hour):
                    self.status |= 0b1

        elif (self.Alarm1_DayDate & 0x40) > 0: # DY/!DT is high
            if (self.Alarm1_DayDate & 0x3f) == dt_object.day + 1:
                self.status |= 0b1
        else: # all flags are low
            if (self.Alarm1_DayDate & 0x3f) == self.format_6bit_bcd(dt_object.day):
                self.status |= 0b1

        # calculating alarm 2
        if (self.Alarm2_Min & 0x80) > 0: #A1M2 is high
            if old_minute != self.format_7bit_bcd(dt_object.minute):
                self.status |= 0b10
        elif (self.Alarm2_Hours & 0x80) > 0: #A1M3 is high
            if (self.Alarm2_Min & 0x7f) == self.format_7bit_bcd(dt_object.minute):
                self.status |= 0b10
        elif (self.Alarm2_DayDate & 0x80) > 0: #A1M4 is high
            if self.twelve_hour_flag > 0:
                if dt_object.hour > 11:
                    if (self.Alarm2_Hours & 0x7f) == self.format_5bit_bcd(dt_object.hour - 12) + 0b01100000:
                        self.status |= 0b10
                else:
                    if (self.Alarm2_Hours & 0x7f) == self.format_5bit_bcd(dt_object.hour - 12) + 0b01000000:
                        self.status |= 0b10

            else:
                if (self.Alarm2_Hours & 0x7f) == self.format_6bit_bcd(dt_object.hour):
                    self.status |= 0b10

        elif (self.Alarm2_DayDate & 0x40) > 0: # DY/!DT is high
            if (self.Alarm2_DayDate & 0x3f) == dt_object.day + 1:
                self.status |= 0b10
        else: # all flags are low
            if (self.Alarm2_DayDate & 0x3f) == self.format_6bit_bcd(dt_object.day):
                self.status |= 0b10


        # note this code does not check if an alarm should have gone off since the
        # last CS, rather it only alerts if it should be on now

        print("cs", csel)

    def access_reg(self, addr, write, data):
        rdata = 0
        dt_object = datetime.fromtimestamp(self.system_time)
        if addr == 0: # BCD Seconds
            if write:
                print("Ignoring write of", hex(data), "to seconds register")
            else:
                # reread clock every time reg 0 is read
                dt_object = datetime.fromtimestamp(self.system_time)
                rdata = self.format_7bit_bcd(dt_object.second)
                print("Seconds [00-59]:", hex(rdata))


        elif addr == 1: # BCD Minutes
            if write:
                print("Ignoring write of", hex(data), "to minutes register")
            else:
                rdata = self.format_7bit_bcd(dt_object.minute)
                print("Minutes [00-59]:", hex(rdata))


        elif addr == 2: # BCD Hours
            if write:
                if (data & 0x40) > 0:
                    print("Setting AM/PM mode")
                    self.twelve_hour_flag = 1
                else:
                    print("Setting 24 hour mode")
                    self.twelve_hour_flag = 0

            else:
                if self.twelve_hour_flag > 0:
                    if dt_object.hour > 11:
                        rdata = self.format_5bit_bcd(dt_object.hour - 12) + 0b01100000
                    else:
                        rdata = self.format_5bit_bcd(dt_object.hour - 12) + 0b01000000

                    print("Hours flags + [00-11]:", hex(data))
                else:
                    rdata = self.format_6bit_bcd(dt_object.hour)
                    print("Hours [00-23]:", hex(rdata))


        elif addr == 3: # 1 based day of week
            if write:
                print("Ignoring write of", hex(data), "to day of week register")
            else:
                rdata = dt_object.day + 1
                print("Day of Week [1-7]:", hex(rdata))


        elif addr == 4: # 1 based day of month
            if write:
                print("Ignoring write of", hex(data), "to day of month register")
            else:
                rdata = self.format_6bit_bcd(dt_object.day)
                print("Day of Month [1-31]:", hex(rdata))

        elif addr == 5: # centry + month
            if write:
                print("Ignoring write of", hex(data), "to month register")
            else:
                rdata = (int(dt_object.year / 100) << 7) + self.format_5bit_bcd(dt_object.month + 1)
                print("Month [1-12]", hex(rdata & 0b11111), "+ [Century]", hex(rdata >> 7), "=", hex(rdata))

        elif addr == 6: # year
            if write:
                print("Ignoring write of", hex(data), "to year register")
            else:
                rdata = (int((dt_object.year % 100) / 10) << 4) + dt_object.year % 10
                print("Year since 1900 [0-99]:", hex(rdata))

        elif addr == 7: # Alarm 1 Seconds
            if write:
                self.Alarm1_Sec = data
                print("Setting ", end="")
                print("Alarm 1 Seconds [00-59]:", hex(data & 0x7f))
                self.print_enabled(data)
            else:
                rdata = self.Alarm1_Sec
                print("Alarm 1 Seconds [00-59]:", hex(rdata & 0x7f))
                self.print_enabled(rdata)

        elif addr == 8: # Alarm 1 Minutes
            if write:
                self.Alarm1_Min = data
                print("Setting ", end="")
                print("Alarm 1 Minutes [00-59]:", hex(data & 0x7f))
                self.print_enabled(data)
            else:
                rdata = self.Alarm1_Min
                print("Alarm 1 Minutes [00-59]:", hex(rdata & 0x7f))
                self.print_enabled(rdata)

        elif addr == 9: # Alarm 1 Hours
            if write:
                self.Alarm1_Hours = data
                print("Setting ", end="")
                print("Alarm 1 Hours [00-23]:", hex(data & 0x6f))
                self.print_enabled(data)
            else:
                rdata= self.Alarm1_Hours
                print("Alarm 1 Hours [00-23]:", hex(rdata & 0x6f))
                self.print_enabled(rdata)

        elif addr == 0xA: # Alarm 1 Day Date
            if write:
                self.Alarm1_DayDate = data
                print("Setting ", end="")
                if (data & 0x40) > 0:
                    print("Alarm 1 Day [1-7]:", hex(data & 0x3f))
                else:
                    print("Alarm 1 Day of Month [1-31]:", hex(data & 0x3f))
                self.print_enabled(data)
            else:
                rdata = self.Alarm1_DayDate
                if (rdata & 0x40) > 0:
                    print("Alarm 1 Day [1-7]:", hex(rdata & 0x3f))
                else:
                    print("Alarm 1 Day of Month [1-31]:", hex(rdata & 0x3f))
                self.print_enabled(rdata)


        elif addr == 0xB: # Alarm 2 Minutes
            if write:
                self.Alarm2_Min = data
                print("Setting ", end="")
                print("Alarm 2 Minutes [00-59]:", hex(data & 0x7f))
                self.print_enabled(data)
            else:
                rdata = self.Alarm2_Min
                print("Alarm 2 Minutes [00-59]:", hex(rdata & 0x7f))
                self.print_enabled(rdata)

        elif addr == 0xC: # Alarm 2 Hours
            if write:
                self.Alarm2_Hours = data
                print("Setting ", end="")
                print("Alarm 2 Hours [00-23]:", hex(data & 0x6f))
                self.print_enabled(data)
            else:
                rdata = self.Alarm2_Hours
                print("Alarm 2 Hours [00-23]:", hex(rdata & 0x6f))
                self.print_enabled(rdata)

        elif addr == 0xD: # Alarm 2 Day Date
            if write:
                self.Alarm1_DayDate = data
                print("Setting ", end="")
                if (data & 0x40) > 0:
                    print("Alarm 2 Day [1-7]:", hex(data & 0x3f))
                else:
                    print("Alarm 2 Day of Month [1-31]:", hex(data & 0x3f))
                self.print_enabled(data)
            else:
                rdata = self.Alarm1_DayDate
                if (data & 0x40) > 0:
                    print("Alarm 2 Day [1-7]:", hex(rdata & 0x3f))
                else:
                    print("Alarm 2 Day of Month [1-31]:", hex(rdata & 0x3f))
                self.print_enabled(rdata)


        elif addr == 0xE: # Control: EOSC BBSQW CONV RS2 RS1 INTCN A2IE A1IE not implemented
            if write:
                self.control = hex(data & 0x1f)
                print("Setting ", end="")
            else:
                rdata = self.control

            print("Oscillator Enabled,", end="")
            print("Square-Wave Disabled,", end="")
            print("Temp Conversion Finished,", end="")

            control = (self.control >> 3) & 0b11

            if control == 0b00:
                print("Frequency 1Hz,", end="")

            elif control == 0b01:
                print("Frequency 1.024kHz,", end="")

            elif control == 0b10:
                print("Frequency 4.096kHz,", end="")

            elif control == 0b11:
                print("Frequency 8.192kHz,", end="")


            if (self.control & 0b100) > 0:
                print("Interrupt Mode,",  end="")
            else:
                print("Oscillator Mode,",  end="")

            if (self.control & 0b10) > 0:
                print("Alarm 2 Enabled,",  end="")
            else:
                print("Alarm 2 Disabled",  end="")

            if (self.control & 0b1) > 0:
                print("Alarm 1 Enabled",  end="")
            else:
                print("Alarm 1 Disabled",  end="")

        elif addr == 0xF: # Status: OSF 0 0 0 EN32kHz BSY A2F A1F
            if write:
                # set bit 3
                self.status = (self.status & 0xf7) | (data & 0b1000)
                # bits 1 and 0, clear on 0 ignore 1
                if (data & 0b10) == 0:
                    self.status &= 0xfd
                if (data & 0b1) == 0:
                    self.status &= 0xfe

            rdata = self.status
            print("Oscillator Running,",  end="") # bit 7
            if (self.status & 0b1000) > 0: # bit 3
                print("32kHz Output Enabled,",  end="")
            else:
                print("32kHz Output Disabled,",  end="")

            print("Temp Conversion Finished,",  end="") # bit 2
            if (self.status & 0b10) > 0:
                print("Alarm 2 Active,",  end="")
            else:
                print("Alarm 2 Inactive,",  end="")

            if (self.status & 0b1) > 0:
                print("Alarm 1 Active",  end="")
            else:
                print("Alarm 1 Inactive",  end="")

        elif addr == 0x10: # Aging offset has no user effect
            if write:
                self.aging = data
                print("Aging offset", hex(data),  end="")
            else:
                rdata = self.aging
                print("Aging offset", hex(data),  end="")

        elif addr == 0x11: # Temperature upper byte not implemented
            # statically return 25C
            rdata = 0b00011001
            print("The Temperature is", hex(rdata), "C",  end="")

        elif addr == 0x12: # Temperature lower byte not implemented
            # statically return .25C
            rdata = 1 << 6
            print((rdata > 6) * 25, end="")

        elif addr == 0x18: # SRAM Address Register
            if write:
                self.SRAM_Address = data
                print("SRAM Address =", hex(data), end="")
            else:
                rdata = self.SRAM_Address
                print("SRAM Address =", hex(rdata), end="")

        elif addr == 0x19: # SRAM Data
            if write:
                self.SRAM_Data[self.SRAM_Address] = data
                print("SRAM Data[", self.SRAM_Address, "] = ", data, sep="", end="")
            else:
                rdata = self.SRAM_Data[self.SRAM_Address]
                print("SRAM Data[", self.SRAM_Address, "] = ", rdata, sep="", end="")

            self.SRAM_Address = (self.SRAM_Address + 1) % 0xff # & 0xff likely redundant

        print("")
        return rdata

    def xfr(self, wrdata, rddata):
        length = len(rddata)
        write = wrdata[0] & 0x80
        addr = wrdata[0] & 0x7f

        # dump intput data
        print("RX [", length, "]", sep="", end="")
        for idx in range(length):
            print(" ", hex(wrdata[idx]), sep=" ", end="")

        print("")

        rddata[0] = 0 # per spec first read byte is always high impedance

        if addr < 0x13: #RTC Registers
            if not write and length == 1: #special case for read one byte
                rddata[0] = self.access_reg( addr, write, wrdata[0])

            for idx in range(1, length):
                rddata[idx] = self.access_reg(((addr + idx - 1) % 0x13), write, wrdata[idx])

        elif addr == 0x18: # SRAM Address
            if length == 1:
                if not write: # Read out the address as a magic first byte
                    rddata[0] = self.access_reg(addr, write, wrdata[0])
            else: # read or write the address as the second byte
                rddata[1] = self.access_reg(addr, write, wrdata[1])

            for idx in range(2, length):
                rddata[idx] = 0 # ignore additional bytes

        elif addr == 0x19: #SRAM Data
            if not write and len == 1: # special case for read one byte
                rddata[0] = self.access_reg( addr, write, wrdata[0])
            for idx in range(1, length):
                rddata[idx] = self.access_reg( addr, write, wrdata[idx])

        else: # undefined per spec, assumed high impedance, implemented as byte count
            for idx in range(length):
                rddata[idx] = int('0') + (idx & 63)



        if addr < 0x20:
            if write:
                print("Write")
            else:
                print("Read")
            print("")


        print("TX [", length, "]", sep="", end="")
        for idx in range(length):
            print(" ", rddata[idx], end="")

        print("")
        return length

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("usage: python3 spi-ds3234-rtc.py <address> <port> <spi bus> <chip select>")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]
    busname = sys.argv[3]
    spi_cs = sys.argv[4]

    cm0 = CoreModel("cm0", address, port, "./../../")

    ds3234_0 = ds3234(busname, int(spi_cs), "spi-device")

    try:
        cm0.attach(ds3234_0)
    except NameError as e:
        print(str(e))
        sys.exit(1)

    cm0.cycle_time = 100000

    cm0.start()

    for i in range(3):
        time.sleep(1)

    cm0.stop_event.set()
    cm0.join()