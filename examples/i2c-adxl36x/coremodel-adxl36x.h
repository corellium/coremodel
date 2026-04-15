#ifndef _ADI_ADXL36X_H
#define _ADI_ADXL36X_H

char *adi_adxl36x_reg_name(unsigned addr);
void adi_adxl36x_print_fields(unsigned addr, unsigned long value, int (*uprintf)(const char *, ...));

#define REG_DEVID_AD                            0x00
#define REG_DEVID_MST                           0x01
#define REG_PART_ID                             0x02
#define REG_REV_ID                              0x03
#define REG_SERIAL_NUMBER_2                     0x05
#define  REG_SERIAL_NUMBER_2_BIT16              0x00000001ul
#define   REG_SERIAL_NUMBER_2_BIT16_GET(r)      (((r) >> 0) & 0x1ul)
#define   REG_SERIAL_NUMBER_2_BIT16_SET(r,v)    do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_SERIAL_NUMBER_1                     0x06
#define REG_SERIAL_NUMBER_0                     0x07
#define REG_XDATA                               0x08
#define REG_YDATA                               0x09
#define REG_ZDATA                               0x0a
#define REG_STATUS                              0x0b
#define  REG_STATUS_ERR_USER_REGS               0x00000080ul
#define   REG_STATUS_ERR_USER_REGS_GET(r)       (((r) >> 7) & 0x1ul)
#define   REG_STATUS_ERR_USER_REGS_SET(r,v)     do { (r) = ((r) & ~0x80ul) | (((v) & (0x1ul)) << 7); } while(0)
#define  REG_STATUS_AWAKE                       0x00000040ul
#define   REG_STATUS_AWAKE_GET(r)               (((r) >> 6) & 0x1ul)
#define   REG_STATUS_AWAKE_SET(r,v)             do { (r) = ((r) & ~0x40ul) | (((v) & (0x1ul)) << 6); } while(0)
#define  REG_STATUS_INACT                       0x00000020ul
#define   REG_STATUS_INACT_GET(r)               (((r) >> 5) & 0x1ul)
#define   REG_STATUS_INACT_SET(r,v)             do { (r) = ((r) & ~0x20ul) | (((v) & (0x1ul)) << 5); } while(0)
#define  REG_STATUS_ACT                         0x00000010ul
#define   REG_STATUS_ACT_GET(r)                 (((r) >> 4) & 0x1ul)
#define   REG_STATUS_ACT_SET(r,v)               do { (r) = ((r) & ~0x10ul) | (((v) & (0x1ul)) << 4); } while(0)
#define  REG_STATUS_FIFO_OVER_RUN               0x00000008ul
#define   REG_STATUS_FIFO_OVER_RUN_GET(r)       (((r) >> 3) & 0x1ul)
#define   REG_STATUS_FIFO_OVER_RUN_SET(r,v)     do { (r) = ((r) & ~0x8ul) | (((v) & (0x1ul)) << 3); } while(0)
#define  REG_STATUS_FIFO_WATER_MARK             0x00000004ul
#define   REG_STATUS_FIFO_WATER_MARK_GET(r)     (((r) >> 2) & 0x1ul)
#define   REG_STATUS_FIFO_WATER_MARK_SET(r,v)   do { (r) = ((r) & ~0x4ul) | (((v) & (0x1ul)) << 2); } while(0)
#define  REG_STATUS_FIFO_READY                  0x00000002ul
#define   REG_STATUS_FIFO_READY_GET(r)          (((r) >> 1) & 0x1ul)
#define   REG_STATUS_FIFO_READY_SET(r,v)        do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define  REG_STATUS_DATA_READY                  0x00000001ul
#define   REG_STATUS_DATA_READY_GET(r)          (((r) >> 0) & 0x1ul)
#define   REG_STATUS_DATA_READY_SET(r,v)        do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_FIFO_ENTRIES_L                      0x0c
#define REG_FIFO_ENTRIES_H                      0x0d
#define REG_XDATA_H                             0x0e
#define REG_XDATA_L                             0x0f
#define  REG_XDATA_L_DATA_MASK                  0x000000fcul
#define   REG_XDATA_L_DATA_SHIFT                2
#define   REG_XDATA_L_DATA_GET(r)               (((r) >> 2) & 0x3ful)
#define   REG_XDATA_L_DATA_SET(r,v)             do { (r) = ((r) & ~0xfcul) | (((v) & (0x3ful)) << 2); } while(0)
#define REG_YDATA_H                             0x10
#define REG_YDATA_L                             0x11
#define  REG_YDATA_L_DATA_MASK                  0x000000fcul
#define   REG_YDATA_L_DATA_SHIFT                2
#define   REG_YDATA_L_DATA_GET(r)               (((r) >> 2) & 0x3ful)
#define   REG_YDATA_L_DATA_SET(r,v)             do { (r) = ((r) & ~0xfcul) | (((v) & (0x3ful)) << 2); } while(0)
#define REG_ZDATA_H                             0x12
#define REG_ZDATA_L                             0x13
#define  REG_ZDATA_L_DATA_MASK                  0x000000fcul
#define   REG_ZDATA_L_DATA_SHIFT                2
#define   REG_ZDATA_L_DATA_GET(r)               (((r) >> 2) & 0x3ful)
#define   REG_ZDATA_L_DATA_SET(r,v)             do { (r) = ((r) & ~0xfcul) | (((v) & (0x3ful)) << 2); } while(0)
#define REG_TEMP_H                              0x14
#define REG_TEMP_L                              0x15
#define  REG_TEMP_L_DATA_MASK                   0x000000fcul
#define   REG_TEMP_L_DATA_SHIFT                 2
#define   REG_TEMP_L_DATA_GET(r)                (((r) >> 2) & 0x3ful)
#define   REG_TEMP_L_DATA_SET(r,v)              do { (r) = ((r) & ~0xfcul) | (((v) & (0x3ful)) << 2); } while(0)
#define REG_EX_ADC_H                            0x16
#define REG_EX_ADC_L                            0x17
#define  REG_EX_ADC_L_DATA_MASK                 0x000000fcul
#define   REG_EX_ADC_L_DATA_SHIFT               2
#define   REG_EX_ADC_L_DATA_GET(r)              (((r) >> 2) & 0x3ful)
#define   REG_EX_ADC_L_DATA_SET(r,v)            do { (r) = ((r) & ~0xfcul) | (((v) & (0x3ful)) << 2); } while(0)
#define REG_I2C_FIFO_DATA                       0x18
#define REG_SOFT_RESET                          0x1f
#define  REG_SOFT_RESET_SOFT_RESET              0x00000002ul
#define   REG_SOFT_RESET_SOFT_RESET_GET(r)      (((r) >> 1) & 0x1ul)
#define   REG_SOFT_RESET_SOFT_RESET_SET(r,v)    do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define REG_THRESH_ACT_H                        0x20
#define  REG_THRESH_ACT_H_DATA_MASK             0x0000007ful
#define   REG_THRESH_ACT_H_DATA_SHIFT           0
#define   REG_THRESH_ACT_H_DATA_GET(r)          (((r) >> 0) & 0x7ful)
#define   REG_THRESH_ACT_H_DATA_SET(r,v)        do { (r) = ((r) & ~0x7ful) | (((v) & (0x7ful)) << 0); } while(0)
#define REG_THRESH_ACT_L                        0x21
#define  REG_THRESH_ACT_L_DATA_MASK             0x000000fcul
#define   REG_THRESH_ACT_L_DATA_SHIFT           2
#define   REG_THRESH_ACT_L_DATA_GET(r)          (((r) >> 2) & 0x3ful)
#define   REG_THRESH_ACT_L_DATA_SET(r,v)        do { (r) = ((r) & ~0xfcul) | (((v) & (0x3ful)) << 2); } while(0)
#define REG_TIME_ACT                            0x22
#define REG_THRESH_INACT_H                      0x23
#define  REG_THRESH_INACT_H_DATA_MASK           0x0000007ful
#define   REG_THRESH_INACT_H_DATA_SHIFT         0
#define   REG_THRESH_INACT_H_DATA_GET(r)        (((r) >> 0) & 0x7ful)
#define   REG_THRESH_INACT_H_DATA_SET(r,v)      do { (r) = ((r) & ~0x7ful) | (((v) & (0x7ful)) << 0); } while(0)
#define REG_THRESH_INACT_L                      0x24
#define  REG_THRESH_INACT_L_DATA_MASK           0x000000fcul
#define   REG_THRESH_INACT_L_DATA_SHIFT         2
#define   REG_THRESH_INACT_L_DATA_GET(r)        (((r) >> 2) & 0x3ful)
#define   REG_THRESH_INACT_L_DATA_SET(r,v)      do { (r) = ((r) & ~0xfcul) | (((v) & (0x3ful)) << 2); } while(0)
#define REG_TIME_INACT_H                        0x25
#define REG_TIME_INACT_L                        0x26
#define REG_ACT_INACT_CTL                       0x27
#define  REG_ACT_INACT_CTL_REG_READBACK_MASK    0x000000c0ul
#define   REG_ACT_INACT_CTL_REG_READBACK_SHIFT  6
#define   REG_ACT_INACT_CTL_REG_READBACK_GET(r) (((r) >> 6) & 0x3ul)
#define   REG_ACT_INACT_CTL_REG_READBACK_SET(r,v) do { (r) = ((r) & ~0xc0ul) | (((v) & (0x3ul)) << 6); } while(0)
#define  REG_ACT_INACT_CTL_LINKLOOP_MASK        0x00000030ul
#define   REG_ACT_INACT_CTL_LINKLOOP_SHIFT      4
#define   REG_ACT_INACT_CTL_LINKLOOP_DEFAULT1   0x00000000ul
#define   REG_ACT_INACT_CTL_LINKLOOP_LINKED     0x00000010ul
#define   REG_ACT_INACT_CTL_LINKLOOP_DEFAULT2   0x00000020ul
#define   REG_ACT_INACT_CTL_LINKLOOP_LOOPED     0x00000030ul
#define   REG_ACT_INACT_CTL_LINKLOOP_GET(r)     (((r) >> 4) & 0x3ul)
#define   REG_ACT_INACT_CTL_LINKLOOP_SET(r,v)   do { (r) = ((r) & ~0x30ul) | (((v) & (0x3ul)) << 4); } while(0)
#define  REG_ACT_INACT_CTL_INTACT_EN_MASK       0x0000000cul
#define   REG_ACT_INACT_CTL_INTACT_EN_SHIFT     2
#define   REG_ACT_INACT_CTL_INTACT_EN_GET(r)    (((r) >> 2) & 0x3ul)
#define   REG_ACT_INACT_CTL_INTACT_EN_SET(r,v)  do { (r) = ((r) & ~0xcul) | (((v) & (0x3ul)) << 2); } while(0)
#define  REG_ACT_INACT_CTL_ACT_EN_MASK          0x00000003ul
#define   REG_ACT_INACT_CTL_ACT_EN_SHIFT        0
#define   REG_ACT_INACT_CTL_ACT_EN_GET(r)       (((r) >> 0) & 0x3ul)
#define   REG_ACT_INACT_CTL_ACT_EN_SET(r,v)     do { (r) = ((r) & ~0x3ul) | (((v) & (0x3ul)) << 0); } while(0)
#define REG_FIFO_CONTROL                        0x28
#define  REG_FIFO_CONTROL_CHANNEL_SELECT_MASK   0x00000078ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_SHIFT 3
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_XYZ   0x00000000ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_X     0x00000008ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_Y     0x00000010ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_Z     0x00000018ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_XYZT  0x00000020ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_XT    0x00000028ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_YT    0x00000030ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_ZT    0x00000038ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_XYZA  0x00000040ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_XA    0x00000048ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_YA    0x00000050ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_ZA    0x00000058ul
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_GET(r) (((r) >> 3) & 0xful)
#define   REG_FIFO_CONTROL_CHANNEL_SELECT_SET(r,v)  do { (r) = ((r) & ~0x78ul) | (((v) & (0xful)) << 3); } while(0)
#define  REG_FIFO_CONTROL_FIFO_SAMPLES          0x00000004ul
#define   REG_FIFO_CONTROL_FIFO_SAMPLES_GET(r)  (((r) >> 2) & 0x1ul)
#define   REG_FIFO_CONTROL_FIFO_SAMPLES_SET(r,v) do { (r) = ((r) & ~0x4ul) | (((v) & (0x1ul)) << 2); } while(0)
#define  REG_FIFO_CONTROL_FIFO_MODE_MASK        0x00000003ul
#define   REG_FIFO_CONTROL_FIFO_MODE_DISABLED   0x00000000ul
#define   REG_FIFO_CONTROL_FIFO_MODE_OLDEST     0x00000001ul
#define   REG_FIFO_CONTROL_FIFO_MODE_STREAM     0x00000002ul
#define   REG_FIFO_CONTROL_FIFO_MODE_TRIGGER    0x00000003ul
#define   REG_FIFO_CONTROL_FIFO_MODE_GET(r)     (((r) >> 0) & 0x3ul)
#define   REG_FIFO_CONTROL_FIFO_MODE_SET(r,v)   do { (r) = ((r) & ~0x3ul) | (((v) & (0x3ul)) << 0); } while(0)
#define REG_FIFO_SAMPLES                        0x29
#define REG_INTMAP1_LOWER                       0x2a
#define  REG_INTMAP1_LOWER_INT_LOW_INT          0x00000080ul
#define   REG_INTMAP1_LOWER_INT_LOW_INT_GET(r)  (((r) >> 7) & 0x1ul)
#define   REG_INTMAP1_LOWER_INT_LOW_INT_SET(r,v) do { (r) = ((r) & ~0x80ul) | (((v) & (0x1ul)) << 7); } while(0)
#define  REG_INTMAP1_LOWER_AWAKE_INT            0x00000040ul
#define   REG_INTMAP1_LOWER_AWAKE_INT_GET(r)    (((r) >> 6) & 0x1ul)
#define   REG_INTMAP1_LOWER_AWAKE_INT_SET(r,v)  do { (r) = ((r) & ~0x40ul) | (((v) & (0x1ul)) << 6); } while(0)
#define  REG_INTMAP1_LOWER_INACT_INT            0x00000020ul
#define   REG_INTMAP1_LOWER_INACT_INT_GET(r)    (((r) >> 5) & 0x1ul)
#define   REG_INTMAP1_LOWER_INACT_INT_SET(r,v)  do { (r) = ((r) & ~0x20ul) | (((v) & (0x1ul)) << 5); } while(0)
#define  REG_INTMAP1_LOWER_ACT_INT              0x00000010ul
#define   REG_INTMAP1_LOWER_ACT_INT_GET(r)      (((r) >> 4) & 0x1ul)
#define   REG_INTMAP1_LOWER_ACT_INT_SET(r,v)    do { (r) = ((r) & ~0x10ul) | (((v) & (0x1ul)) << 4); } while(0)
#define  REG_INTMAP1_LOWER_FIFO_OVERRUN_INT     0x00000008ul
#define   REG_INTMAP1_LOWER_FIFO_OVERRUN_INT_GET(r)   (((r) >> 3) & 0x1ul)
#define   REG_INTMAP1_LOWER_FIFO_OVERRUN_INT_SET(r,v)     do { (r) = ((r) & ~0x8ul) | (((v) & (0x1ul)) << 3); } while(0)
#define  REG_INTMAP1_LOWER_FIFO_WATER_MARK_INT  0x00000004ul
#define   REG_INTMAP1_LOWER_FIFO_WATER_MARK_INT_GET(r)      (((r) >> 2) & 0x1ul)
#define   REG_INTMAP1_LOWER_FIFO_WATER_MARK_INT_SET(r,v)        do { (r) = ((r) & ~0x4ul) | (((v) & (0x1ul)) << 2); } while(0)
#define  REG_INTMAP1_LOWER_FIFO_RDY_INT         0x00000002ul
#define   REG_INTMAP1_LOWER_FIFO_RDY_INT_GET(r) (((r) >> 1) & 0x1ul)
#define   REG_INTMAP1_LOWER_FIFO_RDY_INT_SET(r,v) do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define  REG_INTMAP1_LOWER_DATA_RDY_INT         0x00000001ul
#define   REG_INTMAP1_LOWER_DATA_RDY_INT_GET(r) (((r) >> 0) & 0x1ul)
#define   REG_INTMAP1_LOWER_DATA_RDY_INT_SET(r,v) do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_INTMAP2_LOWER                       0x2b
#define  REG_INTMAP2_LOWER_INT_LOW_INT          0x00000080ul
#define   REG_INTMAP2_LOWER_INT_LOW_INT_GET(r)  (((r) >> 7) & 0x1ul)
#define   REG_INTMAP2_LOWER_INT_LOW_INT_SET(r,v) do { (r) = ((r) & ~0x80ul) | (((v) & (0x1ul)) << 7); } while(0)
#define  REG_INTMAP2_LOWER_AWAKE_INT            0x00000040ul
#define   REG_INTMAP2_LOWER_AWAKE_INT_GET(r)    (((r) >> 6) & 0x1ul)
#define   REG_INTMAP2_LOWER_AWAKE_INT_SET(r,v)  do { (r) = ((r) & ~0x40ul) | (((v) & (0x1ul)) << 6); } while(0)
#define  REG_INTMAP2_LOWER_INACT_INT            0x00000020ul
#define   REG_INTMAP2_LOWER_INACT_INT_GET(r)    (((r) >> 5) & 0x1ul)
#define   REG_INTMAP2_LOWER_INACT_INT_SET(r,v)  do { (r) = ((r) & ~0x20ul) | (((v) & (0x1ul)) << 5); } while(0)
#define  REG_INTMAP2_LOWER_ACT_INT              0x00000010ul
#define   REG_INTMAP2_LOWER_ACT_INT_GET(r)      (((r) >> 4) & 0x1ul)
#define   REG_INTMAP2_LOWER_ACT_INT_SET(r,v)    do { (r) = ((r) & ~0x10ul) | (((v) & (0x1ul)) << 4); } while(0)
#define  REG_INTMAP2_LOWER_FIFO_OVERRUN_INT     0x00000008ul
#define   REG_INTMAP2_LOWER_FIFO_OVERRUN_INT_GET(r)   (((r) >> 3) & 0x1ul)
#define   REG_INTMAP2_LOWER_FIFO_OVERRUN_INT_SET(r,v)     do { (r) = ((r) & ~0x8ul) | (((v) & (0x1ul)) << 3); } while(0)
#define  REG_INTMAP2_LOWER_FIFO_WATER_MARK_INT  0x00000004ul
#define   REG_INTMAP2_LOWER_FIFO_WATER_MARK_INT_GET(r)      (((r) >> 2) & 0x1ul)
#define   REG_INTMAP2_LOWER_FIFO_WATER_MARK_INT_SET(r,v)        do { (r) = ((r) & ~0x4ul) | (((v) & (0x1ul)) << 2); } while(0)
#define  REG_INTMAP2_LOWER_FIFO_RDY_INT         0x00000002ul
#define   REG_INTMAP2_LOWER_FIFO_RDY_INT_GET(r) (((r) >> 1) & 0x1ul)
#define   REG_INTMAP2_LOWER_FIFO_RDY_INT_SET(r,v) do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define  REG_INTMAP2_LOWER_DATA_RDY_INT         0x00000001ul
#define   REG_INTMAP2_LOWER_DATA_RDY_INT_GET(r) (((r) >> 0) & 0x1ul)
#define   REG_INTMAP2_LOWER_DATA_RDY_INT_SET(r,v) do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_FILTER_CTL                          0x2c
#define  REG_FILTER_CTL_RANGE_MASK              0x000000c0ul
#define   REG_FILTER_CTL_RANGE_SHIFT            6
#define   REG_FILTER_CTL_RANGE_GET(r)           (((r) >> 6) & 0x3ul)
#define   REG_FILTER_CTL_RANGE_SET(r,v)         do { (r) = ((r) & ~0xc0ul) | (((v) & (0x3ul)) << 6); } while(0)
#define  REG_FILTER_CTL_I2C_HS                  0x00000020ul
#define   REG_FILTER_CTL_I2C_HS_GET(r)          (((r) >> 5) & 0x1ul)
#define   REG_FILTER_CTL_I2C_HS_SET(r,v)        do { (r) = ((r) & ~0x20ul) | (((v) & (0x1ul)) << 5); } while(0)
#define  REG_FILTER_CTL_EXT_SAMPLE              0x00000008ul
#define   REG_FILTER_CTL_EXT_SAMPLE_GET(r)      (((r) >> 3) & 0x1ul)
#define   REG_FILTER_CTL_EXT_SAMPLE_SET(r,v)    do { (r) = ((r) & ~0x8ul) | (((v) & (0x1ul)) << 3); } while(0)
#define  REG_FILTER_CTL_ODR_MASK                0x00000007ul
#define   REG_FILTER_CTL_ODR_SHIFT              0
#define   REG_FILTER_CTL_ODR_GET(r)             (((r) >> 0) & 0x7ul)
#define   REG_FILTER_CTL_ODR_SET(r,v)           do { (r) = ((r) & ~0x7ul) | (((v) & (0x7ul)) << 0); } while(0)
#define REG_POWER_CTL                           0x2d
#define  REG_POWER_CTL_EXT_CLK                  0x00000040ul
#define   REG_POWER_CTL_EXT_CLK_GET(r)          (((r) >> 6) & 0x1ul)
#define   REG_POWER_CTL_EXT_CLK_SET(r,v)        do { (r) = ((r) & ~0x40ul) | (((v) & (0x1ul)) << 6); } while(0)
#define  REG_POWER_CTL_NOISE_MASK               0x00000030ul
#define   REG_POWER_CTL_NOISE_SHIFT             4
#define   REG_POWER_CTL_NOISE_GET(r)            (((r) >> 4) & 0x3ul)
#define   REG_POWER_CTL_NOISE_SET(r,v)          do { (r) = ((r) & ~0x30ul) | (((v) & (0x3ul)) << 4); } while(0)
#define  REG_POWER_CTL_WAKEUP                   0x00000008ul
#define   REG_POWER_CTL_WAKEUP_GET(r)           (((r) >> 3) & 0x1ul)
#define   REG_POWER_CTL_WAKEUP_SET(r,v)         do { (r) = ((r) & ~0x8ul) | (((v) & (0x1ul)) << 3); } while(0)
#define  REG_POWER_CTL_AUTOSLEEP                0x00000004ul
#define   REG_POWER_CTL_AUTOSLEEP_GET(r)        (((r) >> 2) & 0x1ul)
#define   REG_POWER_CTL_AUTOSLEEP_SET(r,v)      do { (r) = ((r) & ~0x4ul) | (((v) & (0x1ul)) << 2); } while(0)
#define  REG_POWER_CTL_MEASURE_MASK             0x00000003ul
#define   REG_POWER_CTL_MEASURE_SHIFT           0
#define   REG_POWER_CTL_MEASURE_GET(r)          (((r) >> 0) & 0x3ul)
#define   REG_POWER_CTL_MEASURE_SET(r,v)        do { (r) = ((r) & ~0x3ul) | (((v) & (0x3ul)) << 0); } while(0)
#define REG_SELF_TEST                           0x2e
#define  REG_SELF_TEST_ST_FORCE                 0x00000002ul
#define   REG_SELF_TEST_ST_FORCE_GET(r)         (((r) >> 1) & 0x1ul)
#define   REG_SELF_TEST_ST_FORCE_SET(r,v)       do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define  REG_SELF_TEST_ST                       0x00000001ul
#define   REG_SELF_TEST_ST_GET(r)               (((r) >> 0) & 0x1ul)
#define   REG_SELF_TEST_ST_SET(r,v)             do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_TAP_THRESH                          0x2f
#define REG_TAP_DUR                             0x30
#define REG_TAP_LATENT                          0x31
#define REG_TAP_WINDOW                          0x32
#define REG_X_OFFSET                            0x33
#define  REG_X_OFFSET_USER_OFFSET_MASK          0x0000001ful
#define   REG_X_OFFSET_USER_OFFSET_SHIFT        0
#define   REG_X_OFFSET_USER_OFFSET_GET(r)       (((r) >> 0) & 0x1ful)
#define   REG_X_OFFSET_USER_OFFSET_SET(r,v)     do { (r) = ((r) & ~0x1ful) | (((v) & (0x1ful)) << 0); } while(0)
#define REG_Y_OFFSET                            0x34
#define  REG_Y_OFFSET_USER_OFFSET_MASK          0x0000001ful
#define   REG_Y_OFFSET_USER_OFFSET_SHIFT        0
#define   REG_Y_OFFSET_USER_OFFSET_GET(r)       (((r) >> 0) & 0x1ful)
#define   REG_Y_OFFSET_USER_OFFSET_SET(r,v)     do { (r) = ((r) & ~0x1ful) | (((v) & (0x1ful)) << 0); } while(0)
#define REG_Z_OFFSET                            0x35
#define  REG_Z_OFFSET_USER_OFFSET_MASK          0x0000001ful
#define   REG_Z_OFFSET_USER_OFFSET_SHIFT        0
#define   REG_Z_OFFSET_USER_OFFSET_GET(r)       (((r) >> 0) & 0x1ful)
#define   REG_Z_OFFSET_USER_OFFSET_SET(r,v)     do { (r) = ((r) & ~0x1ful) | (((v) & (0x1ful)) << 0); } while(0)
#define REG_X_SENS                              0x36
#define  REG_X_SENS_USER_SENS_MASK              0x0000003ful
#define   REG_X_SENS_USER_SENS_SHIFT            0
#define   REG_X_SENS_USER_SENS_GET(r)           (((r) >> 0) & 0x3ful)
#define   REG_X_SENS_USER_SENS_SET(r,v)         do { (r) = ((r) & ~0x3ful) | (((v) & (0x3ful)) << 0); } while(0)
#define REG_Y_SENS                              0x37
#define  REG_Y_SENS_USER_SENS_MASK              0x0000003ful
#define   REG_Y_SENS_USER_SENS_SHIFT            0
#define   REG_Y_SENS_USER_SENS_GET(r)           (((r) >> 0) & 0x3ful)
#define   REG_Y_SENS_USER_SENS_SET(r,v)         do { (r) = ((r) & ~0x3ful) | (((v) & (0x3ful)) << 0); } while(0)
#define REG_Z_SENS                              0x38
#define  REG_Z_SENS_USER_SENS_MASK              0x0000003ful
#define   REG_Z_SENS_USER_SENS_SHIFT            0
#define   REG_Z_SENS_USER_SENS_GET(r)           (((r) >> 0) & 0x3ful)
#define   REG_Z_SENS_USER_SENS_SET(r,v)         do { (r) = ((r) & ~0x3ful) | (((v) & (0x3ful)) << 0); } while(0)
#define REG_TIMER_CTL                           0x39
#define  REG_TIMER_CTL_WAKEUP_RATE_MASK         0x000000c0ul
#define   REG_TIMER_CTL_WAKEUP_RATE_SHIFT       6
#define   REG_TIMER_CTL_WAKEUP_RATE_GET(r)      (((r) >> 6) & 0x3ul)
#define   REG_TIMER_CTL_WAKEUP_RATE_SET(r,v)    do { (r) = ((r) & ~0xc0ul) | (((v) & (0x3ul)) << 6); } while(0)
#define  REG_TIMER_CTL_TIMER_KEEP_ALIVE_MASK    0x0000001ful
#define   REG_TIMER_CTL_TIMER_KEEP_ALIVE_SHIFT  0
#define   REG_TIMER_CTL_TIMER_KEEP_ALIVE_GET(r) (((r) >> 0) & 0x1ful)
#define   REG_TIMER_CTL_TIMER_KEEP_ALIVE_SET(r,v) do { (r) = ((r) & ~0x1ful) | (((v) & (0x1ful)) << 0); } while(0)
#define REG_INTMAP1_UPPER                       0x3a
#define  REG_INTMAP1_UPPER_ERR_FUSE_INT         0x00000080ul
#define   REG_INTMAP1_UPPER_ERR_FUSE_INT_GET(r) (((r) >> 7) & 0x1ul)
#define   REG_INTMAP1_UPPER_ERR_FUSE_INT_SET(r,v) do { (r) = ((r) & ~0x80ul) | (((v) & (0x1ul)) << 7); } while(0)
#define  REG_INTMAP1_UPPER_ERR_USER_REGS_INT    0x00000040ul
#define   REG_INTMAP1_UPPER_ERR_USER_REGS_INT_GET(r)    (((r) >> 6) & 0x1ul)
#define   REG_INTMAP1_UPPER_ERR_USER_REGS_INT_SET(r,v)      do { (r) = ((r) & ~0x40ul) | (((v) & (0x1ul)) << 6); } while(0)
#define  REG_INTMAP1_UPPER_KPLAV_TIMER_INT      0x00000010ul
#define   REG_INTMAP1_UPPER_KPLAV_TIMER_INT_GET(r)  (((r) >> 4) & 0x1ul)
#define   REG_INTMAP1_UPPER_KPLAV_TIMER_INT_SET(r,v)    do { (r) = ((r) & ~0x10ul) | (((v) & (0x1ul)) << 4); } while(0)
#define  REG_INTMAP1_UPPER_TEMP_ADC_HI_INT      0x00000008ul
#define   REG_INTMAP1_UPPER_TEMP_ADC_HI_INT_GET(r)  (((r) >> 3) & 0x1ul)
#define   REG_INTMAP1_UPPER_TEMP_ADC_HI_INT_SET(r,v)    do { (r) = ((r) & ~0x8ul) | (((v) & (0x1ul)) << 3); } while(0)
#define  REG_INTMAP1_UPPER_TEMP_ADC_LOW_INT     0x00000004ul
#define   REG_INTMAP1_UPPER_TEMP_ADC_LOW_INT_GET(r)   (((r) >> 2) & 0x1ul)
#define   REG_INTMAP1_UPPER_TEMP_ADC_LOW_INT_SET(r,v)     do { (r) = ((r) & ~0x4ul) | (((v) & (0x1ul)) << 2); } while(0)
#define  REG_INTMAP1_UPPER_TAP_TWO_INT          0x00000002ul
#define   REG_INTMAP1_UPPER_TAP_TWO_INT_GET(r)  (((r) >> 1) & 0x1ul)
#define   REG_INTMAP1_UPPER_TAP_TWO_INT_SET(r,v) do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define  REG_INTMAP1_UPPER_TAP_ONE_INT          0x00000001ul
#define   REG_INTMAP1_UPPER_TAP_ONE_INT_GET(r)  (((r) >> 0) & 0x1ul)
#define   REG_INTMAP1_UPPER_TAP_ONE_INT_SET(r,v) do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_INTMAPS2_UPPER                      0x3b
#define  REG_INTMAPS2_UPPER_ERR_FUSE_INT        0x00000080ul
#define   REG_INTMAPS2_UPPER_ERR_FUSE_INT_GET(r) (((r) >> 7) & 0x1ul)
#define   REG_INTMAPS2_UPPER_ERR_FUSE_INT_SET(r,v)  do { (r) = ((r) & ~0x80ul) | (((v) & (0x1ul)) << 7); } while(0)
#define  REG_INTMAPS2_UPPER_ERR_USER_REGS_INT   0x00000040ul
#define   REG_INTMAPS2_UPPER_ERR_USER_REGS_INT_GET(r)     (((r) >> 6) & 0x1ul)
#define   REG_INTMAPS2_UPPER_ERR_USER_REGS_INT_SET(r,v)       do { (r) = ((r) & ~0x40ul) | (((v) & (0x1ul)) << 6); } while(0)
#define  REG_INTMAPS2_UPPER_KPLAV_TIMER_INT     0x00000010ul
#define   REG_INTMAPS2_UPPER_KPLAV_TIMER_INT_GET(r)   (((r) >> 4) & 0x1ul)
#define   REG_INTMAPS2_UPPER_KPLAV_TIMER_INT_SET(r,v)     do { (r) = ((r) & ~0x10ul) | (((v) & (0x1ul)) << 4); } while(0)
#define  REG_INTMAPS2_UPPER_TEMP_ADC_HI_INT     0x00000008ul
#define   REG_INTMAPS2_UPPER_TEMP_ADC_HI_INT_GET(r)   (((r) >> 3) & 0x1ul)
#define   REG_INTMAPS2_UPPER_TEMP_ADC_HI_INT_SET(r,v)     do { (r) = ((r) & ~0x8ul) | (((v) & (0x1ul)) << 3); } while(0)
#define  REG_INTMAPS2_UPPER_TEMP_ADC_LOW_INT    0x00000004ul
#define   REG_INTMAPS2_UPPER_TEMP_ADC_LOW_INT_GET(r)    (((r) >> 2) & 0x1ul)
#define   REG_INTMAPS2_UPPER_TEMP_ADC_LOW_INT_SET(r,v)      do { (r) = ((r) & ~0x4ul) | (((v) & (0x1ul)) << 2); } while(0)
#define  REG_INTMAPS2_UPPER_TAP_TWO_INT         0x00000002ul
#define   REG_INTMAPS2_UPPER_TAP_TWO_INT_GET(r) (((r) >> 1) & 0x1ul)
#define   REG_INTMAPS2_UPPER_TAP_TWO_INT_SET(r,v) do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define  REG_INTMAPS2_UPPER_TAP_ONE_INT         0x00000001ul
#define   REG_INTMAPS2_UPPER_TAP_ONE_INT_GET(r) (((r) >> 0) & 0x1ul)
#define   REG_INTMAPS2_UPPER_TAP_ONE_INT_SET(r,v) do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_ADC_CTL                             0x3c
#define  REG_ADC_CTL_FIFO_8_12BIT_MASK          0x000000c0ul
#define   REG_ADC_CTL_FIFO_8_12BIT_SHIFT        6
#define   REG_ADC_CTL_FIFO_8_12BIT_GET(r)       (((r) >> 6) & 0x3ul)
#define   REG_ADC_CTL_FIFO_8_12BIT_SET(r,v)     do { (r) = ((r) & ~0xc0ul) | (((v) & (0x3ul)) << 6); } while(0)
#define  REG_ADC_CTL_ADC_INACT_EN               0x00000008ul
#define   REG_ADC_CTL_ADC_INACT_EN_GET(r)       (((r) >> 3) & 0x1ul)
#define   REG_ADC_CTL_ADC_INACT_EN_SET(r,v)     do { (r) = ((r) & ~0x8ul) | (((v) & (0x1ul)) << 3); } while(0)
#define  REG_ADC_CTL_ADC_ACT_EN                 0x00000002ul
#define   REG_ADC_CTL_ADC_ACT_EN_GET(r)         (((r) >> 1) & 0x1ul)
#define   REG_ADC_CTL_ADC_ACT_EN_SET(r,v)       do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define  REG_ADC_CTL_ADC_EN                     0x00000001ul
#define   REG_ADC_CTL_ADC_EN_GET(r)             (((r) >> 0) & 0x1ul)
#define   REG_ADC_CTL_ADC_EN_SET(r,v)           do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_TEMP_CTL                            0x3d
#define  REG_TEMP_CTL_NL_COMP_EN                0x00000080ul
#define   REG_TEMP_CTL_NL_COMP_EN_GET(r)        (((r) >> 7) & 0x1ul)
#define   REG_TEMP_CTL_NL_COMP_EN_SET(r,v)      do { (r) = ((r) & ~0x80ul) | (((v) & (0x1ul)) << 7); } while(0)
#define  REG_TEMP_CTL_TEMP_INACT_EN             0x00000008ul
#define   REG_TEMP_CTL_TEMP_INACT_EN_GET(r)     (((r) >> 3) & 0x1ul)
#define   REG_TEMP_CTL_TEMP_INACT_EN_SET(r,v)   do { (r) = ((r) & ~0x8ul) | (((v) & (0x1ul)) << 3); } while(0)
#define  REG_TEMP_CTL_TEMP_ACT_EN               0x00000002ul
#define   REG_TEMP_CTL_TEMP_ACT_EN_GET(r)       (((r) >> 1) & 0x1ul)
#define   REG_TEMP_CTL_TEMP_ACT_EN_SET(r,v)     do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define  REG_TEMP_CTL_TEMP_EN                   0x00000001ul
#define   REG_TEMP_CTL_TEMP_EN_GET(r)           (((r) >> 0) & 0x1ul)
#define   REG_TEMP_CTL_TEMP_EN_SET(r,v)         do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_TEMP_ADC_OVER_THRSH_H               0x3e
#define  REG_TEMP_ADC_OVER_THRSH_H_THRESH_MASK  0x0000007ful
#define   REG_TEMP_ADC_OVER_THRSH_H_THRESH_SHIFT 0
#define   REG_TEMP_ADC_OVER_THRSH_H_THRESH_GET(r) (((r) >> 0) & 0x7ful)
#define   REG_TEMP_ADC_OVER_THRSH_H_THRESH_SET(r,v)   do { (r) = ((r) & ~0x7ful) | (((v) & (0x7ful)) << 0); } while(0)
#define REG_TEMP_ADC_OVER_THRSH_L               0x3f
#define  REG_TEMP_ADC_OVER_THRSH_L_THRESH_MASK  0x000000fcul
#define   REG_TEMP_ADC_OVER_THRSH_L_THRESH_SHIFT 2
#define   REG_TEMP_ADC_OVER_THRSH_L_THRESH_GET(r) (((r) >> 2) & 0x3ful)
#define   REG_TEMP_ADC_OVER_THRSH_L_THRESH_SET(r,v)   do { (r) = ((r) & ~0xfcul) | (((v) & (0x3ful)) << 2); } while(0)
#define REG_TEMP_ADC_UNDER_THRSH_H              0x40
#define  REG_TEMP_ADC_UNDER_THRSH_H_THRESH_MASK 0x0000007ful
#define   REG_TEMP_ADC_UNDER_THRSH_H_THRESH_SHIFT 0
#define   REG_TEMP_ADC_UNDER_THRSH_H_THRESH_GET(r)  (((r) >> 0) & 0x7ful)
#define   REG_TEMP_ADC_UNDER_THRSH_H_THRESH_SET(r,v)    do { (r) = ((r) & ~0x7ful) | (((v) & (0x7ful)) << 0); } while(0)
#define REG_TEMP_ADC_UNDER_THRSH_L              0x41
#define  REG_TEMP_ADC_UNDER_THRSH_L_THRESH_MASK 0x000000fcul
#define   REG_TEMP_ADC_UNDER_THRSH_L_THRESH_SHIFT 2
#define   REG_TEMP_ADC_UNDER_THRSH_L_THRESH_GET(r)  (((r) >> 2) & 0x3ful)
#define   REG_TEMP_ADC_UNDER_THRSH_L_THRESH_SET(r,v)    do { (r) = ((r) & ~0xfcul) | (((v) & (0x3ful)) << 2); } while(0)
#define REG_TEMP_ADC_TIMER                      0x42
#define  REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_INACT_MASK     0x000000f0ul
#define   REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_INACT_SHIFT       4
#define   REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_INACT_GET(r)        (((r) >> 4) & 0xful)
#define   REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_INACT_SET(r,v)          do { (r) = ((r) & ~0xf0ul) | (((v) & (0xful)) << 4); } while(0)
#define  REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_ACT_MASK   0x0000000ful
#define   REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_ACT_SHIFT     0
#define   REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_ACT_GET(r)      (((r) >> 0) & 0xful)
#define   REG_TEMP_ADC_TIMER_TIMER_TEMP_ADC_ACT_SET(r,v)        do { (r) = ((r) & ~0xful) | (((v) & (0xful)) << 0); } while(0)
#define REG_AXIS_MASK                           0x43
#define  REG_AXIS_MASK_TAP_AXIS_MASK            0x00000030ul
#define   REG_AXIS_MASK_TAP_AXIS_SHIFT          4
#define   REG_AXIS_MASK_TAP_AXIS_GET(r)         (((r) >> 4) & 0x3ul)
#define   REG_AXIS_MASK_TAP_AXIS_SET(r,v)       do { (r) = ((r) & ~0x30ul) | (((v) & (0x3ul)) << 4); } while(0)
#define  REG_AXIS_MASK_ACT_INACT_Z              0x00000004ul
#define   REG_AXIS_MASK_ACT_INACT_Z_GET(r)      (((r) >> 2) & 0x1ul)
#define   REG_AXIS_MASK_ACT_INACT_Z_SET(r,v)    do { (r) = ((r) & ~0x4ul) | (((v) & (0x1ul)) << 2); } while(0)
#define  REG_AXIS_MASK_ACT_INACT_Y              0x00000002ul
#define   REG_AXIS_MASK_ACT_INACT_Y_GET(r)      (((r) >> 1) & 0x1ul)
#define   REG_AXIS_MASK_ACT_INACT_Y_SET(r,v)    do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define  REG_AXIS_MASK_ACT_INACT_X              0x00000001ul
#define   REG_AXIS_MASK_ACT_INACT_X_GET(r)      (((r) >> 0) & 0x1ul)
#define   REG_AXIS_MASK_ACT_INACT_X_SET(r,v)    do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_STATUS_COPY                         0x44
#define  REG_STATUS_COPY_ERR_USER_REGS          0x00000080ul
#define   REG_STATUS_COPY_ERR_USER_REGS_GET(r)  (((r) >> 7) & 0x1ul)
#define   REG_STATUS_COPY_ERR_USER_REGS_SET(r,v) do { (r) = ((r) & ~0x80ul) | (((v) & (0x1ul)) << 7); } while(0)
#define  REG_STATUS_COPY_AWAKE                  0x00000040ul
#define   REG_STATUS_COPY_AWAKE_GET(r)          (((r) >> 6) & 0x1ul)
#define   REG_STATUS_COPY_AWAKE_SET(r,v)        do { (r) = ((r) & ~0x40ul) | (((v) & (0x1ul)) << 6); } while(0)
#define  REG_STATUS_COPY_INACT                  0x00000020ul
#define   REG_STATUS_COPY_INACT_GET(r)          (((r) >> 5) & 0x1ul)
#define   REG_STATUS_COPY_INACT_SET(r,v)        do { (r) = ((r) & ~0x20ul) | (((v) & (0x1ul)) << 5); } while(0)
#define  REG_STATUS_COPY_ACT                    0x00000010ul
#define   REG_STATUS_COPY_ACT_GET(r)            (((r) >> 4) & 0x1ul)
#define   REG_STATUS_COPY_ACT_SET(r,v)          do { (r) = ((r) & ~0x10ul) | (((v) & (0x1ul)) << 4); } while(0)
#define  REG_STATUS_COPY_FIFO_OVER_RUN          0x00000008ul
#define   REG_STATUS_COPY_FIFO_OVER_RUN_GET(r)  (((r) >> 3) & 0x1ul)
#define   REG_STATUS_COPY_FIFO_OVER_RUN_SET(r,v) do { (r) = ((r) & ~0x8ul) | (((v) & (0x1ul)) << 3); } while(0)
#define  REG_STATUS_COPY_FIFO_WATER_MARK        0x00000004ul
#define   REG_STATUS_COPY_FIFO_WATER_MARK_GET(r) (((r) >> 2) & 0x1ul)
#define   REG_STATUS_COPY_FIFO_WATER_MARK_SET(r,v)  do { (r) = ((r) & ~0x4ul) | (((v) & (0x1ul)) << 2); } while(0)
#define  REG_STATUS_COPY_FIFO_READY             0x00000002ul
#define   REG_STATUS_COPY_FIFO_READY_GET(r)     (((r) >> 1) & 0x1ul)
#define   REG_STATUS_COPY_FIFO_READY_SET(r,v)   do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define  REG_STATUS_COPY_DATA_READY             0x00000001ul
#define   REG_STATUS_COPY_DATA_READY_GET(r)     (((r) >> 0) & 0x1ul)
#define   REG_STATUS_COPY_DATA_READY_SET(r,v)   do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_STATUS_2                            0x45
#define  REG_STATUS_2_ERR_FUSE_REGS             0x00000080ul
#define   REG_STATUS_2_ERR_FUSE_REGS_GET(r)     (((r) >> 7) & 0x1ul)
#define   REG_STATUS_2_ERR_FUSE_REGS_SET(r,v)   do { (r) = ((r) & ~0x80ul) | (((v) & (0x1ul)) << 7); } while(0)
#define  REG_STATUS_2_FUSE_REFRESH              0x00000040ul
#define   REG_STATUS_2_FUSE_REFRESH_GET(r)      (((r) >> 6) & 0x1ul)
#define   REG_STATUS_2_FUSE_REFRESH_SET(r,v)    do { (r) = ((r) & ~0x40ul) | (((v) & (0x1ul)) << 6); } while(0)
#define  REG_STATUS_2_TIMER                     0x00000010ul
#define   REG_STATUS_2_TIMER_GET(r)             (((r) >> 4) & 0x1ul)
#define   REG_STATUS_2_TIMER_SET(r,v)           do { (r) = ((r) & ~0x10ul) | (((v) & (0x1ul)) << 4); } while(0)
#define  REG_STATUS_2_TEMP_ADC_HI               0x00000008ul
#define   REG_STATUS_2_TEMP_ADC_HI_GET(r)       (((r) >> 3) & 0x1ul)
#define   REG_STATUS_2_TEMP_ADC_HI_SET(r,v)     do { (r) = ((r) & ~0x8ul) | (((v) & (0x1ul)) << 3); } while(0)
#define  REG_STATUS_2_TEMP_ADC_LOW              0x00000004ul
#define   REG_STATUS_2_TEMP_ADC_LOW_GET(r)      (((r) >> 2) & 0x1ul)
#define   REG_STATUS_2_TEMP_ADC_LOW_SET(r,v)    do { (r) = ((r) & ~0x4ul) | (((v) & (0x1ul)) << 2); } while(0)
#define  REG_STATUS_2_TAP_TWO                   0x00000002ul
#define   REG_STATUS_2_TAP_TWO_GET(r)           (((r) >> 1) & 0x1ul)
#define   REG_STATUS_2_TAP_TWO_SET(r,v)         do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define  REG_STATUS_2_TAP_ONE                   0x00000001ul
#define   REG_STATUS_2_TAP_ONE_GET(r)           (((r) >> 0) & 0x1ul)
#define   REG_STATUS_2_TAP_ONE_SET(r,v)         do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_STATUS_3                            0x46
#define  REG_STATUS_3_PEDOMETER_OVERFLOW        0x00000001ul
#define   REG_STATUS_3_PEDOMETER_OVERFLOW_GET(r) (((r) >> 0) & 0x1ul)
#define   REG_STATUS_3_PEDOMETER_OVERFLOW_SET(r,v)  do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_PEDOMETER_STEP_CNT_H                0x47
#define REG_PEDOMETER_STEP_CNT_L                0x48
#define REG_PEDOMETER_CTL                       0x49
#define  REG_PEDOMETER_CTL_RESET_STEP           0x00000004ul
#define   REG_PEDOMETER_CTL_RESET_STEP_GET(r)   (((r) >> 2) & 0x1ul)
#define   REG_PEDOMETER_CTL_RESET_STEP_SET(r,v) do { (r) = ((r) & ~0x4ul) | (((v) & (0x1ul)) << 2); } while(0)
#define  REG_PEDOMETER_CTL_RESET_OF             0x00000002ul
#define   REG_PEDOMETER_CTL_RESET_OF_GET(r)     (((r) >> 1) & 0x1ul)
#define   REG_PEDOMETER_CTL_RESET_OF_SET(r,v)   do { (r) = ((r) & ~0x2ul) | (((v) & (0x1ul)) << 1); } while(0)
#define  REG_PEDOMETER_CTL_EN                   0x00000001ul
#define   REG_PEDOMETER_CTL_EN_GET(r)           (((r) >> 0) & 0x1ul)
#define   REG_PEDOMETER_CTL_EN_SET(r,v)         do { (r) = ((r) & ~0x1ul) | (((v) & (0x1ul)) << 0); } while(0)
#define REG_PEDOMETER_THRES_H                   0x4a
#define  REG_PEDOMETER_THRES_H_THRESHOLD_MASK   0x0000007ful
#define   REG_PEDOMETER_THRES_H_THRESHOLD_SHIFT 0
#define   REG_PEDOMETER_THRES_H_THRESHOLD_GET(r) (((r) >> 0) & 0x7ful)
#define   REG_PEDOMETER_THRES_H_THRESHOLD_SET(r,v)  do { (r) = ((r) & ~0x7ful) | (((v) & (0x7ful)) << 0); } while(0)
#define REG_PEDOMETER_THRES_L                   0x4b
#define  REG_PEDOMETER_THRES_L_THRESHOLD_MASK   0x000000fful
#define   REG_PEDOMETER_THRES_L_THRESHOLD_SHIFT 0
#define   REG_PEDOMETER_THRES_L_THRESHOLD_GET(r) (((r) >> 0) & 0xfful)
#define   REG_PEDOMETER_THRES_L_THRESHOLD_SET(r,v)  do { (r) = ((r) & ~0xfful) | (((v) & (0xfful)) << 0); } while(0)
#define REG_PEDOMETER_SENS_H                    0x4c
#define  REG_PEDOMETER_SENS_H_SENSITIVITY_MASK  0x0000007ful
#define   REG_PEDOMETER_SENS_H_SENSITIVITY_SHIFT 0
#define   REG_PEDOMETER_SENS_H_SENSITIVITY_GET(r) (((r) >> 0) & 0x7ful)
#define   REG_PEDOMETER_SENS_H_SENSITIVITY_SET(r,v)   do { (r) = ((r) & ~0x7ful) | (((v) & (0x7ful)) << 0); } while(0)
#define REG_PEDOMETER_SENS_L                    0x4d
#define  REG_PEDOMETER_SENS_L_SENSITIVITY_MASK  0x000000fful
#define   REG_PEDOMETER_SENS_L_SENSITIVITY_SHIFT 0
#define   REG_PEDOMETER_SENS_L_SENSITIVITY_GET(r) (((r) >> 0) & 0xfful)
#define   REG_PEDOMETER_SENS_L_SENSITIVITY_SET(r,v)   do { (r) = ((r) & ~0xfful) | (((v) & (0xfful)) << 0); } while(0)

#endif
