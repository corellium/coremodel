#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import print_function
import ctypes
import os, os.path
import sys
import threading
from types import MethodType
import faulthandler
from pathlib import Path
faulthandler.enable()

COREMODEL_UART = 0
COREMODEL_I2C = 1
COREMODEL_SPI = 2
COREMODEL_GPIO = 3
COREMODEL_USBH = 4
COREMODEL_CAN = 5
COREMODEL_INVALID = -1

TYPE_STRING = ["uart", "i2c", "spi", "gpio", "usbh", "can" ]

class coremodel_device_list_t(ctypes.Structure):
    _fields_ = [
        ("type",        ctypes.c_int, 32),
        ("name",        ctypes.c_char_p),
        ("num",         ctypes.c_int, 32)
    ]

UART_TX = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint8))
UART_BRK = ctypes.CFUNCTYPE(None, ctypes.c_void_p)
UART_RXRDY = ctypes.CFUNCTYPE(None, ctypes.c_void_p)

class coremodel_uart_func_t(ctypes.Structure):
    _fields_ = [
        ("tx",    UART_TX),
        ("brk",   UART_BRK),
        ("rxrdy", UART_RXRDY)
    ]

I2C_START = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p)
I2C_WRITE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint8))
I2C_READ = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint8))
I2C_STOP = ctypes.CFUNCTYPE(None, ctypes.c_void_p)

COREMODEL_I2C_START_ACK = 1  # evice must ACK all starts
COREMODEL_I2C_WRITE_ACK = 2  # device must ACK all writes

class coremodel_i2c_func_t(ctypes.Structure):
    _fields_ = [
        ("start", I2C_START),
        ("write", I2C_WRITE),
        ("read",  I2C_READ),
        ("stop",  I2C_STOP)
    ]

SPI_CS = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_uint32)
SPI_XFR = ctypes.CFUNCTYPE(ctypes.c_int32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8))

COREMODEL_SPI_BLOCK = 1  # device must handle >1 byte transfers

class coremodel_spi_func_t(ctypes.Structure):
    _fields_ = [
        ("cs",  SPI_CS),
        ("xfr", SPI_XFR)
    ]

GPIO_NOTIFY = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_int32)

class coremodel_gpio_func_t(ctypes.Structure):
    _fields_ = [
        ("notify", GPIO_NOTIFY)
    ]

USB_RST = ctypes.CFUNCTYPE(None, ctypes.c_void_p)
USB_XFR = ctypes.CFUNCTYPE(ctypes.c_int32, ctypes.c_void_p, ctypes.c_uint8, ctypes.c_uint8, ctypes.c_uint8, ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint32, ctypes.c_uint8)

USB_TKN_OUT     =  0
USB_TKN_IN      =  1
USB_TKN_SETUP   =  2
USB_XFR_NAK     = -1
USB_XFR_STALL   = -2
USB_SPEED_LOW   =  0
USB_SPEED_FULL  =  1
USB_SPEED_HIGH  =  2
USB_SPEED_SUPER =  3

class coremodel_usbh_func_t(ctypes.Structure):
    _fields_ = [
        ("rst", USB_RST),
        ("xfr", USB_XFR)
    ]


CAN_CTRL1_SEC = (1 << 59)
CAN_CTRL1_SDT_SHIFT = 51
CAN_CTRL1_SDT_MASK = (0xFF << CAN_CTRL1_SDT_SHIFT)
CAN_CTRL1_VCID_SHIFT = 43
CAN_CTRL1_VCID_MASK = (0xFF << CAN_CTRL1_VCID_SHIFT)
CAN_CTRL1_PRIO_SHIFT = 32
CAN_CTRL1_PRIO_MASK = (0x7FF << CAN_CTRL1_PRIO_SHIFT)
CAN_CTRL1_AF_SHIFT = 0
CAN_CTRL1_AF_MASK = (0xFFFFFFFF << CAN_CTRL1_AF_SHIFT)


CAN_CTRL_XLF = (1 << 49)
CAN_CTRL_FDF = (1 << 48)
CAN_CTRL_ID_SHIFT = 36
CAN_CTRL_ID_MASK = (0x7FF << CAN_CTRL_ID_SHIFT)
CAN_CTRL_RTR = (1 << 35)
CAN_CTRL_IDE = (1 << 34)
CAN_CTRL_EID_SHIFT = 16
CAN_CTRL_EID_MASK = (0x3FFFF << CAN_CTRL_EID_SHIFT)

CAN_CTRL_ERTR = (1 << 15)
CAN_CTRL_EDL = (1 << 14)
CAN_CTRL_BRS = (1 << 12)
CAN_CTRL_ESI = (1 << 11)
CAN_CTRL_DLC_SHIFT = 0
CAN_CTRL_DLC_MASK = (0x7FF << CAN_CTRL_DLC_SHIFT)

CAN_ACK = 0
CAN_NAK = 1
CAN_STALL = -1

CAN_DATALEN= [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64 ]

CAN_TX = ctypes.CFUNCTYPE(ctypes.c_int32, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint64), ctypes.POINTER(ctypes.c_uint8))
CAN_RXCOMPLETE = ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_int32)

class coremodel_can_func_t(ctypes.Structure):
    _fields_ = [
        ("tx", CAN_TX),
        ("rxcomplete", CAN_RXCOMPLETE)
    ]

class CoreModel(threading.Thread):

    def __init__(self, name, address, port, path):

        super().__init__(name=name)

        self.address = address
        self.port = port
        self.addressport = self.address + ':' + self.port
        self.connection = 1
        self.path = path
        self.attached_objs = list()

        self.cycle_time = 100000 # 100ms
        self.stop_event = threading.Event()
        self.cm = ctypes.c_void_p(None)

        try:
            self.libcm = ctypes.CDLL( self.path + 'libcoremodel.so')
        except OSError:
            print("libcoremodel.so not found path ", self.path)
            self.libcm = None
            sys.exit(0)

        self.libcm.coremodel_connect.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_char_p]
        self.libcm.coremodel_connect.restype = ctypes.c_int

        self.libcm.coremodel_list.argtypes = [ctypes.c_void_p]
        self.libcm.coremodel_list.restype = ctypes.POINTER(coremodel_device_list_t)

        self.libcm.coremodel_free_list.argtypes = [ctypes.POINTER(coremodel_device_list_t)]

        self.libcm.coremodel_disconnect.argtypes = [ctypes.c_void_p]

        self.libcm.coremodel_mainloop.argtypes = [ctypes.c_void_p, ctypes.c_longlong]
        self.libcm.coremodel_mainloop.restype = ctypes.c_int

        self.devlist = None

        self.libcm.coremodel_detach.argtypes = [ctypes.c_void_p]

        self.libcm.coremodel_attach_uart.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(coremodel_uart_func_t), ctypes.c_void_p]
        self.libcm.coremodel_attach_uart.restype = ctypes.c_void_p

        self.libcm.coremodel_uart_rx.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint8)]
        self.libcm.coremodel_uart_rx.restype = ctypes.c_int

        self.libcm.coremodel_attach_i2c.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_uint8, ctypes.POINTER(coremodel_i2c_func_t), ctypes.c_void_p, ctypes.c_uint16]
        self.libcm.coremodel_attach_i2c.restype = ctypes.c_void_p

        self.libcm.coremodel_i2c_ready.argtypes = [ctypes.c_void_p]

        self.libcm.coremodel_attach_spi.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_uint32, ctypes.POINTER(coremodel_spi_func_t), ctypes.c_void_p, ctypes.c_uint16]
        self.libcm.coremodel_attach_spi.restype = ctypes.c_void_p

        self.libcm.coremodel_spi_ready.argtypes = [ctypes.c_void_p]

        self.libcm.coremodel_attach_gpio.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_uint32, ctypes.POINTER(coremodel_gpio_func_t), ctypes.c_void_p]
        self.libcm.coremodel_attach_gpio.restype = ctypes.c_void_p

        self.libcm.coremodel_gpio_set.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_int32]

        self.libcm.coremodel_attach_usbh.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_uint32, ctypes.POINTER(coremodel_usbh_func_t), ctypes.c_void_p, ctypes.c_uint32]
        self.libcm.coremodel_attach_usbh.restype = ctypes.c_void_p

        self.libcm.coremodel_usbh_ready.argtypes = [ctypes.c_void_p, ctypes.c_uint8, ctypes.c_uint8]
        self.libcm.coremodel_usbh_ready.restype = None

        self.libcm.coremodel_attach_can.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.POINTER(coremodel_can_func_t), ctypes.c_void_p]
        self.libcm.coremodel_attach_can.restype = ctypes.c_void_p

        self.libcm.coremodel_can_rx.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint64), ctypes.POINTER(ctypes.c_uint8)]
        self.libcm.coremodel_can_rx.restype = ctypes.c_int32

        self.libcm.coremodel_can_ready.argtypes = [ctypes.c_void_p]
        self.libcm.coremodel_can_ready.restype = None

        self._connect(self.address, self.port)

    def _connect(self, address, port):

        if self.connection == 0:
            print("Connection already established")
            print(self.address, " ", self.port)
            return

        self.address = address
        self.port = port
        self.addressport = self.address + ':' + self.port

        try:
            self.connection = self.libcm.coremodel_connect(ctypes.pointer(self.cm) , ctypes.c_char_p(self.addressport.encode('utf-8')))
        except Exception as e:
            print(str(e))
            sys.exit(1)

        if self.connection:
            sys.exit(1)
        else:
            print("Connection Successful")

        self.devlist = self.libcm.coremodel_list(self.cm)

        if self.devlist is None:
            print("failed to list devices")
            self.libcm.coremodel_disconnect(self.cm)
            sys.exit(1)

    def device_list(self):

        if self.devlist is None:
            print("failed to list devices")
            if self.connection == 0:
                self.libcm.coremodel_disconnect()
            sys.exit(0)

        print("{:<16}{:<16}{:<16}{:<16}".format("Index", "Bus Type", "Name", "Number"))
        print("-" * (16 * 4))
        i = 0
        while self.devlist[i].type != COREMODEL_INVALID:
            print("{:<16}{:<16}{:<16}{:<16}".format(i, TYPE_STRING[self.devlist[i].type], self.devlist[i].name.decode('utf-8'), self.devlist[i].num))
            i += 1

    def _uart_rx(self, byte_data):
        length = ctypes.c_int(len(byte_data))
        data = (ctypes.c_uint8 * len(byte_data))(*byte_data)

        if self.handle is None:
            return -1
        ret = self._rx(self.handle, length.value, data)
        c_ret = ctypes.c_int32(ret)
        return c_ret.value

    def _i2c_ready(self):

        if self.handle is None:
            return
        self._ready(self.handle)

    def _spi_ready(self):

        if self.handle is None:
            return
        self._ready(self.handle)

    def _gpio_set(self, driven, mvolt):

        if self.handle is None:
            return
        self.driven = driven
        self.mvolt = mvolt
        c_driven = ctypes.c_uint32(driven)
        c_mvolt = ctypes.c_int32(mvolt)
        self._set(self.handle, c_driven.value, c_mvolt.value)

    def _usbh_ready(self, ep, tkn):

        if self.handle is None:
            return
        self.ep = ep
        self.tkn = tkn
        c_ep = ctypes.c_uint8(ep)
        c_tkn = ctypes.c_uint8(tkn)
        self._ready(self.handle, c_ep.value, c_tkn.value)

    def _can_ready(self):

        if self.handle is None:
            return
        self._ready(self.handle)

    def _can_rx(self, ctrl, data):

        if self.handle is None:
            return 0
        c_ctrl = (ctypes.c_uint64 * len(ctrl))(*ctrl)
        dlen = CAN_DATALEN[(ctrl[0] & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT]
        c_data = (ctypes.c_uint8 * dlen)(*data)
        ret = self._rx(self.handle, c_ctrl, c_data)
        c_ret = ctypes.c_int32(ret)
        return c_ret.value

    def attach(self, obj):

        i = 0
        while self.devlist[i].type != COREMODEL_INVALID:
            if self.devlist[i].name.decode('utf-8') == obj.name:
                break
            i += 1
        else:
            raise NameError("Device not found")

        if obj in self.attached_objs:
            raise NameError("Device already attached")

        py_obj = id(obj)
        priv = ctypes.c_void_p(py_obj)

        if obj.type == COREMODEL_UART:
            print("Attaching Uart")
            obj.uart_func = coremodel_uart_func_t( tx = UART_TX(obj._tx), brk = UART_BRK(obj._brk), rxrdy = UART_RXRDY(obj._rxrdy))

            obj.handle = self.libcm.coremodel_attach_uart(self.cm, obj.name.encode('utf-8'), ctypes.pointer(obj.uart_func), priv)

            if obj.handle is None:
                raise NameError("Attach Uart Failed")
            else:
                obj._rx = self.libcm.coremodel_uart_rx
                obj.rx = MethodType(CoreModel._uart_rx, obj)
                self.attached_objs.append(obj)
                obj.cm = self

        elif obj.type == COREMODEL_I2C:
            print("Attaching I2C")
            obj.i2c_func = coremodel_i2c_func_t(start = I2C_START(obj._start), write = I2C_WRITE(obj._write), read = I2C_READ(obj._read), stop = I2C_STOP(obj._stop))
            addr = ctypes.c_uint8(obj.address)
            flags = ctypes.c_uint16(obj.flags)

            obj.handle = self.libcm.coremodel_attach_i2c(self.cm, obj.name.encode('utf-8'), addr.value, ctypes.pointer(obj.i2c_func), priv, flags.value)

            if obj.handle is None:
                raise NameError("Attach I2C Failed")
            else:
                obj._ready = self.libcm.coremodel_i2c_ready
                obj.ready = MethodType(CoreModel._i2c_ready, obj)
                self.attached_objs.append(obj)
                obj.cm = self

        elif obj.type == COREMODEL_SPI:
            print("Attaching SPI")
            obj.spi_func = coremodel_spi_func_t(cs = SPI_CS(obj._cs), xfr = SPI_XFR(obj._xfr))
            cs = ctypes.c_uint32(obj.cs)
            flags = ctypes.c_uint16(obj.flags)

            obj.handle = self.libcm.coremodel_attach_spi(self.cm, obj.name.encode('utf-8'), cs.value, ctypes.pointer(obj.spi_func), priv,flags.value)

            if obj.handle is None:
                raise NameError("Attach SPI Failed")
            else:
                obj._ready = self.libcm.coremodel_spi_ready
                obj.ready = MethodType(CoreModel._spi_ready, obj)
                self.attached_objs.append(obj)
                obj.cm = self
        elif obj.type == COREMODEL_GPIO:
            obj.gpio_func = coremodel_gpio_func_t(notify = GPIO_NOTIFY(obj._notify))
            pin = ctypes.c_uint32(obj.pin)
            obj.handle = self.libcm.coremodel_attach_gpio(self.cm, obj.name.encode('utf-8'), pin.value, ctypes.pointer(obj.gpio_func), priv)
            if obj.handle is None:
                raise NameError("Attach GPIO Failed")
            else:
                obj._set = self.libcm.coremodel_gpio_set
                obj.set = MethodType(CoreModel._gpio_set, obj)
                self.attached_objs.append(obj)
                obj.cm = self
        elif obj.type == COREMODEL_USBH:
            print("Attaching USBH")
            obj.usbh_func = coremodel_usbh_func_t( rst = USB_RST(obj._rst), xfr = USB_XFR(obj._xfr))
            port = ctypes.c_uint32(obj.port)
            speed = ctypes.c_uint32(obj.speed)
            obj.handle = self.libcm.coremodel_attach_usbh(self.cm, obj.name.encode('utf-8'), port.value, ctypes.pointer(obj.usbh_func), priv, speed.value)
            if obj.handle is None:
                raise NameError("Attach USBH Failed")
            else:
                obj._ready = self.libcm.coremodel_usbh_ready
                obj.ready = MethodType(CoreModel._usbh_ready, obj)
                self.attached_objs.append(obj)
                obj.cm = self
        elif obj.type == COREMODEL_CAN:
            print("Attaching CAN")
            obj.can_func = coremodel_can_func_t(tx = CAN_TX(obj._tx), rxcomplete = CAN_RXCOMPLETE(obj._rxcomplete))
            obj.handle = self.libcm.coremodel_attach_can(self.cm, obj.name.encode('utf-8'), ctypes.pointer(obj.can_func), priv)
            if obj.handle is None:
                raise NameError("Attach CAN Failed")
            else:
                obj._ready = self.libcm.coremodel_can_ready
                obj._rx = self.libcm.coremodel_can_rx
                obj.ready = MethodType(CoreModel._can_ready, obj)
                obj.rx = MethodType(CoreModel._can_rx, obj)
                self.attached_objs.append(obj)
                obj.cm = self
        elif obj.type == COREMODEL_INVALID:
            raise NameError("Object Type Invalid")
        else:
            raise NameError("Object Type not Supported")

    def detach(self, obj):
        if obj.handle is not None:
            self.libcm.coremodel_detach(obj.handle)
            self.attached_objs.remove(obj)

    def run(self):
        while not self.stop_event.is_set():
            ret = 0
            try:
                if self.cycle_time == -1:
                    ret = self.libcm.coremodel_mainloop(self.cm, ctypes.c_longlong(300000))
                else:
                    ret = self.libcm.coremodel_mainloop(self.cm, ctypes.c_longlong(self.cycle_time))
                if ret < 0:
                    sys.exit(0)
            except KeyboardInterrupt:
                sys.exit(0)

    def mainloop(self):
        try:
            ret = self.libcm.coremodel_mainloop(self.cm, ctypes.c_longlong(self.cycle_time))
        except KeyboardInterrupt:
            sys.exit(0)
        return ret

    def __del__(self):
        if self.libcm is not None:
            if self.attached_objs is not None:
                for obj in self.attached_objs:
                    self.libcm.coremodel_detach(obj.handle)
            if self.devlist is not None:
                self.libcm.coremodel_free_list(self.devlist)
            if self.connection == 0:
               self.libcm.coremodel_disconnect(self.cm)
               print("Connection ", self.addressport, " Closed")

class CoreModelUart():
    def __init__(self, busname):

        self.name = busname
        self.type = COREMODEL_UART
        self.handle = None
        self.cm = None

    @staticmethod
    def _tx(priv, len, datap):
        py_obj = ctypes.cast(priv, ctypes.py_object).value
        byte_data = ctypes.string_at(datap, len)
        ret = py_obj.tx(byte_data)
        c_ret = ctypes.c_int(ret)
        return c_ret.value

    def tx(self, data):
        length_of_data_consumed = len(data)
        print(data.decode('utf-8') , end= '')

        return length_of_data_consumed

    @staticmethod
    def _brk(priv):
        py_obj = ctypes.cast(priv, ctypes.py_object).value
        py_obj.brk()

    def brk(self):
        pass

    @staticmethod
    def _rxrdy(priv):
        py_obj = ctypes.cast(priv, ctypes.py_object).value
        py_obj.rxrdy()

    def rxrdy(self):
        pass

    def _rx(self):
        pass #defined by coremodel

    def rx(self, byte_data):
        pass #defined by coremodel

    def __del__(self):
        pass

class CoreModelI2C():
    def __init__(self, busname, address):
        self.name = busname
        self.type = COREMODEL_I2C
        self.flags = 0
        self.address = address
        self.handle = None
        self.cm = None

    @staticmethod
    def _start(obj):
        py_obj = ctypes.cast(obj, ctypes.py_object).value
        ret = py_obj.start()
        c_ret = ctypes.c_int(ret)
        return c_ret.value

    def start(self):
        return 1

    @staticmethod
    def _write(obj, len, data):
        py_obj = ctypes.cast(obj, ctypes.py_object).value
        byte_data = ctypes.string_at(data, len)
        ret = py_obj.write(byte_data)
        c_ret = ctypes.c_int(ret)
        return c_ret.value

    def write(self, data):
        length_of_data_consumed = len(data)
        print("WRITE len [", length_of_data_consumed, "]")
        for idx, byte in enumerate(data):
            print("WRITE idx [", idx, "] value", hex(byte))
        return length_of_data_consumed

    @staticmethod
    def _read(obj, length, data):
        py_obj = ctypes.cast(obj, ctypes.py_object).value
        c_data = (ctypes.c_uint8 * length).from_address(ctypes.addressof(data.contents))
        ret = py_obj.read(c_data)
        c_ret = ctypes.c_int(ret)
        return c_ret.value

    def read(self, data):
        length_of_data_consumed = len(data)
        print("READ len [", length_of_data_consumed,"]", "register", hex(self.regaddr))
        for idx in range(length_of_data_consumed):
            # byte array
            data[idx] = 0xa0 + (idx & 0x3f)
        return length_of_data_consumed

    @staticmethod
    def _stop(obj):
        py_obj = ctypes.cast(obj, ctypes.py_object).value
        py_obj.stop()

    def stop(self):
        pass

    def _ready(self):
        pass #defined by coremodel

    def ready(self):
        pass #defined by coremodel

    def __del__(self):
        pass

class CoreModelSpi():
    def __init__(self, busname, cs):
        self.name = busname
        self.type = COREMODEL_SPI
        self.cs = cs
        self.flags = 0
        self.handle = None
        self.cm = None

    @staticmethod
    def _cs(obj, csel):
        py_obj = ctypes.cast(obj, ctypes.py_object).value
        c_csel = ctypes.c_int(csel)
        py_obj.cs(c_csel.value)

    def cs(self, csel):
        pass

    @staticmethod
    def _xfr(obj, length, wrdata, rddata):
        py_obj = ctypes.cast(obj, ctypes.py_object).value
        wrbyte_data = (ctypes.c_uint8 * length).from_address(ctypes.addressof(wrdata.contents))
        rdbyte_data = (ctypes.c_uint8 * length).from_address(ctypes.addressof(rddata.contents))
        ret = py_obj.xfr(wrbyte_data, rdbyte_data)
        c_ret = ctypes.c_int(ret)
        return c_ret.value

    def xfr(self, wrdata, rddata):
        return len(rddata)

    def _ready(self):
        pass  #defined by coremodel

    def ready(self):
        pass #defined by coremodel

    def __del__(self):
        pass

class CoreModelGpio():
    def __init__(self, busname , pin):
        self.name = busname
        self.type = COREMODEL_GPIO
        self.pin = pin
        self.handle = None
        self.cm = None

    @staticmethod
    def _notify(obj, mvolt):
        py_obj = ctypes.cast(obj, ctypes.py_object).value
        c_mvolt = ctypes.c_int(mvolt)
        py_obj.notify(c_mvolt.value)

    def notify(self, mvolt):
        pass

    def _set(self):
        pass  #defined by coremodel

    def set(self, mvolt, driven):
        pass  #defined by coremodel

    def __del__(self):
        pass

class CoreModelUsbh():
    def __init__(self, busname, port, speed):
        self.name = busname
        self.type = COREMODEL_USBH
        self.port = port
        self.speed = speed
        self.handle = None
        self.cm = None

    @staticmethod
    def _rst(obj):
        py_obj = ctypes.cast(obj, ctypes.py_object).value
        py_obj.rst()

    def rst(self):
        pass

    @staticmethod
    def _xfr(obj, dev, ep, tkn, buf, size, end):
        py_obj = ctypes.cast(obj, ctypes.py_object).value
        c_dev = ctypes.c_uint8(dev)
        c_ep = ctypes.c_uint8(ep)
        c_tkn = ctypes.c_uint8(tkn)
        c_size = ctypes.c_uint32(size)
        c_end = ctypes.c_uint8(end)
        c_buf = (ctypes.c_uint8 * c_size.value).from_address(ctypes.addressof(buf.contents))
        ret = py_obj.xfr(c_dev.value, c_ep.value, c_tkn.value, c_buf, c_size.value, c_end)
        c_ret = ctypes.c_int(ret)
        return c_ret.value

    def xfr(self, dev, ep, tkn, buf, size, end):
        pass

    def _ready(self):
        pass  #defined by coremodel

    def ready(self):
        pass  #defined by coremodel

    def __del__(self):
        pass

class CoreModelCan():
    def __init__(self, busname):
        self.name = busname
        self.type = COREMODEL_CAN
        self.handle = None
        self.cm = None

    @staticmethod
    def _tx(obj, ctrl, data):
        py_obj = ctypes.cast(obj, ctypes.py_object).value
        c_ctrl = (ctypes.c_uint64 * 2).from_address(ctypes.addressof(ctrl.contents))
        dlen = CAN_DATALEN[(c_ctrl[0] & CAN_CTRL_DLC_MASK) >> CAN_CTRL_DLC_SHIFT]
        c_data = (ctypes.c_uint8 * dlen).from_address(ctypes.addressof(data.contents))
        ret = py_obj.tx(c_ctrl, c_data)
        c_ret = ctypes.c_int(ret)
        return c_ret.value

    def tx(self, ctrl, data):
        pass

    @staticmethod
    def _rxcomplete(obj, nak):
        py_obj = ctypes.cast(obj, ctypes.py_object).value
        c_nak = ctypes.c_int32(nak)
        py_obj.rxcomplete(c_nak.value)

    def rxcomplete(self, nak):
        pass

    def _ready(self):
        pass  #defined by coremodel

    def ready(self):
        pass  #defined by coremodel

    def _rx(self):
        pass #defined by coremodel

    def rx(self):
        pass #defined by coremodel

    def __del__(self):
        pass