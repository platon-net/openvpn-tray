#include <stdio.h>
#include <string.h>
#include "openvpn-tray.h"
#include "logging.h"

int should_log_status_summary(void)
{
    time_t current_time = time(NULL);
    return (current_time - last_log_time) >= STATUS_SUMMARY_INTERVAL;
}

void update_log_time(void)
{
    last_log_time = time(NULL);
}

void print_vpn_status_summary(void)
{
    if (vpn_count == 0) {
        printf("No VPNs configured\n");
        return;
    }

    // Calculate maximum VPN name length
    int max_name_len = 8; // Minimum for "VPN Name"
    for (int i = 0; i < vpn_count; i++) {
        int len = strlen(vpn_labels[i]);
        if (len > max_name_len) {
            max_name_len = len;
        }
    }

    // Table dimensions
    int status_col_width = 6; // "Status"
    int total_width = max_name_len + 3 + status_col_width + 3 + 1; // padding + borders
    
    // Calculate centering for title
    const char *title = " " APP_NAME " status ";
    int title_len = strlen(title);
    int available_space = total_width - 2; // minus the + borders
    int left_padding = (available_space - title_len) / 2;
    int right_padding = available_space - title_len - left_padding;

    // Top border with centered title
    printf("+");
    for (int i = 0; i < left_padding; i++) printf("-");
    printf("%s", title);
    for (int i = 0; i < right_padding; i++) printf("-");
    printf("+\n");

    // Column headers
    printf("| %-*s | %-*s |\n", max_name_len, "VPN Name", status_col_width, "Status");

    // Separator row
    printf("+");
    for (int i = 0; i < max_name_len + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < status_col_width + 2; i++) printf("-");
    printf("+\n");

    // VPN entries
    for (int i = 0; i < vpn_count; i++) {
        printf("| %-*s | %-*s |\n", 
                max_name_len, vpn_labels[i], 
                status_col_width, vpn_states[i] ? "ON" : "OFF");
    }

    // Bottom border
    printf("+");
    for (int i = 0; i < max_name_len + 2; i++) printf("-");
    printf("+");
    for (int i = 0; i < status_col_width + 2; i++) printf("-");
    printf("+\n");
}

void log_vpn_status_changes(void)
{
    int changes_detected = 0;
    int force_summary = should_log_status_summary();
    
    if (first_run || force_summary) {
        print_vpn_status_summary();
        changes_detected = 1;
        first_run = 0;
    } else {
        for (int i = 0; i < vpn_count; i++) {
            if (previous_vpn_states[i] != vpn_states[i]) {
                printf("%s: VPN %s changed from %s to %s\n", 
                       APP_NAME, vpn_labels[i],
                       previous_vpn_states[i] ? "ON" : "OFF",
                       vpn_states[i] ? "ON" : "OFF");
                changes_detected = 1;
            }
        }
        
        if (changes_detected) {
            print_vpn_status_summary();
        }
    }
    
    if (changes_detected) {
        update_log_time();
    }
    
    for (int i = 0; i < vpn_count; i++) {
        previous_vpn_states[i] = vpn_states[i];
    }
}
