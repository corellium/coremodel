import threading
import time
from datetime import datetime
from types import *

class TickTimerT:
    def __init__(self, priv, name, lock):
        self.name = name

        if priv is None:
            raise TypeError("Expected an object, but got None")
        elif priv.compare_cb is None:
            raise TypeError("Expected an object with compare_cb(self, icomp), but got None")
        elif priv.period_cb is None:
            raise TypeError("Expected an object with period_cb(self), but got None")

        self.priv = priv

        if not isinstance(lock, type(threading.Lock())):
            raise TypeError(f"Expected an instance of ticktimert, but got {type(lock).__name__}")
        self.lock = lock

    def _compare_cb(self, icomp):
        if self.priv.compare_cb is None:
            return
        self.priv.compare_cb(icomp)

    def _period_cb(self):
        if self.priv.period_cb is None:
            return
        self.priv.period_cb()

class TickEngine:
    def __init__(self, ticktimert):
        if not isinstance(ticktimert, TickTimerT):
            raise TypeError(f"Expected an instance of ticktimert, but got {type(TickTimerT).__name__}")
        self.ticktimert = ticktimert
        self.interval = 1/1000
        self.current_value = 0  # The value to be compared
        self.timer = None
        self._running = False
        self.icomp = [-1]*8
        self.compare = [-1]*8
        self.frequency = 0
        self.period = 0
        self.inner_lock = threading.Lock()

    def set_frequency(self, frequency):
        self.frequency = frequency
        self.interval = 1/frequency


    def set_period(self, period):
        self.period = period

    def set_compare(self, icomp, comp):
        if icomp < 0 or icomp >= 8:
            return
        self.icomp[icomp] = 1
        self.compare[icomp] = comp

    def set_value(self, value):
        self.current_value = value

    def get_value(self):
        value = self.current_value
        return value

    def _tick(self):
        self.ticktimert.lock.acquire()
        self.current_value += 1

        for idx, i in enumerate(self.icomp):
            if i == -1:
                continue
            if self.compare[idx] == -1:
                continue
            if self.current_value >= self.compare[idx]:
                self.ticktimert._compare_cb(idx)
        if self.current_value >= self.period:
            self.ticktimert._period_cb()

        self.ticktimert.lock.release()

        if self._running:
            self.timer = threading.Timer(self.interval, self._tick)
            self.timer.start()

    def start(self):
        if not self._running:
            self._running = True
            self.timer = threading.Timer(self.interval, self._tick)
            self.timer.start()
            print(f"Timer started with interval {self.interval}")

    def stop(self):
        if self._running:
            self._running = False
            if self.timer:
                self.timer.cancel()
            print("Timer stopped.")