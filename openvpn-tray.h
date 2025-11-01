#ifndef OPENVPN_TRAY_H
#define OPENVPN_TRAY_H

#define APP_NAME "openvpn-tray"
#define APP_VERSION "0.7"
#define OPENVPN_CONF_DIR "/etc/openvpn/"
#define MAX_VPNS 100
#define MAX_VPN_NAME_LEN 32
#define STATUS_SUMMARY_INTERVAL 600

extern int read_only_mode;

int check_privileges(void);

#endif
