#ifndef FAN_H
#define FAN_H

int print_fan_status(void);
int print_available_fan_modes(void);
int set_fan_mode(const char *mode);
int set_cooler_boost(const char *state);

#endif
