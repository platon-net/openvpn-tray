#ifndef LOGGING_H
#define LOGGING_H

#include <time.h>

extern char vpn_labels[][32];
extern int vpn_states[];
extern int previous_vpn_states[];
extern int vpn_count;
extern time_t last_log_time;
extern int first_run;

void print_vpn_status_summary(void);
void log_vpn_status_changes(void);
int should_log_status_summary(void);
void update_log_time(void);

#endif
