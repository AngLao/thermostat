#ifndef _INTERRUPT_H_
#define	_INTERRUPT_H_


void set_fan_output(int fan_inex, int mode);
void set_thyristor_output(int time);

int get_pwm_value(void);
char get_fan_mode(int fan_inex);

void interrupt_init(void);
void interrupt_task(void *arg);


#endif