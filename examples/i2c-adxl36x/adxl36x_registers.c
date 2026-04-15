#include <stddef.h>
char *adi_adxl36x_reg_name(unsigned addr)
{
    switch(addr) {
    case 0x00u: return "devid_ad";
    case 0x01u: return "devid_mst";
    case 0x02u: return "part_id";
    case 0x03u: return "rev_id";
    case 0x05u: return "serial_number_2";
    case 0x06u: return "serial_number_1";
    case 0x07u: return "serial_number_0";
    case 0x08u: return "xdata";
    case 0x09u: return "ydata";
    case 0x0au: return "zdata";
    case 0x0bu: return "status";
    case 0x0cu: return "fifo_entries_l";
    case 0x0du: return "fifo_entries_h";
    case 0x0eu: return "xdata_h";
    case 0x0fu: return "xdata_l";
    case 0x10u: return "ydata_h";
    case 0x11u: return "ydata_l";
    case 0x12u: return "zdata_h";
    case 0x13u: return "zdata_l";
    case 0x14u: return "temp_h";
    case 0x15u: return "temp_l";
    case 0x16u: return "ex_adc_h";
    case 0x17u: return "ex_adc_l";
    case 0x18u: return "i2c_fifo_data";
    case 0x1fu: return "soft_reset";
    case 0x20u: return "thresh_act_h";
    case 0x21u: return "thresh_act_l";
    case 0x22u: return "time_act";
    case 0x23u: return "thresh_inact_h";
    case 0x24u: return "thresh_inact_l";
    case 0x25u: return "time_inact_h";
    case 0x26u: return "time_inact_l";
    case 0x27u: return "act_inact_ctl";
    case 0x28u: return "fifo_control";
    case 0x29u: return "fifo_samples";
    case 0x2au: return "intmap1_lower";
    case 0x2bu: return "intmap2_lower";
    case 0x2cu: return "filter_ctl";
    case 0x2du: return "power_ctl";
    case 0x2eu: return "self_test";
    case 0x2fu: return "tap_thresh";
    case 0x30u: return "tap_dur";
    case 0x31u: return "tap_latent";
    case 0x32u: return "tap_window";
    case 0x33u: return "x_offset";
    case 0x34u: return "y_offset";
    case 0x35u: return "z_offset";
    case 0x36u: return "x_sens";
    case 0x37u: return "y_sens";
    case 0x38u: return "z_sens";
    case 0x39u: return "timer_ctl";
    case 0x3au: return "intmap1_upper";
    case 0x3bu: return "intmaps2_upper";
    case 0x3cu: return "adc_ctl";
    case 0x3du: return "temp_ctl";
    case 0x3eu: return "temp_adc_over_thrsh_h";
    case 0x3fu: return "temp_adc_over_thrsh_l";
    case 0x40u: return "temp_adc_under_thrsh_h";
    case 0x41u: return "temp_adc_under_thrsh_l";
    case 0x42u: return "temp_adc_timer";
    case 0x43u: return "axis_mask";
    case 0x44u: return "status_copy";
    case 0x45u: return "status_2";
    case 0x46u: return "status_3";
    case 0x47u: return "pedometer_step_cnt_h";
    case 0x48u: return "pedometer_step_cnt_l";
    case 0x49u: return "pedometer_ctl";
    case 0x4au: return "pedometer_thres_h";
    case 0x4bu: return "pedometer_thres_l";
    case 0x4cu: return "pedometer_sens_h";
    case 0x4du: return "pedometer_sens_l";
    }
    return NULL;
}

void adi_adxl36x_print_fields(unsigned addr, unsigned long value, int (*uprintf)(const char *, ...))
{
    switch(addr) {
    case 0x00u:
        uprintf("0x%08lx", value);
        return;
    case 0x01u:
        uprintf("0x%08lx", value);
        return;
    case 0x02u:
        uprintf("0x%08lx", value);
        return;
    case 0x03u:
        uprintf("0x%08lx", value);
        return;
    case 0x05u:
        uprintf("bit16:%d", (value >> 0) & 1);
        return;
    case 0x06u:
        uprintf("0x%08lx", value);
        return;
    case 0x07u:
        uprintf("0x%08lx", value);
        return;
    case 0x08u:
        uprintf("0x%08lx", value);
        return;
    case 0x09u:
        uprintf("0x%08lx", value);
        return;
    case 0x0au:
        uprintf("0x%08lx", value);
        return;
    case 0x0bu:
        uprintf("err_user_regs:%d", (value >> 7) & 1);
        uprintf(",awake:%d", (value >> 6) & 1);
        uprintf(",inact:%d", (value >> 5) & 1);
        uprintf(",act:%d", (value >> 4) & 1);
        uprintf(",fifo_over_run:%d", (value >> 3) & 1);
        uprintf(",fifo_water_mark:%d", (value >> 2) & 1);
        uprintf(",fifo_ready:%d", (value >> 1) & 1);
        uprintf(",data_ready:%d", (value >> 0) & 1);
        return;
    case 0x0cu:
        uprintf("0x%08lx", value);
        return;
    case 0x0du:
        uprintf("0x%08lx", value);
        return;
    case 0x0eu:
        uprintf("0x%08lx", value);
        return;
    case 0x0fu:
        uprintf("data:%d", (value >> 2) & 63);
        return;
    case 0x10u:
        uprintf("0x%08lx", value);
        return;
    case 0x11u:
        uprintf("data:%d", (value >> 2) & 63);
        return;
    case 0x12u:
        uprintf("0x%08lx", value);
        return;
    case 0x13u:
        uprintf("data:%d", (value >> 2) & 63);
        return;
    case 0x14u:
        uprintf("0x%08lx", value);
        return;
    case 0x15u:
        uprintf("data:%d", (value >> 2) & 63);
        return;
    case 0x16u:
        uprintf("0x%08lx", value);
        return;
    case 0x17u:
        uprintf("data:%d", (value >> 2) & 63);
        return;
    case 0x18u:
        uprintf("0x%08lx", value);
        return;
    case 0x1fu:
        uprintf("soft_reset:%d", (value >> 1) & 1);
        return;
    case 0x20u:
        uprintf("data:%d", (value >> 0) & 127);
        return;
    case 0x21u:
        uprintf("data:%d", (value >> 2) & 63);
        return;
    case 0x22u:
        uprintf("0x%08lx", value);
        return;
    case 0x23u:
        uprintf("data:%d", (value >> 0) & 127);
        return;
    case 0x24u:
        uprintf("data:%d", (value >> 2) & 63);
        return;
    case 0x25u:
        uprintf("0x%08lx", value);
        return;
    case 0x26u:
        uprintf("0x%08lx", value);
        return;
    case 0x27u:
        uprintf("reg_readback:%d", (value >> 6) & 3);
        uprintf(",linkloop:");
        switch((value >> 4) & 3) {
        case 0: uprintf("default1"); break;
        case 1: uprintf("linked"); break;
        case 2: uprintf("default2"); break;
        case 3: uprintf("looped");
        }
        uprintf(",intact_en:%d", (value >> 2) & 3);
        uprintf(",act_en:%d", (value >> 0) & 3);
        return;
    case 0x28u:
        uprintf("channel_select:");
        switch((value >> 3) & 15) {
        case 0: uprintf("xyz"); break;
        case 1: uprintf("x"); break;
        case 2: uprintf("y"); break;
        case 3: uprintf("z"); break;
        case 4: uprintf("xyzt"); break;
        case 5: uprintf("xt"); break;
        case 6: uprintf("yt"); break;
        case 7: uprintf("zt"); break;
        case 8: uprintf("xyza"); break;
        case 9: uprintf("xa"); break;
        case 10: uprintf("ya"); break;
        case 11: uprintf("za"); break;
        default: uprintf("%d", (value >> 3) & 15);
        }
        uprintf(",fifo_samples:%d", (value >> 2) & 1);
        uprintf(",fifo_mode:");
        switch((value >> 0) & 3) {
        case 0: uprintf("disabled"); break;
        case 1: uprintf("oldest"); break;
        case 2: uprintf("stream"); break;
        case 3: uprintf("trigger");
        }
        return;
    case 0x29u:
        uprintf("0x%08lx", value);
        return;
    case 0x2au:
        uprintf("int_low_int:%d", (value >> 7) & 1);
        uprintf(",awake_int:%d", (value >> 6) & 1);
        uprintf(",inact_int:%d", (value >> 5) & 1);
        uprintf(",act_int:%d", (value >> 4) & 1);
        uprintf(",fifo_overrun_int:%d", (value >> 3) & 1);
        uprintf(",fifo_water_mark_int:%d", (value >> 2) & 1);
        uprintf(",fifo_rdy_int:%d", (value >> 1) & 1);
        uprintf(",data_rdy_int:%d", (value >> 0) & 1);
        return;
    case 0x2bu:
        uprintf("int_low_int:%d", (value >> 7) & 1);
        uprintf(",awake_int:%d", (value >> 6) & 1);
        uprintf(",inact_int:%d", (value >> 5) & 1);
        uprintf(",act_int:%d", (value >> 4) & 1);
        uprintf(",fifo_overrun_int:%d", (value >> 3) & 1);
        uprintf(",fifo_water_mark_int:%d", (value >> 2) & 1);
        uprintf(",fifo_rdy_int:%d", (value >> 1) & 1);
        uprintf(",data_rdy_int:%d", (value >> 0) & 1);
        return;
    case 0x2cu:
        uprintf("range:%d", (value >> 6) & 3);
        uprintf(",i2c_hs:%d", (value >> 5) & 1);
        uprintf(",ext_sample:%d", (value >> 3) & 1);
        uprintf(",odr:%d", (value >> 0) & 7);
        return;
    case 0x2du:
        uprintf("ext_clk:%d", (value >> 6) & 1);
        uprintf(",noise:%d", (value >> 4) & 3);
        uprintf(",wakeup:%d", (value >> 3) & 1);
        uprintf(",autosleep:%d", (value >> 2) & 1);
        uprintf(",measure:%d", (value >> 0) & 3);
        return;
    case 0x2eu:
        uprintf("st_force:%d", (value >> 1) & 1);
        uprintf(",st:%d", (value >> 0) & 1);
        return;
    case 0x2fu:
        uprintf("0x%08lx", value);
        return;
    case 0x30u:
        uprintf("0x%08lx", value);
        return;
    case 0x31u:
        uprintf("0x%08lx", value);
        return;
    case 0x32u:
        uprintf("0x%08lx", value);
        return;
    case 0x33u:
        uprintf("user_offset:%d", (value >> 0) & 31);
        return;
    case 0x34u:
        uprintf("user_offset:%d", (value >> 0) & 31);
        return;
    case 0x35u:
        uprintf("user_offset:%d", (value >> 0) & 31);
        return;
    case 0x36u:
        uprintf("user_sens:%d", (value >> 0) & 63);
        return;
    case 0x37u:
        uprintf("user_sens:%d", (value >> 0) & 63);
        return;
    case 0x38u:
        uprintf("user_sens:%d", (value >> 0) & 63);
        return;
    case 0x39u:
        uprintf("wakeup_rate:%d", (value >> 6) & 3);
        uprintf(",timer_keep_alive:%d", (value >> 0) & 31);
        return;
    case 0x3au:
        uprintf("err_fuse_int:%d", (value >> 7) & 1);
        uprintf(",err_user_regs_int:%d", (value >> 6) & 1);
        uprintf(",kplav_timer_int:%d", (value >> 4) & 1);
        uprintf(",temp_adc_hi_int:%d", (value >> 3) & 1);
        uprintf(",temp_adc_low_int:%d", (value >> 2) & 1);
        uprintf(",tap_two_int:%d", (value >> 1) & 1);
        uprintf(",tap_one_int:%d", (value >> 0) & 1);
        return;
    case 0x3bu:
        uprintf("err_fuse_int:%d", (value >> 7) & 1);
        uprintf(",err_user_regs_int:%d", (value >> 6) & 1);
        uprintf(",kplav_timer_int:%d", (value >> 4) & 1);
        uprintf(",temp_adc_hi_int:%d", (value >> 3) & 1);
        uprintf(",temp_adc_low_int:%d", (value >> 2) & 1);
        uprintf(",tap_two_int:%d", (value >> 1) & 1);
        uprintf(",tap_one_int:%d", (value >> 0) & 1);
        return;
    case 0x3cu:
        uprintf("fifo_8_12BIT:%d", (value >> 6) & 3);
        uprintf(",adc_inact_en:%d", (value >> 3) & 1);
        uprintf(",adc_act_en:%d", (value >> 1) & 1);
        uprintf(",adc_en:%d", (value >> 0) & 1);
        return;
    case 0x3du:
        uprintf("nl_comp_en:%d", (value >> 7) & 1);
        uprintf(",temp_inact_en:%d", (value >> 3) & 1);
        uprintf(",temp_act_en:%d", (value >> 1) & 1);
        uprintf(",temp_en:%d", (value >> 0) & 1);
        return;
    case 0x3eu:
        uprintf("thresh:%d", (value >> 0) & 127);
        return;
    case 0x3fu:
        uprintf("thresh:%d", (value >> 2) & 63);
        return;
    case 0x40u:
        uprintf("thresh:%d", (value >> 0) & 127);
        return;
    case 0x41u:
        uprintf("thresh:%d", (value >> 2) & 63);
        return;
    case 0x42u:
        uprintf("timer_temp_adc_inact:%d", (value >> 4) & 15);
        uprintf(",timer_temp_adc_act:%d", (value >> 0) & 15);
        return;
    case 0x43u:
        uprintf("tap_axis:%d", (value >> 4) & 3);
        uprintf(",act_inact_z:%d", (value >> 2) & 1);
        uprintf(",act_inact_y:%d", (value >> 1) & 1);
        uprintf(",act_inact_x:%d", (value >> 0) & 1);
        return;
    case 0x44u:
        uprintf("err_user_regs:%d", (value >> 7) & 1);
        uprintf(",awake:%d", (value >> 6) & 1);
        uprintf(",inact:%d", (value >> 5) & 1);
        uprintf(",act:%d", (value >> 4) & 1);
        uprintf(",fifo_over_run:%d", (value >> 3) & 1);
        uprintf(",fifo_water_mark:%d", (value >> 2) & 1);
        uprintf(",fifo_ready:%d", (value >> 1) & 1);
        uprintf(",data_ready:%d", (value >> 0) & 1);
        return;
    case 0x45u:
        uprintf("err_fuse_regs:%d", (value >> 7) & 1);
        uprintf(",fuse_refresh:%d", (value >> 6) & 1);
        uprintf(",timer:%d", (value >> 4) & 1);
        uprintf(",temp_adc_hi:%d", (value >> 3) & 1);
        uprintf(",temp_adc_low:%d", (value >> 2) & 1);
        uprintf(",tap_two:%d", (value >> 1) & 1);
        uprintf(",tap_one:%d", (value >> 0) & 1);
        return;
    case 0x46u:
        uprintf("pedometer_overflow:%d", (value >> 0) & 1);
        return;
    case 0x47u:
        uprintf("0x%08lx", value);
        return;
    case 0x48u:
        uprintf("0x%08lx", value);
        return;
    case 0x49u:
        uprintf("reset_step:%d", (value >> 2) & 1);
        uprintf(",reset_of:%d", (value >> 1) & 1);
        uprintf(",en:%d", (value >> 0) & 1);
        return;
    case 0x4au:
        uprintf("threshold:%d", (value >> 0) & 127);
        return;
    case 0x4bu:
        uprintf("threshold:%d", (value >> 0) & 255);
        return;
    case 0x4cu:
        uprintf("sensitivity:%d", (value >> 0) & 127);
        return;
    case 0x4du:
        uprintf("sensitivity:%d", (value >> 0) & 255);
        return;
    }
}
