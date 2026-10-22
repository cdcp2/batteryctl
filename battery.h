#ifndef BATTERY_H
#define BATTERY_H

int print_battery_status(void);
int set_charge_limit(unsigned int percentage);
int set_performance_mode(const char *mode);
int print_battery_health(void);

#endif
