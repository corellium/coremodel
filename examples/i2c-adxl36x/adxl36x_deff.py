
REG_DEVID_AD                                    = 0x00
REG_DEVID_MST                                   = 0x01
REG_PART_ID                                     = 0x02
REG_REV_ID                                      = 0x03
REG_SERIAL_NUMBER_2                             = 0x05
REG_SERIAL_NUMBER_2_BIT16                       = 0x00000001
REG_SERIAL_NUMBER_1                             = 0x06
REG_SERIAL_NUMBER_0                             = 0x07
REG_XDATA                                       = 0x08
REG_YDATA                                       = 0x09
REG_ZDATA                                       = 0x0a
REG_STATUS                                      = 0x0b
REG_STATUS_ERR_USER_REGS                        = 0x00000080
REG_STATUS_AWAKE                                = 0x00000040
REG_STATUS_INACT                                = 0x00000020
REG_STATUS_ACT                                  = 0x00000010
REG_STATUS_FIFO_OVER_RUN                        = 0x00000008
REG_STATUS_FIFO_WATER_MARK                      = 0x00000004
REG_STATUS_FIFO_READY                           = 0x00000002
REG_STATUS_DATA_READY                           = 0x00000001
REG_FIFO_ENTRIES_L                              = 0x0c
REG_FIFO_ENTRIES_H                              = 0x0d
REG_XDATA_H                                     = 0x0e
REG_XDATA_L                                     = 0x0f
REG_XDATA_L_DATA_MASK                           = 0x000000fc
REG_XDATA_L_DATA_SHIFT                          =  2
REG_YDATA_H                                     = 0x10
REG_YDATA_L                                     = 0x11
REG_YDATA_L_DATA_MASK                           = 0x000000fc
REG_YDATA_L_DATA_SHIFT                          = 2
REG_ZDATA_H                                     = 0x12
REG_ZDATA_L                                     = 0x13
REG_ZDATA_L_DATA_MASK                           = 0x000000fc
REG_ZDATA_L_DATA_SHIFT                          = 2
REG_TEMP_H                                      = 0x14
REG_TEMP_L                                      = 0x15
REG_TEMP_L_DATA_MASK                            = 0x000000fc
REG_TEMP_L_DATA_SHIFT                           = 2
REG_EX_ADC_H                                    = 0x16
REG_EX_ADC_L                                    = 0x17
REG_EX_ADC_L_DATA_MASK                          = 0x000000fc
REG_EX_ADC_L_DATA_SHIFT                         = 2
REG_I2C_FIFO_DATA                               = 0x18
REG_SOFT_RESET                                  = 0x1f
REG_SOFT_RESET_SOFT_RESET                       = 0x00000002
REG_THRESH_ACT_H                                = 0x20
REG_THRESH_ACT_H_DATA_MASK                      = 0x0000007f
REG_THRESH_ACT_H_DATA_SHIFT                     = 0
REG_THRESH_ACT_L                                = 0x21
REG_THRESH_ACT_L_DATA_MASK                      = 0x000000fc
REG_THRESH_ACT_L_DATA_SHIFT                     = 2
REG_TIME_ACT                                    = 0x22
REG_THRESH_INACT_H                              = 0x23
REG_THRESH_INACT_H_DATA_MASK                    = 0x0000007f
REG_THRESH_INACT_H_DATA_SHIFT                   = 0
REG_THRESH_INACT_L                              = 0x24
REG_THRESH_INACT_L_DATA_MASK                    = 0x000000fc
REG_THRESH_INACT_L_DATA_SHIFT                   = 2
REG_TIME_INACT_H                                = 0x25
REG_TIME_INACT_L                                = 0x26
REG_ACT_INACT_CTL                               = 0x27
REG_ACT_INACT_CTL_REG_READBACK_MASK             = 0x000000c0
REG_ACT_INACT_CTL_REG_READBACK_SHIFT            = 6
REG_ACT_INACT_CTL_LINKLOOP_MASK                 = 0x00000030
REG_ACT_INACT_CTL_LINKLOOP_SHIFT                = 4
REG_ACT_INACT_CTL_LINKLOOP_DEFAULT1             = 0x00000000
REG_ACT_INACT_CTL_LINKLOOP_LINKED               = 0x00000010
REG_ACT_INACT_CTL_LINKLOOP_DEFAULT2             = 0x00000020
REG_ACT_INACT_CTL_LINKLOOP_LOOPED               = 0x00000030
REG_ACT_INACT_CTL_INTACT_EN_MASK                = 0x0000000c
REG_ACT_INACT_CTL_INTACT_EN_SHIFT               = 2
REG_ACT_INACT_CTL_ACT_EN_MASK                   = 0x00000003
REG_ACT_INACT_CTL_ACT_EN_SHIFT                  = 0
REG_FIFO_CONTROL                                = 0x28
REG_FIFO_CONTROL_CHANNEL_SELECT_MASK            = 0x00000078
REG_FIFO_CONTROL_CHANNEL_SELECT_SHIFT           = 3
REG_FIFO_CONTROL_CHANNEL_SELECT_XYZ             = 0x00000000
REG_FIFO_CONTROL_CHANNEL_SELECT_X               = 0x00000008
REG_FIFO_CONTROL_CHANNEL_SELECT_Y               = 0x00000010
REG_FIFO_CONTROL_CHANNEL_SELECT_Z               = 0x00000018
REG_FIFO_CONTROL_CHANNEL_SELECT_XYZT            = 0x00000020
REG_FIFO_CONTROL_CHANNEL_SELECT_XT              = 0x00000028
REG_FIFO_CONTROL_CHANNEL_SELECT_YT              = 0x00000030
REG_FIFO_CONTROL_CHANNEL_SELECT_ZT              = 0x00000038
REG_FIFO_CONTROL_CHANNEL_SELECT_XYZA            = 0x00000040
REG_FIFO_CONTROL_CHANNEL_SELECT_XA              = 0x00000048
REG_FIFO_CONTROL_CHANNEL_SELECT_YA              = 0x00000050
REG_FIFO_CONTROL_CHANNEL_SELECT_ZA              = 0x00000058
REG_FIFO_CONTROL_FIFO_SAMPLES                   = 0x00000004
REG_FIFO_CONTROL_FIFO_MODE_MASK                 = 0x00000003
REG_FIFO_CONTROL_FIFO_MODE_DISABLED             = 0x00000000
REG_FIFO_CONTROL_FIFO_MODE_OLDEST               = 0x00000001
REG_FIFO_CONTROL_FIFO_MODE_STREAM               = 0x00000002
REG_FIFO_CONTROL_FIFO_MODE_TRIGGER              = 0x00000003
REG_FIFO_SAMPLES                                =  0x29
REG_INTMAP1_LOWER                               =  0x2a
REG_INTMAP1_LOWER_INT_LOW_INT                   =  0x00000080
REG_INTMAP1_LOWER_AWAKE_INT                     = 0x00000040
REG_INTMAP1_LOWER_INACT_INT                     =  0x00000020
REG_INTMAP1_LOWER_ACT_INT                       =  0x00000010
REG_INTMAP1_LOWER_FIFO_OVERRUN_INT              = 0x00000008
REG_INTMAP1_LOWER_FIFO_WATER_MARK_INT           = 0x00000004
REG_INTMAP1_LOWER_FIFO_RDY_INT                  = 0x00000002
REG_INTMAP1_LOWER_DATA_RDY_INT                  = 0x00000001
REG_INTMAP2_LOWER                               = 0x2b
REG_INTMAP2_LOWER_INT_LOW_INT                   = 0x00000080
REG_INTMAP2_LOWER_AWAKE_INT                     = 0x00000040
REG_INTMAP2_LOWER_INACT_INT                     = 0x00000020
REG_INTMAP2_LOWER_ACT_INT                       = 0x00000010
REG_INTMAP2_LOWER_FIFO_OVERRUN_INT              = 0x00000008
REG_INTMAP2_LOWER_FIFO_WATER_MARK_INT           = 0x00000004
REG_INTMAP2_LOWER_FIFO_RDY_INT                  = 0x00000002
REG_INTMAP2_LOWER_DATA_RDY_INT                  = 0x00000001
REG_FILTER_CTL                                  = 0x2c
REG_FILTER_CTL_RANGE_MASK                       = 0x000000c0
REG_FILTER_CTL_RANGE_SHIFT                      = 6
REG_FILTER_CTL_I2C_HS                           = 0x00000020
REG_FILTER_CTL_EXT_SAMPLE                       = 0x00000008
REG_FILTER_CTL_ODR_MASK                         = 0x00000007
REG_FILTER_CTL_ODR_SHIFT                        = 0
REG_POWER_CTL                                   = 0x2d
REG_POWER_CTL_EXT_CLK                           = 0x00000040
REG_POWER_CTL_NOISE_MASK                        = 0x00000030
REG_POWER_CTL_NOISE_SHIFT                       = 4
REG_POWER_CTL_WAKEUP                            = 0x00000008
REG_POWER_CTL_AUTOSLEEP                         = 0x00000004
REG_POWER_CTL_MEASURE_MASK                      = 0x00000003
REG_POWER_CTL_MEASURE_SHIFT                     = 0
REG_SELF_TEST                                   = 0x2e
REG_SELF_TEST_ST_FORCE                          = 0x00000002
REG_SELF_TEST_ST                                = 0x00000001
REG_TAP_THRESH                                  = 0x2f
REG_TAP_DUR                                     = 0x30
REG_TAP_LATENT                                  = 0x31
REG_TAP_WINDOW                                  = 0x32
REG_X_OFFSET                                    = 0x33
REG_X_OFFSET_USER_OFFSET_MASK                   = 0x0000001f
REG_X_OFFSET_USER_OFFSET_SHIFT                  = 0
REG_Y_OFFSET                                    = 0x34
REG_Y_OFFSET_USER_OFFSET_MASK                   = 0x0000001f
REG_Y_OFFSET_USER_OFFSET_SHIFT                  = 0
REG_Z_OFFSET                                    = 0x35
REG_Z_OFFSET_USER_OFFSET_MASK                   = 0x0000001f
REG_Z_OFFSET_USER_OFFSET_SHIFT                  = 0
REG_X_SENS                                      = 0x36
REG_X_SENS_USER_SENS_MASK                       = 0x0000003f
REG_X_SENS_USER_SENS_SHIFT                      = 0
REG_Y_SENS                                      = 0x37
REG_Y_SENS_USER_SENS_MASK                       = 0x0000003f
REG_Y_SENS_USER_SENS_SHIFT                      = 0
REG_Z_SENS                                      = 0x38
REG_Z_SENS_USER_SENS_MASK                       = 0x0000003f
REG_Z_SENS_USER_SENS_SHIFT                      = 0
REG_TIMER_CTL                                   = 0x39
REG_TIMER_CTL_WAKEUP_RATE_MASK                  = 0x000000c0
REG_TIMER_CTL_WAKEUP_RATE_SHIFT                 = 6
REG_TIMER_CTL_TIMER_KEEP_ALIVE_MASK             = 0x0000001f
REG_TIMER_CTL_TIMER_KEEP_ALIVE_SHIFT            = 0
REG_INTMAP1_UPPER                               = 0x3a
REG_INTMAP1_UPPER_ERR_FUSE_INT                  = 0x00000080
REG_INTMAP1_UPPER_ERR_USER_REGS_INT             = 0x00000040
REG_INTMAP1_UPPER_KPLAV_TIMER_INT               = 0x00000010
REG_INTMAP1_UPPER_TEMP_ADC_HI_INT               = 0x00000008
REG_INTMAP1_UPPER_TEMP_ADC_LOW_INT              = 0x00000004
REG_INTMAP1_UPPER_TAP_TWO_INT                   = 0x00000002
REG_INTMAP1_UPPER_TAP_ONE_INT                   = 0x00000001
REG_INTMAPS2_UPPER                              = 0x3b
REG_INTMAPS2_UPPER_ERR_FUSE_INT                 = 0x00000080
REG_INTMAPS2_UPPER_ERR_USER_REGS_INT            = 0x00000040
REG_INTMAPS2_UPPER_KPLAV_TIMER_INT              = 0x00000010
REG_INTMAPS2_UPPER_TEMP_ADC_HI_INT              = 0x00000008
REG_INTMAPS2_UPPER_TEMP_ADC_LOW_INT             = 0x00000004
REG_INTMAPS2_UPPER_TAP_TWO_INT                  = 0x00000002
REG_INTMAPS2_UPPER_TAP_ONE_INT                  = 0x00000001
REG_ADC_CTL                                     = 0x3c
REG_ADC_CTL_FIFO_8_12BIT_MASK                   = 0x000000c0
REG_ADC_CTL_FIFO_8_12BIT_SHIFT                  = 6
REG_ADC_CTL_ADC_INACT_EN                        = 0x00000008
REG_ADC_CTL_ADC_ACT_EN                          = 0x00000002
REG_ADC_CTL_ADC_EN                              = 0x00000001
REG_TEMP_CTL                                    = 0x3d
REG_TEMP_CTL_NL_COMP_EN                         = 0x00000080
REG_TEMP_CTL_TEMP_INACT_EN                      = 0x00000008
REG_TEMP_CTL_TEMP_ACT_EN                        = 0x00000002
REG_TEMP_CTL_TEMP_EN                            = 0x00000001
REG_TEMP_ADC_OVER_THRSH_H                       = 0x3e
REG_TEMP_ADC_OVER_THRSH_H_THRESH_MASK           = 0x0000007f
REG_TEMP_ADC_OVER_THRSH_H_THRESH_SHIFT          = 0
REG_TEMP_ADC_OVER_THRSH_L                       = 0x3f
REG_TEMP_ADC_OVER_THRSH_L_THRESH_MASK           = 0x000000fc
REG_TEMP_ADC_OVER_THRSH_L_THRESH_SHIFT          = 2
REG_TEMP_ADC_UNDER_THRSH_H                      = 0x40
REG_TEMP_ADC_UNDER_THRSH_H_THRESH_MASK          = 0x0000007f
REG_TEMP_ADC_UNDER_THRSH_H_THRESH_SHIFT         = 0
REG_TEMP_ADC_UNDER_THRSH_L                      = 0x41
REG_TEMP_ADC_UNDER_THRSH_L_THRESH_MASK          = 0x000000fc
REG_TEMP_ADC_UNDER_THRSH_L_THRESH_SHIFT         = 2
REG_TEMP_ADC_TIMER                              = 0x42
REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_INACT_MASK    = 0x000000f0
REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_INACT_SHIFT   = 4
REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_ACT_MASK      = 0x0000000f
REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_ACT_SHIFT     = 0
REG_AXIS_MASK                                   = 0x43
REG_AXIS_MASK_TAP_AXIS_MASK                     = 0x00000030
REG_AXIS_MASK_TAP_AXIS_SHIFT                    = 4
REG_AXIS_MASK_ACT_INACT_Z                       = 0x00000004
REG_AXIS_MASK_ACT_INACT_Y                       = 0x00000002
REG_AXIS_MASK_ACT_INACT_X                       = 0x00000001
REG_STATUS_COPY                                 = 0x44
REG_STATUS_COPY_ERR_USER_REGS                   = 0x00000080
REG_STATUS_COPY_AWAKE                           = 0x00000040
REG_STATUS_COPY_INACT                           = 0x00000020
REG_STATUS_COPY_ACT                             = 0x00000010
REG_STATUS_COPY_FIFO_OVER_RUN                   = 0x00000008
REG_STATUS_COPY_FIFO_WATER_MARK                 = 0x00000004
REG_STATUS_COPY_FIFO_READY                      = 0x00000002
REG_STATUS_COPY_DATA_READY                      = 0x00000001
REG_STATUS_2                                    = 0x45
REG_STATUS_2_ERR_FUSE_REGS                      = 0x00000080
REG_STATUS_2_FUSE_REFRESH                       = 0x00000040
REG_STATUS_2_TIMER                              = 0x00000010
REG_STATUS_2_TEMP_ADC_HI                        = 0x00000008
REG_STATUS_2_TEMP_ADC_LOW                       = 0x00000004
REG_STATUS_2_TAP_TWO                            = 0x00000002
REG_STATUS_2_TAP_ONE                            = 0x00000001
REG_STATUS_3                                    = 0x46
REG_STATUS_3_PEDOMETER_OVERFLOW                 = 0x00000001
REG_PEDOMETER_STEP_CNT_H                        = 0x47
REG_PEDOMETER_STEP_CNT_L                        = 0x48
REG_PEDOMETER_CTL                               = 0x49
REG_PEDOMETER_CTL_RESET_STEP                    = 0x00000004
REG_PEDOMETER_CTL_RESET_OF                      = 0x00000002
REG_PEDOMETER_CTL_EN                            = 0x00000001
REG_PEDOMETER_THRES_H                           = 0x4a
REG_PEDOMETER_THRES_H_THRESHOLD_MASK            = 0x0000007f
REG_PEDOMETER_THRES_H_THRESHOLD_SHIFT           = 0
REG_PEDOMETER_THRES_L                           = 0x4b
REG_PEDOMETER_THRES_L_THRESHOLD_MASK            = 0x000000ff
REG_PEDOMETER_THRES_L_THRESHOLD_SHIFT           = 0
REG_PEDOMETER_SENS_H                            = 0x4c
REG_PEDOMETER_SENS_H_SENSITIVITY_MASK           = 0x0000007f
REG_PEDOMETER_SENS_H_SENSITIVITY_SHIFT          = 0
REG_PEDOMETER_SENS_L                            = 0x4d
REG_PEDOMETER_SENS_L_SENSITIVITY_MASK           = 0x000000ff
REG_PEDOMETER_SENS_L_SENSITIVITY_SHIFT          = 0

def adi_adxl36x_reg_name(addr):
    if addr == 0x00:
        return "devid_ad"
    elif addr == 0x01:
        return "devid_mst"
    elif addr == 0x02:
        return "part_id"
    elif addr == 0x03:
        return "rev_id"
    elif addr == 0x05:
        return "serial_number_2"
    elif addr == 0x06:
        return "serial_number_1"
    elif addr == 0x07:
        return "serial_number_0"
    elif addr == 0x08:
        return "xdata"
    elif addr == 0x09:
        return "ydata"
    elif addr == 0x0a:
        return "zdata"
    elif addr == 0x0b:
        return "status"
    elif addr == 0x0c:
        return "fifo_entries_l"
    elif addr == 0x0d:
        return "fifo_entries_h"
    elif addr == 0x0e:
        return "xdata_h"
    elif addr == 0x0f:
        return "xdata_l"
    elif addr == 0x10:
        return "ydata_h"
    elif addr == 0x11:
        return "ydata_l"
    elif addr == 0x12:
        return "zdata_h"
    elif addr == 0x13:
        return "zdata_l"
    elif addr == 0x14:
        return "temp_h"
    elif addr == 0x15:
        return "temp_l"
    elif addr == 0x16:
        return "ex_adc_h"
    elif addr == 0x17:
        return "ex_adc_l"
    elif addr == 0x18:
        return "i2c_fifo_data"
    elif addr == 0x1f:
        return "soft_reset"
    elif addr == 0x20:
        return "thresh_act_h"
    elif addr == 0x21:
        return "thresh_act_l"
    elif addr == 0x22:
        return "time_act"
    elif addr == 0x23:
        return "thresh_inact_h"
    elif addr == 0x24:
        return "thresh_inact_l"
    elif addr == 0x25:
        return "time_inact_h"
    elif addr == 0x26:
        return "time_inact_l"
    elif addr == 0x27:
        return "act_inact_ctl"
    elif addr == 0x28:
        return "fifo_control"
    elif addr == 0x29:
        return "fifo_samples"
    elif addr == 0x2a:
        return "intmap1_lower"
    elif addr == 0x2b:
        return "intmap2_lower"
    elif addr == 0x2c:
        return "filter_ctl"
    elif addr == 0x2d:
        return "power_ctl"
    elif addr == 0x2e:
        return "self_test"
    elif addr == 0x2f:
        return "tap_thresh"
    elif addr == 0x30:
        return "tap_dur"
    elif addr == 0x31:
        return "tap_latent"
    elif addr == 0x32:
        return "tap_window"
    elif addr == 0x33:
        return "x_offset"
    elif addr == 0x34:
        return "y_offset"
    elif addr == 0x35:
        return "z_offset"
    elif addr == 0x36:
        return "x_sens"
    elif addr == 0x37:
        return "y_sens"
    elif addr == 0x38:
        return "z_sens"
    elif addr == 0x39:
        return "timer_ctl"
    elif addr == 0x3a:
        return "intmap1_upper"
    elif addr == 0x3b:
        return "intmaps2_upper"
    elif addr == 0x3c:
        return "adc_ctl"
    elif addr == 0x3d:
        return "temp_ctl"
    elif addr == 0x3e:
        return "temp_adc_over_thrsh_h"
    elif addr == 0x3f:
        return "temp_adc_over_thrsh_l"
    elif addr == 0x40:
        return "temp_adc_under_thrsh_h"
    elif addr == 0x41:
        return "temp_adc_under_thrsh_l"
    elif addr == 0x42:
        return "temp_adc_timer"
    elif addr == 0x43:
        return "axis_mask"
    elif addr == 0x44:
        return "status_copy"
    elif addr == 0x45:
        return "status_2"
    elif addr == 0x46:
        return "status_3"
    elif addr == 0x47:
        return "pedometer_step_cnt_h"
    elif addr == 0x48:
        return "pedometer_step_cnt_l"
    elif addr == 0x49:
        return "pedometer_ctl"
    elif addr == 0x4a:
        return "pedometer_thres_h"
    elif addr == 0x4b:
        return "pedometer_thres_l"
    elif addr == 0x4c:
        return "pedometer_sens_h"
    elif addr == 0x4d:
        return "pedometer_sens_l"
    return None

def adi_adxl36x_print_fields(addr, value):
    if addr == 0x00:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x01:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x02:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x03:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x05:
        print("bit16:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x06:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x07:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x08:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x09:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x0a:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x0b:
        print("err_user_regs:", (value >> 7) & 1, end="", sep="")
        print(",awake:", (value >> 6) & 1, end="", sep="")
        print(",inact:", (value >> 5) & 1, end="", sep="")
        print(",act:", (value >> 4) & 1, end="", sep="")
        print(",fifo_over_run:", (value >> 3) & 1, end="", sep="")
        print(",fifo_water_mark:", (value >> 2) & 1, end="", sep="")
        print(",fifo_ready:", (value >> 1) & 1, end="", sep="")
        print(",data_ready:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x0c:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x0d:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x0e:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x0f:
        print("data:", (value >> 2) & 63, end="", sep="")
        return
    elif addr == 0x10:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x11:
        print("data:", (value >> 2) & 63, end="", sep="")
        return
    elif addr == 0x12:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x13:
        print("data:", (value >> 2) & 63, end="", sep="")
        return
    elif addr == 0x14:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x15:
        print("data:", (value >> 2) & 63, end="", sep="")
        return
    elif addr == 0x16:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x17:
        print("data:", (value >> 2) & 63, end="", sep="")
        return
    elif addr == 0x18:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x1f:
        print("soft_reset:", (value >> 1) & 1, end="", sep="")
        return
    elif addr == 0x20:
        print("data:", (value >> 0) & 127, end="", sep="")
        return
    elif addr == 0x21:
        print("data:", (value >> 2) & 63, end="", sep="")
        return
    elif addr == 0x22:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x23:
        print("data:", (value >> 0) & 127, end="", sep="")
        return
    elif addr == 0x24:
        print("data:", (value >> 2) & 63, end="", sep="")
        return
    elif addr == 0x25:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x26:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x27:
        print("reg_readback:", (value >> 6) & 3, end="", sep="")
        print(",linkloop:", end="", sep="")
        localvalue = (value >> 4) & 3
        if localvalue == 0:
            print("default1", end="", sep="")
        elif localvalue == 1:
            print("linked", end="", sep="")
        elif localvalue == 2:
            print("default2", end="", sep="")
        elif localvalue == 3:
            print("looped", end="", sep="")

        print(",intact_en:", (value >> 2) & 3, end="", sep="")
        print(",act_en:", (value >> 0) & 3, end="", sep="")
        return
    elif addr == 0x28:
        print("channel_select:", end="", sep="")
        localvalue = (value >> 3) & 15
        if localvalue == 0:
            print("xyz", end="", sep="")
        elif localvalue == 1:
            print("x", end="", sep="")
        elif localvalue == 2:
            print("y", end="", sep="")
        elif localvalue == 3:
            print("z", end="", sep="")
        elif localvalue == 4:
            print("xyzt", end="", sep="")
        elif localvalue == 5:
            print("xt", end="", sep="")
        elif localvalue == 6:
            print("yt", end="", sep="")
        elif localvalue == 7:
            print("zt", end="", sep="")
        elif localvalue == 8:
            print("xyza", end="", sep="")
        elif localvalue == 9:
            print("xa", end="", sep="")
        elif localvalue == 10:
            print("ya", end="", sep="")
        elif localvalue == 11:
            print("za", end="", sep="")
        else:
            print((value >> 3) & 15, end="", sep="")

        print(",fifo_samples:", (value >> 2) & 1, end="", sep="")
        print(",fifo_mode:", end="", sep="")
        localvalue = (value >> 0) & 3
        if localvalue == 0:
            print("disabled", end="", sep="")
        elif localvalue == 1:
            print("oldest", end="", sep="")
        elif localvalue == 2:
            print("stream", end="", sep="")
        elif localvalue == 3:
            print("trigger", end="", sep="")

        return
    elif addr == 0x29:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x2a:
        print("int_low_int:", (value >> 7) & 1, end="", sep="")
        print(",awake_int:", (value >> 6) & 1, end="", sep="")
        print(",inact_int:", (value >> 5) & 1, end="", sep="")
        print(",act_int:", (value >> 4) & 1, end="", sep="")
        print(",fifo_overrun_int:", (value >> 3) & 1, end="", sep="")
        print(",fifo_water_mark_int:", (value >> 2) & 1, end="", sep="")
        print(",fifo_rdy_int:", (value >> 1) & 1, end="", sep="")
        print(",data_rdy_int:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x2b:
        print("int_low_int:", (value >> 7) & 1, end="", sep="")
        print(",awake_int:", (value >> 6) & 1, end="", sep="")
        print(",inact_int:", (value >> 5) & 1, end="", sep="")
        print(",act_int:", (value >> 4) & 1, end="", sep="")
        print(",fifo_overrun_int:", (value >> 3) & 1, end="", sep="")
        print(",fifo_water_mark_int:", (value >> 2) & 1, end="", sep="")
        print(",fifo_rdy_int:", (value >> 1) & 1, end="", sep="")
        print(",data_rdy_int:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x2c:
        print("range:", (value >> 6) & 3, end="", sep="")
        print(",i2c_hs:", (value >> 5) & 1, end="", sep="")
        print(",ext_sample:", (value >> 3) & 1, end="", sep="")
        print(",odr:", (value >> 0) & 7, end="", sep="")
        return
    elif addr == 0x2d:
        print("ext_clk:", (value >> 6) & 1, end="", sep="")
        print(",noise:", (value >> 4) & 3, end="", sep="")
        print(",wakeup:", (value >> 3) & 1, end="", sep="")
        print(",autosleep:", (value >> 2) & 1, end="", sep="")
        print(",measure:", (value >> 0) & 3, end="", sep="")
        return
    elif addr == 0x2e:
        print("st_force:", (value >> 1) & 1, end="", sep="")
        print(",st:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x2f:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x30:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x31:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x32:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x33:
        print("user_offset:", (value >> 0) & 31, end="", sep="")
        return
    elif addr == 0x34:
        print("user_offset:", (value >> 0) & 31, end="", sep="")
        return
    elif addr == 0x35:
        print("user_offset:", (value >> 0) & 31, end="", sep="")
        return
    elif addr == 0x36:
        print("user_sens:", (value >> 0) & 63, end="", sep="")
        return
    elif addr == 0x37:
        print("user_sens:", (value >> 0) & 63, end="", sep="")
        return
    elif addr == 0x38:
        print("user_sens:", (value >> 0) & 63, end="", sep="")
        return
    elif addr == 0x39:
        print("wakeup_rate:", (value >> 6) & 3, end="", sep="")
        print(",timer_keep_alive:", (value >> 0) & 31, end="", sep="")
        return
    elif addr == 0x3a:
        print("err_fuse_int:", (value >> 7) & 1, end="", sep="")
        print(",err_user_regs_int:", (value >> 6) & 1, end="", sep="")
        print(",kplav_timer_int:", (value >> 4) & 1, end="", sep="")
        print(",temp_adc_hi_int:", (value >> 3) & 1, end="", sep="")
        print(",temp_adc_low_int:", (value >> 2) & 1, end="", sep="")
        print(",tap_two_int:", (value >> 1) & 1, end="", sep="")
        print(",tap_one_int:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x3b:
        print("err_fuse_int:", (value >> 7) & 1, end="", sep="")
        print(",err_user_regs_int:", (value >> 6) & 1, end="", sep="")
        print(",kplav_timer_int:", (value >> 4) & 1, end="", sep="")
        print(",temp_adc_hi_int:", (value >> 3) & 1, end="", sep="")
        print(",temp_adc_low_int:", (value >> 2) & 1, end="", sep="")
        print(",tap_two_int:", (value >> 1) & 1, end="", sep="")
        print(",tap_one_int:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x3c:
        print("fifo_8_12BIT:", (value >> 6) & 3, end="", sep="")
        print(",adc_inact_en:", (value >> 3) & 1, end="", sep="")
        print(",adc_act_en:", (value >> 1) & 1, end="", sep="")
        print(",adc_en:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x3d:
        print("nl_comp_en:", (value >> 7) & 1, end="", sep="")
        print(",temp_inact_en:", (value >> 3) & 1, end="", sep="")
        print(",temp_act_en:", (value >> 1) & 1, end="", sep="")
        print(",temp_en:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x3e:
        print("thresh:", (value >> 0) & 127, end="", sep="")
        return
    elif addr == 0x3f:
        print("thresh:", (value >> 2) & 63, end="", sep="")
        return
    elif addr == 0x40:
        print("thresh:", (value >> 0) & 127, end="", sep="")
        return
    elif addr == 0x41:
        print("thresh:", (value >> 2) & 63, end="", sep="")
        return
    elif addr == 0x42:
        print("timer_temp_adc_inact:", (value >> 4) & 15, end="", sep="")
        print(",timer_temp_adc_act:", (value >> 0) & 15, end="", sep="")
        return
    elif addr == 0x43:
        print("tap_axis:", (value >> 4) & 3, end="", sep="")
        print(",act_inact_z:", (value >> 2) & 1, end="", sep="")
        print(",act_inact_y:", (value >> 1) & 1, end="", sep="")
        print(",act_inact_x:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x44:
        print("err_user_regs:", (value >> 7) & 1, end="", sep="")
        print(",awake:", (value >> 6) & 1, end="", sep="")
        print(",inact:", (value >> 5) & 1, end="", sep="")
        print(",act:", (value >> 4) & 1, end="", sep="")
        print(",fifo_over_run:", (value >> 3) & 1, end="", sep="")
        print(",fifo_water_mark:", (value >> 2) & 1, end="", sep="")
        print(",fifo_ready:", (value >> 1) & 1, end="", sep="")
        print(",data_ready:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x45:
        print("err_fuse_regs:", (value >> 7) & 1, end="", sep="")
        print(",fuse_refresh:", (value >> 6) & 1, end="", sep="")
        print(",timer:", (value >> 4) & 1, end="", sep="")
        print(",temp_adc_hi:", (value >> 3) & 1, end="", sep="")
        print(",temp_adc_low:", (value >> 2) & 1, end="", sep="")
        print(",tap_two:", (value >> 1) & 1, end="", sep="")
        print(",tap_one:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x46:
        print("pedometer_overflow:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x47:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x48:
        print(hex(value), end="", sep="")
        return
    elif addr == 0x49:
        print("reset_step:", (value >> 2) & 1, end="", sep="")
        print(",reset_of:", (value >> 1) & 1, end="", sep="")
        print(",en:", (value >> 0) & 1, end="", sep="")
        return
    elif addr == 0x4a:
        print("threshold:", (value >> 0) & 127, end="", sep="")
        return
    elif addr == 0x4b:
        print("threshold:", (value >> 0) & 255, end="", sep="")
        return
    elif addr == 0x4c:
        print("sensitivity:", (value >> 0) & 127, end="", sep="")
        return
    elif addr == 0x4d:
        print("sensitivity:", (value >> 0) & 255, end="", sep="")
        return