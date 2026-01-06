from coremodel import *
from adxl36x_tick import *
from adxl36x_deff import *

import logging

LOGGER = logging.getLogger(__name__)
LOGGER.setLevel(logging.DEBUG)
logging.basicConfig(level=logging.DEBUG)

FIFO_SIZE   = 2048

SELF_TEST_VALUE = [ [ 1440, 4320 ], [ 1440*2, 4320*2 ], [ 1440*4, 4320*4 ] ] # 360 << 2, 1080 << 2
RANGE_SCALE = [ 1, 2, 4 ]

class adxl36x(CoreModelI2C):
    def __init__(self, busname, address, devname):
        super().__init__(busname = busname, address = address)
        self.devname = devname
        self.regaddr = 0
        self.count = 0
        self.lock = threading.Lock()
        self.tickt = TickTimerT(self, self.name, self.lock)
        self.ticke = TickEngine(self.tickt)

        self.devid_ad = 0
        self.devid_mst = 0
        self.part_id = 0
        self.rev_id = 0
        self.serial_number_2 = 0
        self.serial_number_1 = 0
        self.serial_number_0 = 0
        self.status = [0] * 3
        self.fifo_entries_l = 0
        self.fifo_entries_h = 0
        self.xdata_h = 0
        self.xdata_l = 0
        self.ydata_h = 0
        self.ydata_l = 0
        self.zdata_h = 0
        self.zdata_l = 0
        self.temp_h = 0
        self.temp_l = 0
        self.ex_adc_h = 0
        self.ex_adc_l = 0
        self.soft_reset = 0
        self.thresh_act_h = 0
        self.thresh_act_l = 0
        self.time_act = 0
        self.thresh_inact_h = 0
        self.thresh_inact_l = 0
        self.time_inact_h = 0
        self.time_inact_l = 0
        self.act_inact_ctl = 0
        self.fifo_control = 0
        self.fifo_samples = 0
        self.intmapn_lower = [0]* 2
        self.filter_ctl = 0
        self.power_ctl = 0
        self.self_test = 0
        self.tap_thresh = 0
        self.tap_dur = 0
        self.tap_latent = 0
        self.tap_window = 0
        self.x_offset = 0
        self.y_offset = 0
        self.z_offset = 0
        self.x_sens = 0
        self.y_sens = 0
        self.z_sens = 0
        self.timer_ctl = 0
        self.intmapn_upper = [0]* 2
        self.adc_ctl = 0
        self.temp_ctl = 0
        self.temp_adc_over_thrsh_h = 0
        self.temp_adc_over_thrsh_l = 0
        self.temp_adc_under_thrsh_h = 0
        self.temp_adc_under_thrsh_l = 0
        self.temp_adc_timer = 0
        self.axis_mask = 0
        self.pedometer_step_cnt_h = 0
        self.pedometer_step_cnt_l = 0
        self.pedometer_ctl = 0
        self.pedometer_thres_h = 0
        self.pedometer_thres_l = 0
        self.pedometer_sens_h = 0
        self.pedometer_sens_l = 0
        self.addr = 0
        self.count = 0
        self.stc = 0
        self.tap_sample = 0
        self.tap_state = 0
        self.activity_mode = 0
        self.data_range = 0
        self.odr = 0
        self.tme_init = 0
        self.act_threshold = 0
        self.inact_threshold = 0
        self.temp_threshold_h = 0
        self.temp_threshold_l = 0
        self.time_inact = 0
        self.act_ref = [0] * 3
        self.inact_ref = [0] * 3
        self.time_temp_inact = 0
        self.time_temp_act = 0
        self.sample_count = 0
        self.sample_fifo_wpt = 0
        self.sample_fifo_rpt = 0
        self.read_count = 0
        self.sample_size = 0
        self.fifo_count = 0
        self.sample_fifo = [0] * FIFO_SIZE
        self.fifo_trig = 0
        self.tme_cnt = 0
        self.tap_time = 0
        self.accel = [0] * 3
        self.temp = 0

        self.irq_line = [0] * 2

        self.intpin = [0] * 2

        self.adi_adxl36x_reset()

    def adi_adxl36x_mmio_debug(self, index,  addr,  size,  val, tag):
        name = adi_adxl36x_reg_name(addr)
        if name :
            #LOGGER.debug("[adi-adxl36x:{}] {}{} {}({}) {}: ".format( index, tag, size, hex(addr), name, hex(val)))
            print("[adi-adxl36x:{}] {}{} {}({}) {}: ".format( index, tag, size, hex(addr), name, hex(val)))
            adi_adxl36x_print_fields(addr, val)
            print("")
        else:
            print("[adi-adxl36x:{}] {}{} {} {}".format(index, tag, size, hex(addr), hex(val)))


    def adi_adxl36x_irq_update(self):
        # irql = 0
        # pin_state = 0

        for idx in range(2):
            irql  = bool((self.status[0] & self.intmapn_lower[idx] & 0x7f) | (self.status[1] & self.intmapn_upper[idx] & 0xbf) | (self.status[0] & self.intmapn_upper[idx] & 0x80))

            if irql != self.irq_line[idx]:
                if self.intpin[idx]:
                    pass
                    # pin_state = gpio_pin_get(state->intpin[idx])
                    # gpio_pin_set(state->intpin[idx], pin_state ^ 1)

                self.irq_line[idx] = irql

    def adi_adxl36x_fifo_sample(self):

        fifo_samples = ((self.fifo_control & REG_FIFO_CONTROL_FIFO_SAMPLES) << 6) | self.fifo_samples
        fifo_rsize =  self.sample_fifo_wpt - self.sample_fifo_rpt
        fifo_wsize = FIFO_SIZE - fifo_rsize
        control = self.fifo_control & REG_FIFO_CONTROL_FIFO_MODE_MASK
        temp_buf = [0] * 4


        if control == 1:
            # oldest save mode
            if not fifo_wsize:
                self.status[0] |= REG_STATUS_FIFO_OVER_RUN
                return
        elif control == 2:
            pass
            # stream mode
        elif control == 3:
            # trigger mode
            if(self.sample_count == fifo_samples) or (not self.fifo_trig) or (not fifo_wsize):
                return

        temp_buf[0] = self.accel[0]
        temp_buf[1] = self.accel[0]
        temp_buf[2] = self.accel[0]
        temp_buf[3] = self.temp

        sdc_ctl_bit = (self.adc_ctl & REG_ADC_CTL_FIFO_8_12BIT_MASK) >> REG_ADC_CTL_FIFO_8_12BIT_SHIFT

        if sdc_ctl_bit == 1:
            temp_buf[0] >>= 6
            temp_buf[1] >>= 6
            temp_buf[2] >>= 6
            temp_buf[3] >>= 6

        elif sdc_ctl_bit == 2:
            temp_buf[0] >>= 4
            temp_buf[1] >>= 4
            temp_buf[2] >>= 4
            temp_buf[3] >>= 4
        elif sdc_ctl_bit == 0 or sdc_ctl_bit == 3:
            temp_buf[1] |= 1 << 14
            temp_buf[2] |= 2 << 14
            temp_buf[3] |= 3 << 14

        fif0_control_channel = self.fifo_control & REG_FIFO_CONTROL_CHANNEL_SELECT_MASK

        if fif0_control_channel == REG_FIFO_CONTROL_CHANNEL_SELECT_XYZ:
            if fifo_wsize < 3:
                return

            self.sample_size = 3
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[0]
            self.sample_fifo_wpt += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[1]
            self.sample_fifo_wpt += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[2]
            self.sample_fifo_wpt += 1
            self.sample_count += 1
            self.fifo_count += 3
        elif fif0_control_channel ==  REG_FIFO_CONTROL_CHANNEL_SELECT_X:
            if fifo_wsize < 1:
                return
            self.sample_size = 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[0]
            self.sample_fifo_wpt += 1
            self.sample_count += 1
            self.fifo_count += 1
        elif fif0_control_channel == REG_FIFO_CONTROL_CHANNEL_SELECT_Y:
            if fifo_wsize < 1:
                return

            self.sample_size = 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[1]
            self.sample_fifo_wpt += 1
            self.sample_count += 1
            self.fifo_count += 1
        elif fif0_control_channel == REG_FIFO_CONTROL_CHANNEL_SELECT_Z:
            if fifo_wsize < 1:
                return

            self.sample_size = 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[2]
            self.sample_fifo_wpt  += 1
            self.sample_count  += 1
            self.fifo_count += 1
        elif fif0_control_channel == REG_FIFO_CONTROL_CHANNEL_SELECT_XYZT:
            if fifo_wsize < 4:
                return

            self.sample_size = 4
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[0]
            self.sample_fifo_wpt  += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[1]
            self.sample_fifo_wpt  += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[2]
            self.sample_fifo_wpt  += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[3]
            self.sample_fifo_wpt  += 1
            self.sample_count += 1
            self.fifo_count += 4
        elif fif0_control_channel == REG_FIFO_CONTROL_CHANNEL_SELECT_XT:
            if fifo_wsize < 2:
                return

            self.sample_size = 2
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[0]
            self.sample_fifo_wpt += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[3]
            self.sample_fifo_wpt += 1
            self.sample_count += 1
            self.fifo_count += 2
        elif fif0_control_channel == REG_FIFO_CONTROL_CHANNEL_SELECT_YT:
            if fifo_wsize < 2:
                return

            self.sample_size = 2
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[1]
            self.sample_fifo_wpt += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[3]
            self.sample_fifo_wpt += 1
            self.sample_count += 1
            self.fifo_count += 2
        elif fif0_control_channel == REG_FIFO_CONTROL_CHANNEL_SELECT_ZT:
            if fifo_wsize < 2:
                return

            self.sample_size = 2
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[2]
            self.sample_fifo_wpt +=1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[3]
            self.sample_fifo_wpt += 1
            self.sample_count += 1
            self.fifo_count += 2
        elif fif0_control_channel == REG_FIFO_CONTROL_CHANNEL_SELECT_XYZA:
            if fifo_wsize < 4:
                return

            self.sample_size = 4
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[0]
            self.sample_fifo_wpt += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[1]
            self.sample_fifo_wpt += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[2]
            self.sample_fifo_wpt += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = 0
            self.sample_fifo_wpt += 1
            self.sample_count += 1
            self.fifo_count += 4
        elif fif0_control_channel == REG_FIFO_CONTROL_CHANNEL_SELECT_XA:
            if fifo_wsize < 2:
                return

            self.sample_size = 2
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[0]
            self.sample_fifo_wpt += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = 0
            self.sample_fifo_wpt += 1
            self.sample_count += 1
            self.fifo_count += 2
        elif fif0_control_channel == REG_FIFO_CONTROL_CHANNEL_SELECT_YA:
            if fifo_wsize < 2:
                return

            self.sample_size = 2
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[1]
            self.sample_fifo_wpt += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = 0
            self.sample_fifo_wpt += 1
            self.sample_count += 1
            self.fifo_count += 2
        elif fif0_control_channel == REG_FIFO_CONTROL_CHANNEL_SELECT_ZA:
            if fifo_wsize < 2:
                return

            self.sample_size = 2
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = temp_buf[2]
            self.sample_fifo_wpt += 1
            self.sample_fifo[self.sample_fifo_wpt%FIFO_SIZE] = 0
            self.sample_fifo_wpt += 1
            self.sample_count += 1
            self.fifo_count += 2

        if self.sample_count >= fifo_samples:
            self.status[0] |= REG_STATUS_FIFO_WATER_MARK

    def adi_adxl36x_accel_convert(self):

        # get accel data in m/s^2
        # sensor_get_float2fix(state->vm, 'acel', state->accel, 3, 8); # Q56.8
        # TODO this is EL2 only need to provide or generate data

        for i in range(3):
            self.accel[i] *= 256000000 # Q56.8  256000000  m/s^2 to mm/s^2 * 1000 Q56.8 -> Q48.16
            self.accel[i] += 32768     # Q48.16 32768 rounding 0.5
            self.accel[i] /= 2510502   # Q56.8  2510502 9806.65 mm/s^2  Q48.16 -> Q56.8

            self.accel[i] /= RANGE_SCALE[(self.filter_ctl & REG_FILTER_CTL_RANGE_MASK) >> REG_FILTER_CTL_RANGE_SHIFT] # apply scalar factor
            self.accel[i] >>= 4

        self.xdata_h = (self.accel[0] & 0xff00) >> 8
        self.xdata_l = (self.accel[0] & 0x00fc)
        self.ydata_h = (self.accel[1] & 0xff00) >> 8
        self.ydata_l = (self.accel[1] & 0x00fc)
        self.zdata_h = (self.accel[2] & 0xff00) >> 8
        self.zdata_l = (self.accel[2] & 0x00fc)

        for i in range(3):
            self.accel[i] >>= 2

    def adi_adxl36x_temp_convert(self):

        # sensor_get_float2fix(state->vm, 'temp', &state->temp, 1, 8); # Q56.8
        # TODO this is EL2 only need to provide or generate data

        self.temp -= 6400       # Q56.8 -25
        self.temp *= 13824      # Q48.16 *54
        self.temp >>= 8         # Q56.8
        self.temp += 42240      # Q56.8 +165
        self.temp >>= 6

        self.temp_h = (self.temp & 0xff00) >> 8
        self.temp_l = (self.temp & 0x00fc)
        self.temp >>= 2

    def start(self):
        LOGGER.debug("start")
        self.lock.acquire()

        if self.count == 0:
            if (self.power_ctl & REG_POWER_CTL_MEASURE_MASK) == 2: # measure mode
                self.adi_adxl36x_accel_convert()

            if self.temp_ctl & REG_TEMP_CTL_TEMP_EN:
                self.adi_adxl36x_temp_convert()

        self.lock.release()
        return 1

    def write(self, data):
        length = len(data)
        ret = 0

        self.lock.acquire()

        self.ticke.stop()
        self.tme_cnt = self.ticke.get_value()

        for i in range(length):

            if self.count == 0:
                self.addr = 0x7f & data[i]

                #LOGGER.debug("adxl366 address {}".format( hex(self.addr)))

                self.count += 1
                continue

            # TODO debug
            self.adi_adxl36x_mmio_debug(0, self.addr, length, data[i], "W")

            if self.addr == REG_SOFT_RESET:
                self.soft_reset = data[i] & REG_SOFT_RESET_SOFT_RESET
            elif self.addr == REG_THRESH_ACT_H:
                self.thresh_act_h = data[i] & REG_THRESH_ACT_H_DATA_MASK
            elif self.addr == REG_THRESH_ACT_L:
                self.thresh_act_l = data[i] & REG_THRESH_ACT_L_DATA_MASK
                self.act_threshold = (self.thresh_act_l >> 2) | (self.thresh_act_h << 6)
            elif self.addr == REG_TIME_ACT:
                self.time_act = data[i]
                if ((self.act_inact_ctl & REG_ACT_INACT_CTL_ACT_EN_MASK) == 1) or ((self.act_inact_ctl & REG_ACT_INACT_CTL_ACT_EN_MASK) == 3):
                    # tme_set_comp(&state->tme, 1, state->tme_cnt + state->time_act);
                    self.ticke.set_compare(1, self.tme_cnt + self.time_act)

            elif self.addr == REG_THRESH_INACT_H:
                self.thresh_inact_h = data[i] & REG_THRESH_INACT_H_DATA_MASK
            elif self.addr == REG_THRESH_INACT_L:
                self.thresh_inact_l = data[i] & REG_THRESH_INACT_L_DATA_MASK
                self.inact_threshold = (self.thresh_inact_l >> 2) | (self.thresh_inact_h << 6)
            elif self.addr == REG_TIME_INACT_H:
                self.time_inact_h = data[i]
            elif self.addr == REG_TIME_INACT_L:
                self.time_inact_l = data[i]
                self.time_inact = (self.time_inact_l >> 2) | (self.time_inact_h << 6)
                if((((self.act_inact_ctl & REG_ACT_INACT_CTL_INTACT_EN_MASK) >> REG_ACT_INACT_CTL_INTACT_EN_SHIFT) == 1) or
                        (((self.act_inact_ctl & REG_ACT_INACT_CTL_INTACT_EN_MASK) >> REG_ACT_INACT_CTL_INTACT_EN_SHIFT) == 3)):
                    # tme_set_comp(&state->tme, 2, state->tme_cnt + state->time_inact);
                    self.ticke.set_compare(2, self.tme_cnt + self.time_inact)
            elif self.addr == REG_ACT_INACT_CTL:
                self.act_inact_ctl = data[i] & (REG_ACT_INACT_CTL_REG_READBACK_MASK | REG_ACT_INACT_CTL_LINKLOOP_MASK | REG_ACT_INACT_CTL_INTACT_EN_MASK |
                    REG_ACT_INACT_CTL_ACT_EN_MASK)
                self.activity_mode = (self.act_inact_ctl & REG_ACT_INACT_CTL_LINKLOOP_MASK) >> REG_ACT_INACT_CTL_LINKLOOP_SHIFT
            elif self.addr == REG_FIFO_CONTROL:
                self.fifo_control = data[i] & (REG_FIFO_CONTROL_CHANNEL_SELECT_MASK | REG_FIFO_CONTROL_FIFO_SAMPLES | REG_FIFO_CONTROL_FIFO_MODE_MASK)
            elif self.addr == REG_FIFO_SAMPLES:
                self.fifo_samples = data[i]
            elif self.addr == REG_INTMAP1_LOWER:
                self.intmapn_lower[0] = data[i] & (REG_INTMAP1_LOWER_INT_LOW_INT | REG_INTMAP1_LOWER_AWAKE_INT | REG_INTMAP1_LOWER_INACT_INT |
                    REG_INTMAP1_LOWER_ACT_INT | REG_INTMAP1_LOWER_FIFO_OVERRUN_INT | REG_INTMAP1_LOWER_FIFO_WATER_MARK_INT | REG_INTMAP1_LOWER_FIFO_RDY_INT |
                    REG_INTMAP1_LOWER_DATA_RDY_INT)
                if self.intmapn_lower[0] & REG_INTMAP1_LOWER_INT_LOW_INT:
                    if self.intpin[0]:
                        #gpio_pin_set(state->intpin[0], 1);
                        pass
            elif self.addr == REG_INTMAP2_LOWER:
                self.intmapn_lower[1] = data[i] & (REG_INTMAP2_LOWER_INT_LOW_INT | REG_INTMAP2_LOWER_AWAKE_INT | REG_INTMAP2_LOWER_INACT_INT |
                    REG_INTMAP2_LOWER_ACT_INT | REG_INTMAP2_LOWER_FIFO_OVERRUN_INT | REG_INTMAP2_LOWER_FIFO_WATER_MARK_INT | REG_INTMAP2_LOWER_FIFO_RDY_INT |
                    REG_INTMAP2_LOWER_DATA_RDY_INT)
                if self.intmapn_lower[1] & REG_INTMAP1_LOWER_INT_LOW_INT:
                    if self.intpin[1]:
                        #gpio_pin_set(state->intpin[1], 1);
                        pass
            elif self.addr == REG_FILTER_CTL:
                self.filter_ctl = data[i] & (REG_FILTER_CTL_RANGE_MASK | REG_FILTER_CTL_I2C_HS | REG_FILTER_CTL_EXT_SAMPLE | REG_FILTER_CTL_ODR_MASK)

                self.data_range = (self.filter_ctl & REG_FILTER_CTL_RANGE_MASK) >> REG_FILTER_CTL_RANGE_SHIFT

                filter_ctl_odr = self.filter_ctl & REG_FILTER_CTL_ODR_MASK

                # based around 1000Hz tick
                if filter_ctl_odr == 0:   # 12.5 Hz
                    self.odr = 80
                elif filter_ctl_odr == 1: # 25 Hz
                    self.odr = 40
                elif filter_ctl_odr == 2: # 50 Hz
                    self.odr = 20
                elif filter_ctl_odr == 3: # 100 Hz
                    self.odr = 10
                elif filter_ctl_odr == 4: # 200 Hz
                    self.odr = 5
                elif filter_ctl_odr == 5: # 400 Hz
                    self.odr = 2.5

                self.ticke.set_compare(0, self.odr)
                self.ticke.set_value(0)
            elif self.addr == REG_POWER_CTL:
                self.power_ctl = data[i] & (REG_POWER_CTL_EXT_CLK | REG_POWER_CTL_NOISE_MASK | REG_POWER_CTL_WAKEUP | REG_POWER_CTL_AUTOSLEEP |
                    REG_POWER_CTL_MEASURE_MASK)
            elif self.addr == REG_SELF_TEST:
                self.self_test = data[i] & (REG_SELF_TEST_ST_FORCE | REG_SELF_TEST_ST)
            elif self.addr == REG_TAP_THRESH:
                self.tap_thresh = data[i]
            elif self.addr == REG_TAP_DUR:
                self.tap_dur = data[i]
            elif self.addr == REG_TAP_LATENT:
                self.tap_latent = data[i]
            elif self.addr == REG_TAP_WINDOW:
                self.tap_window = data[i]
            elif self.addr == REG_X_OFFSET:
                self.x_offset = data[i] & REG_X_OFFSET_USER_OFFSET_MASK
            elif self.addr == REG_Y_OFFSET:
                self.y_offset = data[i] & REG_Y_OFFSET_USER_OFFSET_MASK
            elif self.addr == REG_Z_OFFSET:
                self.z_offset = data[i] & REG_Z_OFFSET_USER_OFFSET_MASK
            elif self.addr == REG_X_SENS:
                self.x_sens = data[i] & REG_X_SENS_USER_SENS_MASK
            elif self.addr == REG_Y_SENS:
                self.y_sens = data[i] & REG_Y_SENS_USER_SENS_MASK
            elif self.addr == REG_Z_SENS:
                self.z_sens = data[i] & REG_Z_SENS_USER_SENS_MASK
            elif self.addr == REG_TIMER_CTL:
                self.timer_ctl = data[i] & (REG_TIMER_CTL_WAKEUP_RATE_MASK | REG_TIMER_CTL_TIMER_KEEP_ALIVE_MASK)
            elif self.addr == REG_INTMAP1_UPPER:
                self.intmapn_upper[0] = data[i] & (REG_INTMAP1_UPPER_ERR_FUSE_INT | REG_INTMAP1_UPPER_ERR_USER_REGS_INT | REG_INTMAP1_UPPER_KPLAV_TIMER_INT |
                    REG_INTMAP1_UPPER_TEMP_ADC_HI_INT | REG_INTMAP1_UPPER_TEMP_ADC_LOW_INT | REG_INTMAP1_UPPER_TAP_TWO_INT | REG_INTMAP1_UPPER_TAP_ONE_INT)
            elif self.addr == REG_INTMAPS2_UPPER:
                self.intmapn_upper[1] = data[i] & (REG_INTMAPS2_UPPER_ERR_FUSE_INT | REG_INTMAPS2_UPPER_ERR_USER_REGS_INT | REG_INTMAPS2_UPPER_KPLAV_TIMER_INT |
                    REG_INTMAPS2_UPPER_TEMP_ADC_HI_INT | REG_INTMAPS2_UPPER_TEMP_ADC_LOW_INT | REG_INTMAPS2_UPPER_TAP_TWO_INT | REG_INTMAPS2_UPPER_TAP_ONE_INT)
            elif self.addr == REG_ADC_CTL:
                self.adc_ctl = data[i] & (REG_ADC_CTL_FIFO_8_12BIT_MASK | REG_ADC_CTL_ADC_INACT_EN | REG_ADC_CTL_ADC_ACT_EN | REG_ADC_CTL_ADC_EN)
            elif self.addr == REG_TEMP_CTL:
                self.temp_ctl = data[i] & (REG_TEMP_CTL_NL_COMP_EN | REG_TEMP_CTL_TEMP_INACT_EN | REG_TEMP_CTL_TEMP_ACT_EN | REG_TEMP_CTL_TEMP_EN)
            elif self.addr == REG_TEMP_ADC_OVER_THRSH_H:
                self.temp_adc_over_thrsh_h = data[i] & REG_TEMP_ADC_OVER_THRSH_H_THRESH_MASK
                self.temp_threshold_h = self.temp_adc_over_thrsh_h << 6
            elif self.addr == REG_TEMP_ADC_OVER_THRSH_L:
                self.temp_adc_over_thrsh_l = data[i] & REG_TEMP_ADC_OVER_THRSH_L_THRESH_MASK
                self.temp_threshold_h |= self.temp_adc_over_thrsh_l >> 2
            elif self.addr == REG_TEMP_ADC_UNDER_THRSH_H:
                self.temp_adc_under_thrsh_h = data[i] & REG_TEMP_ADC_UNDER_THRSH_H_THRESH_MASK
                self.temp_threshold_l = self.temp_adc_under_thrsh_h << 6
            elif self.addr == REG_TEMP_ADC_UNDER_THRSH_L:
                self.temp_adc_under_thrsh_l = data[i] & REG_TEMP_ADC_UNDER_THRSH_L_THRESH_MASK
                self.temp_threshold_l |= self.temp_adc_under_thrsh_l >> 2
            elif self.addr == REG_TEMP_ADC_TIMER:
                self.temp_adc_timer = data[i] & (REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_INACT_MASK | REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_ACT_MASK)
                self.time_temp_inact = (self.temp_adc_timer & REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_INACT_MASK) >> REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_INACT_SHIFT
                self.time_temp_act = self.temp_adc_timer & REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_ACT_MASK

                if self.temp_ctl & REG_TEMP_CTL_TEMP_ACT_EN:
                    # tme_set_comp(&state->tme, 4, state->tme_cnt + state->time_temp_act)
                    self.ticke.set_compare(4, self.tme_cnt + self.time_temp_act)

                if self.temp_ctl & REG_TEMP_CTL_TEMP_INACT_EN:
                    self.ticke.set_compare(5, self.tme_cnt + self.time_temp_inact)
            elif self.addr == REG_AXIS_MASK:
                self.axis_mask = data[i] & (REG_AXIS_MASK_TAP_AXIS_MASK | REG_AXIS_MASK_ACT_INACT_Z | REG_AXIS_MASK_ACT_INACT_Y | REG_AXIS_MASK_ACT_INACT_X)
            elif self.addr == REG_PEDOMETER_CTL:
                self.pedometer_ctl = data[i] & (REG_PEDOMETER_CTL_RESET_STEP | REG_PEDOMETER_CTL_RESET_OF | REG_PEDOMETER_CTL_EN)
            elif self.addr == REG_PEDOMETER_THRES_H:
                self.pedometer_thres_h = data[i] & REG_PEDOMETER_THRES_H_THRESHOLD_MASK
            elif self.addr == REG_PEDOMETER_THRES_L:
                self.pedometer_thres_l = data[i] & REG_PEDOMETER_THRES_L_THRESHOLD_MASK
            elif self.addr == REG_PEDOMETER_SENS_H:
                self.pedometer_sens_h = data[i] & REG_PEDOMETER_SENS_H_SENSITIVITY_MASK
            elif self.addr == REG_PEDOMETER_SENS_L:
                self.pedometer_sens_l = data[i] & REG_PEDOMETER_SENS_L_SENSITIVITY_MASK
            else:
                pass

            if self.count:
                self.count += 1
                self.addr += 1
            ret = i

        if (self.power_ctl & REG_POWER_CTL_MEASURE_MASK) == 2:
            self.tme_init = 1
            self.ticke.start()
        else:
            self.tme_init = 0
            self.ticke.stop()

        self.lock.release()
        return ret

    def read(self, data):
        length = len(data)

        ret = 0

        self.lock.acquire()

        for i in range(length):

            if self.addr == REG_DEVID_AD:
                data[i] = self.devid_ad
            elif self.addr == REG_DEVID_MST:
                data[i] = self.devid_mst
            elif self.addr == REG_PART_ID:
                data[i] = self.part_id
            elif self.addr == REG_REV_ID:
                data[i] = self.rev_id
            elif self.addr == REG_SERIAL_NUMBER_2:
                data[i] = self.serial_number_2
            elif self.addr == REG_SERIAL_NUMBER_1:
                data[i] = self.serial_number_1
            elif self.addr == REG_SERIAL_NUMBER_0:
                data[i] = self.serial_number_0
            elif self.addr == REG_XDATA:
                data[i] = self.xdata_h
                if self.self_test & 1:
                    data[i] = (SELF_TEST_VALUE[(self.filter_ctl & REG_FILTER_CTL_RANGE_MASK) >> REG_FILTER_CTL_RANGE_SHIFT][self.stc] & 0xff00) >> 8
                    self.stc += 1
                    if self.stc > 1:
                        self.stc = 0
            elif self.addr == REG_YDATA:
                data[i] = self.ydata_h
            elif self.addr == REG_ZDATA:
                data[i] = self.zdata_h
            elif self.addr == REG_STATUS:
                data[i] = self.status[0]
                if self.activity_mode == 0 or self.activity_mode == 2:
                    # default
                    if self.status[0] & REG_STATUS_ACT:
                        self.status[0] &= ~REG_STATUS_ACT
                        if self.act_inact_ctl & REG_ACT_INACT_CTL_INTACT_EN_MASK:
                            #tme_set_comp(&state->tme, 2, state->tme_cnt + state->time_inact);
                            self.ticke.set_compare(2, self.tme_cnt + self.time_inact)

                    if self.status[0] & REG_STATUS_INACT:
                        self.status[0] &= ~REG_STATUS_INACT
                        if self.act_inact_ctl & REG_ACT_INACT_CTL_ACT_EN_MASK:
                            #tme_set_comp(&state->tme, 1, state->tme_cnt + state->time_act);
                            self.ticke.set_compare(1, self.tme_cnt + self.time_act)
                elif self.activity_mode == 1:
                    # link
                    # set after interrupt clear
                    if self.status[0] & REG_STATUS_ACT:
                        self.status[0] &= ~REG_STATUS_ACT
                        self.status[0] |= REG_STATUS_AWAKE
                        if self.act_inact_ctl & REG_ACT_INACT_CTL_INTACT_EN_MASK:
                            # tme_set_comp(&state->tme, 2, state->tme_cnt + state->time_inact);
                            self.ticke.set_compare(2, self.tme_cnt + self.time_inact)

                    elif self.status[0] & REG_STATUS_INACT:
                        self.status[0] &= ~REG_STATUS_INACT
                        self.status[0] &= ~REG_STATUS_AWAKE
                        if self.act_inact_ctl & REG_ACT_INACT_CTL_ACT_EN_MASK:
                            # tme_set_comp(&state->tme, 1, state->tme_cnt + state->time_act);
                            self.ticke.set_compare(1, self.tme_cnt + self.time_act)
                elif self.activity_mode == 3:
                    # loop
                    self.status[0] &= ~(REG_STATUS_ACT | REG_STATUS_INACT)
                self.status[0] &= ~(REG_STATUS_DATA_READY | REG_STATUS_FIFO_WATER_MARK | REG_STATUS_FIFO_OVER_RUN | REG_STATUS_FIFO_READY | REG_STATUS_ERR_USER_REGS)
            elif self.addr == REG_FIFO_ENTRIES_L:
                data[i] = self.fifo_entries_l
            elif self.addr == REG_FIFO_ENTRIES_H:
                data[i] = self.fifo_entries_h
            elif self.addr == REG_XDATA_H:
                data[i] = self.xdata_h
                if self.self_test & 1:
                    data[i] = (SELF_TEST_VALUE[(self.filter_ctl & REG_FILTER_CTL_RANGE_MASK) >> REG_FILTER_CTL_RANGE_SHIFT][self.stc] & 0xff00) >> 8
            elif self.addr == REG_XDATA_L:
                data[i] = self.xdata_l
                if self.self_test & 1:
                    data[i] = SELF_TEST_VALUE[(self.filter_ctl & REG_FILTER_CTL_RANGE_MASK) >> REG_FILTER_CTL_RANGE_SHIFT][self.stc] & 0xff
                    self.stc += 1
                    if self.stc > 1:
                        self.stc = 0
            elif self.addr == REG_YDATA_H:
                data[i] = self.ydata_h
            elif self.addr == REG_YDATA_L:
                data[i] = self.ydata_l
            elif self.addr == REG_ZDATA_H:
                data[i] = self.zdata_h
            elif self.addr == REG_ZDATA_L:
                data[i] = self.zdata_l
            elif self.addr == REG_TEMP_H:
                data[i] = self.temp_h
            elif self.addr == REG_TEMP_L:
                data[i] = self.temp_l
            elif self.addr == REG_EX_ADC_H:
                data[i] = self.ex_adc_h
            elif self.addr == REG_EX_ADC_L:
                data[i] = self.ex_adc_l
            elif self.addr == REG_I2C_FIFO_DATA:
                if self.fifo_count == 0:
                    pass
                else:
                    adc_ctl_bit = (self.adc_ctl & REG_ADC_CTL_FIFO_8_12BIT_MASK) >> REG_ADC_CTL_FIFO_8_12BIT_SHIFT
                    if adc_ctl_bit == 1:
                    # 8 bit
                        data[i] = self.sample_fifo[self.sample_fifo_rpt%FIFO_SIZE]
                        self.sample_fifo_rpt += 1
                        self.fifo_count -= 1
                    elif adc_ctl_bit == 2:
                    # 12 bit
                        count_check = self.read_count%3
                        if count_check == 0:
                            data[i] = self.sample_fifo[self.sample_fifo_rpt%FIFO_SIZE] >> 4
                        elif count_check == 1:
                            data[i] = (self.sample_fifo[self.sample_fifo_rpt%FIFO_SIZE] & 0xf) << 4
                            self.sample_fifo_rpt += 1
                            self.fifo_count -= 1
                            data[i] |= (self.sample_fifo[self.sample_fifo_rpt%FIFO_SIZE] & 0xf00) >> 8
                        elif count_check == 2:
                            data[i] = self.sample_fifo[self.sample_fifo_rpt%FIFO_SIZE] & 0xff
                            self.sample_fifo_rpt += 1
                            self.fifo_count -= 1
                    elif adc_ctl_bit == 0 or adc_ctl_bit == 3:
                        # 16 bit
                        count_check = self.read_count%2
                        if count_check == 0:
                            data[i] = self.sample_fifo[self.sample_fifo_rpt%FIFO_SIZE] >> 8
                        elif count_check == 1:
                            data[i] = self.sample_fifo[self.sample_fifo_rpt%FIFO_SIZE] & 0xff
                            self.sample_fifo_rpt += 1
                            self.fifo_count -= 1

                    if not (self.fifo_count%self.sample_size):
                        self.sample_count -= 1
                    self.read_count += 1
            elif self.addr == REG_SOFT_RESET:
                data[i] = self.soft_reset
            elif self.addr == REG_THRESH_ACT_H:
                data[i] = self.thresh_act_h
            elif self.addr == REG_THRESH_ACT_L:
                data[i] = self.thresh_act_l
            elif self.addr == REG_TIME_ACT:
                data[i] = self.time_act
            elif self.addr == REG_THRESH_INACT_H:
                data[i] = self.thresh_inact_h
            elif self.addr == REG_THRESH_INACT_L:
                data[i] = self.thresh_inact_l
            elif self.addr == REG_TIME_INACT_H:
                data[i] = self.time_inact_h
            elif self.addr == REG_TIME_INACT_L:
                data[i] = self.time_inact_l
            elif self.addr == REG_ACT_INACT_CTL:
                data[i] = self.act_inact_ctl
            elif self.addr == REG_FIFO_CONTROL:
                data[i] = self.fifo_control
            elif self.addr == REG_FIFO_SAMPLES:
                data[i] = self.fifo_samples
            elif self.addr == REG_INTMAP1_LOWER:
                data[i] = self.intmapn_lower[0]
            elif self.addr == REG_INTMAP2_LOWER:
                data[i] = self.intmapn_lower[1]
            elif self.addr == REG_FILTER_CTL:
                data[i] = self.filter_ctl
            elif self.addr == REG_POWER_CTL:
                data[i] = self.power_ctl
            elif self.addr == REG_SELF_TEST:
                data[i] = self.self_test
            elif self.addr == REG_TAP_THRESH:
                data[i] = self.tap_thresh
            elif self.addr == REG_TAP_DUR:
                data[i] = self.tap_dur
            elif self.addr == REG_TAP_LATENT:
                data[i] = self.tap_latent
            elif self.addr == REG_TAP_WINDOW:
                data[i] = self.tap_window
            elif self.addr == REG_X_OFFSET:
                data[i] = self.x_offset
            elif self.addr == REG_Y_OFFSET:
                data[i] = self.y_offset
            elif self.addr == REG_Z_OFFSET:
                data[i] = self.z_offset
            elif self.addr == REG_X_SENS:
                data[i] = self.x_sens
            elif self.addr == REG_Y_SENS:
                data[i] = self.y_sens
            elif self.addr == REG_Z_SENS:
                data[i] = self.z_sens
            elif self.addr == REG_TIMER_CTL:
                data[i] = self.timer_ctl
            elif self.addr == REG_INTMAP1_UPPER:
                data[i] = self.intmapn_upper[0]
            elif self.addr == REG_INTMAPS2_UPPER:
                data[i] = self.intmapn_upper[1]
            elif self.addr == REG_ADC_CTL:
                data[i] = self.adc_ctl
            elif self.addr == REG_TEMP_CTL:
                data[i] = self.temp_ctl
            elif self.addr == REG_TEMP_ADC_OVER_THRSH_H:
                data[i] = self.temp_adc_over_thrsh_h
            elif self.addr == REG_TEMP_ADC_OVER_THRSH_L:
                data[i] = self.temp_adc_over_thrsh_l
            elif self.addr == REG_TEMP_ADC_UNDER_THRSH_H:
                data[i] = self.temp_adc_under_thrsh_h
            elif self.addr == REG_TEMP_ADC_UNDER_THRSH_L:
                data[i] = self.temp_adc_under_thrsh_l
            elif self.addr == REG_TEMP_ADC_TIMER:
                data[i] = self.temp_adc_timer
            elif self.addr == REG_AXIS_MASK:
                data[i] = self.axis_mask
            elif self.addr == REG_STATUS_COPY:
                data[i] = self.status[0]
            elif self.addr == REG_STATUS_2:
                data[i] = self.status[1]

                if (self.temp_ctl & REG_TEMP_CTL_TEMP_ACT_EN) and (self.status[1] & REG_STATUS_2_TEMP_ADC_HI):
                    self.ticke.set_compare(4, self.tme_cnt + self.time_temp_act)
                    self.status[1] &= ~REG_STATUS_2_TEMP_ADC_HI

                if (self.temp_ctl & REG_TEMP_CTL_TEMP_INACT_EN) and (self.status[1] & REG_STATUS_2_TEMP_ADC_LOW):
                    self.ticke.set_compare( 5, self.tme_cnt + self.time_temp_inact)
                    self.status[1] &= ~REG_STATUS_2_TEMP_ADC_LOW

                self.status[1] &= ~(REG_STATUS_2_TAP_TWO | REG_STATUS_2_TAP_ONE)

            elif self.addr == REG_STATUS_3:
                data[i] = self.status[2]
            elif self.addr == REG_PEDOMETER_STEP_CNT_H:
                data[i] = self.pedometer_step_cnt_h
            elif self.addr == REG_PEDOMETER_STEP_CNT_L:
                data[i] = self.pedometer_step_cnt_l
            elif self.addr == REG_PEDOMETER_CTL:
                data[i] = self.pedometer_ctl
            elif self.addr == REG_PEDOMETER_THRES_H:
                data[i] = self.pedometer_thres_h
            elif self.addr == REG_PEDOMETER_THRES_L:
                data[i] = self.pedometer_thres_l
            elif self.addr == REG_PEDOMETER_SENS_H:
                data[i] = self.pedometer_sens_h
            elif self.addr == REG_PEDOMETER_SENS_L:
                data[i] = self.pedometer_sens_l

            self.adi_adxl36x_mmio_debug(0, self.addr, length, data[i], "r")

            if self.count and (self.addr != REG_I2C_FIFO_DATA):
                self.count += 1
                self.addr += 1
                if self.addr == REG_I2C_FIFO_DATA:
                    self.addr = REG_SOFT_RESET
            ret = i

        self.lock.release()

        return ret

    def stop(self):
        print("stop")
        self.count = 0
        self.addr = 0

    def adi_adxl36x_reset(self):

        self.devid_ad = 0xAD
        self.devid_mst = 0x1D
        self.part_id = 0xF7
        self.rev_id = 0x3 #/366 0x5 367 0x3
        self.serial_number_2 = 0
        self.serial_number_1 = 0
        self.serial_number_0 = 0
        self.status[0] = REG_STATUS_AWAKE
        self.fifo_entries_l = 0
        self.fifo_entries_h = 0
        self.xdata_h = 0
        self.xdata_l = 0
        self.ydata_h = 0
        self.ydata_l = 0
        self.zdata_h = 0
        self.zdata_l = 0
        self.temp_h = 0
        self.temp_l = 0
        self.ex_adc_h = 0
        self.ex_adc_l = 0
        self.soft_reset = 0x52
        self.thresh_act_h = 0
        self.thresh_act_l = 0
        self.time_act = 0
        self.thresh_inact_h = 0
        self.thresh_inact_l = 0
        self.time_inact_h = 0
        self.time_inact_l = 0
        self.act_inact_ctl = 0
        self.fifo_control = 0
        self.fifo_samples = 0x80
        self.intmapn_lower[0] = 0
        self.intmapn_lower[1] = 0
        self.filter_ctl = REG_FILTER_CTL_I2C_HS | (3 << REG_FILTER_CTL_ODR_SHIFT)
        self.power_ctl = 0
        self.self_test = 0
        self.tap_thresh = 0
        self.tap_dur = 0
        self.tap_latent = 0
        self.tap_window = 0
        self.x_offset = 0
        self.y_offset = 0
        self.z_offset = 0
        self.x_sens = 0
        self.y_sens = 0
        self.z_sens = 0
        self.timer_ctl = 0
        self.intmapn_upper[0] = 0
        self.intmapn_upper[1] = 0
        self.adc_ctl = 0
        self.temp_ctl = 0
        self.temp_adc_over_thrsh_h = 0
        self.temp_adc_over_thrsh_l = 0
        self.temp_adc_under_thrsh_h = 0
        self.temp_adc_under_thrsh_l = 0
        self.temp_adc_timer = 0
        self.axis_mask = 0
        self.status[1] = 0
        self.status[2] = 0
        self.pedometer_step_cnt_h = 0
        self.pedometer_step_cnt_l = 0
        self.stc = 0
        self.tap_state = 0
        self.tap_sample = 0
        self.tap_time = 0

        for i in range(3):
            self.accel[i] = 0
            self.act_ref[i] = 0
            self.inact_ref[i] = 0


        self.sample_count = 0
        self.sample_fifo_wpt = 0
        self.sample_fifo_rpt = 0
        self.fifo_trig = 0

        self.sample_fifo = [0] * FIFO_SIZE

        self.tme_init = 0
        self.odr = 80
        self.tme_cnt = 0

        self.ticke.stop()
        self.ticke.set_value(self.tme_cnt)
        self.ticke.set_period(0xffffffff)

        self.ticke.set_compare(0, self.odr)
