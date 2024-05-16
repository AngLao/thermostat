#include "PID.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/param.h>
#include <esp_system.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "interrupt.h"
#include "temperature.h"

void PIDController_Init(PIDController *pid) {

	/* Clear controller variables */
	pid->integrator = 0.0f;
	pid->prevError  = 0.0f;

	pid->differentiator  = 0.0f;
	pid->prevMeasurement = 0.0f;

	pid->out = 0.0f;

}

float PIDController_Update(PIDController *pid, float setpoint, float measurement) {

	/*
	* Error signal
	*/
    float error = setpoint - measurement;


	/*
	* Proportional
	*/
    float proportional = pid->Kp * error;


	/*
	* Integral
	*/
    pid->integrator = pid->integrator + 0.5f * pid->Ki * pid->T * (error + pid->prevError);

	/* Anti-wind-up via integrator clamping */
    if (pid->integrator > pid->limMaxInt) {

        pid->integrator = pid->limMaxInt;

    } else if (pid->integrator < pid->limMinInt) {

        pid->integrator = pid->limMinInt;

    }


	/*
	* Derivative (band-limited differentiator)
	*/
		
    pid->differentiator = -(2.0f * pid->Kd * (measurement - pid->prevMeasurement)	/* Note: derivative on measurement, therefore minus sign in front of equation! */
                        + (2.0f * pid->tau - pid->T) * pid->differentiator)
                        / (2.0f * pid->tau + pid->T);


	/*
	* Compute output and apply limits
	*/
    pid->out = proportional + pid->integrator + pid->differentiator;

    if (pid->out > pid->limMax) {

        pid->out = pid->limMax;

    } else if (pid->out < pid->limMin) {

        pid->out = pid->limMin;

    }

	/* Store error and measurement for later use */
    pid->prevError       = error;
    pid->prevMeasurement = measurement;

	/* Return controller output */
    return pid->out;

}


static PIDController temperature_controller = { .Kp = -4820,
                                                .Ki = -123,
                                                .Kd = 0,
                                                .T = 0.2,
                                                .tau = 1,
                                                .limMin = -3000,
                                                .limMax = 3000,
                                                .limMinInt = -4000,
                                                .limMaxInt = 4000,
};

void control_task(void* arg)
{
    const int offest = 5000;
    PIDController_Init(&temperature_controller);
    while(true){
        //传感器未接入停止控制温度
        if(!temperature_device_online()){
        	int pwm_out = offest + (int)temperature_controller.limMax;
            set_thyristor_output(pwm_out);
            set_fan_output(1,0);
            vTaskDelay(20/portTICK_PERIOD_MS);
            continue;
        }

        float setpoint = get_expect_temperature();
        float measurement = get_measure_temperature();
        float out = PIDController_Update(&temperature_controller, setpoint, measurement);
        int pwm_out = offest + (int)out;
        set_thyristor_output(pwm_out);

        //超过0.2度打开风扇
        if((measurement - setpoint) > 0.2)
            set_fan_output(1,1);
        else
            set_fan_output(1,0);
        
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
}

void set_temperature_controller_kp(float value)
{
	temperature_controller.Kp = value;
}

void set_temperature_controller_ki(float value)
{
	temperature_controller.Ki = value;
}


float get_temperature_controller_kp(void)
{
	return temperature_controller.Kp;
}

float get_temperature_controller_ki(void)
{
	return temperature_controller.Ki;
}
