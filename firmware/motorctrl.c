#include "motorctrl.h"
#include "dac.h"
#include "usb.h"
#include "dma_spi.h"
#include "debugdata.h"

const unsigned int K_STARTUP_PWR = 0;

/* Controller parameters */
const int32_t K_CONTROL_MIN = 0;
const int32_t K_CONTROL_MAX = 4095;
const int32_t K_CONTROL_CENTER = 2048; //(K_CONTROL_MAX + K_CONTROL_MIN) / 2;

const int32_t K_INTEGRAL_CLAMP = 1000000;

int des_speed = 0;

int32_t integral = 0;

uint32_t seq_num = 0;
int debug_out = 0;

/* Set desired speed of motor. */
void set_speed(int speed)
{
    des_speed = speed;
}

/* Initiate motor startup sequence. */
void motor_startup(void)
{
    laser_low_on();
    set_motor_power(K_STARTUP_PWR);
}

/* Enable controller debug output to USB endpoint. */
void enable_debug_out(int enable)
{
    if(enable) {
        seq_num = 0;
        debug_out = 1;
    } else {
        debug_out = 0;
    }
}

/* Run a single PI controller step. Returns current difference between speed 
   and desired speed. */
int motor_ctrl_step(uint32_t delta)
{
    int32_t speed = (uint32_t) 1000000000 / delta;
    
    int32_t control = K_CONTROL_CENTER + (speed - des_speed) + integral/128;
    if(control > K_CONTROL_MAX) control = K_CONTROL_MAX;
    if(control < K_CONTROL_MIN) control = K_CONTROL_MIN;
    
    integral += (speed - des_speed);
    
    if(integral > K_INTEGRAL_CLAMP) integral = K_INTEGRAL_CLAMP;
    if(integral < -K_INTEGRAL_CLAMP) integral = -K_INTEGRAL_CLAMP;
    
    set_motor_power(control);
    
    if(debug_out) {
        struct debug_data_t debug_data = {
            .seq_num = seq_num,
            .speed = speed,
            .des_speed = des_speed,
            .control = control,
            .delta = delta,
        };
        
        seq_num++;
        usb_put_debug_packet(&debug_data);
    }
    
    return (speed - des_speed);
}
