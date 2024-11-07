#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <gtk/gtk.h>

#define OPENVPN_CONF_DIR "/etc/openvpn/"
#define MAX_VPNS 100
#define MAX_VPN_NAME_LEN 32

//#include "openvpn-on.xpm"
//#include "openvpn-off.xpm"

static char vpn_labels[MAX_VPNS][MAX_VPN_NAME_LEN];
static int vpn_states[MAX_VPNS];
static int vpn_count = 0;
static int update_interval = 10;
static GdkPixbuf *pixbuf_on = NULL;
static GdkPixbuf *pixbuf_off = NULL;


void update_icon(GtkStatusIcon *tray_icon);
void fetch_vpn_list(GtkStatusIcon *tray_icon);
GtkWidget* create_vpn_list(GtkStatusIcon *tray_icon);
void on_vpn_toggle(GtkCheckMenuItem *item, gpointer data);
void on_all_vpn_toggle(GtkMenuItem *item, gpointer tray_icon);
void turn_on_vpn(const char *vpn_name);
void turn_off_vpn(const char *vpn_name);
void turn_on_all_vpns(void);
void turn_off_all_vpns(void);
gboolean refresh_vpn_list(gpointer tray_icon);
void on_tray_icon_left_click(GtkStatusIcon *tray_icon);
void on_tray_icon_right_click(GtkStatusIcon *tray_icon, guint button, guint activate_time);
GtkWidget* create_right_click_menu(GtkStatusIcon *tray_icon);
void show_preferences_dialog(GtkStatusIcon *tray_icon);

void load_icons() {
    pixbuf_on = gdk_pixbuf_new_from_resource("/org/platon/images/openvpn-on.png", NULL);
    pixbuf_off = gdk_pixbuf_new_from_resource("/org/platon/images/openvpn-off.png", NULL);
}

void update_icon(GtkStatusIcon *tray_icon) {
    int any_vpn_on = 0;

    // Check if any VPN is ON
    for (int i = 0; i < vpn_count; i++) {
        if (vpn_states[i] == 1) {
            any_vpn_on = 1;
            break;
        }
    }

    // Use the preloaded pixbufs based on VPN state
    if (any_vpn_on) {
        gtk_status_icon_set_from_pixbuf(tray_icon, pixbuf_on);
        gtk_status_icon_set_tooltip_text(tray_icon, "OpenVPN - VPN(s) running");
    } else {
        gtk_status_icon_set_from_pixbuf(tray_icon, pixbuf_off);
        gtk_status_icon_set_tooltip_text(tray_icon, "OpenVPN - All VPNs off");
    }
}

void cleanup_icons() {
    // Free the pixbufs during program cleanup
    if (pixbuf_on) {
        g_object_unref(pixbuf_on);
    }
    if (pixbuf_off) {
        g_object_unref(pixbuf_off);
    }
}

void fetch_vpn_list(GtkStatusIcon *tray_icon) {
    glob_t glob_result;
    int i;

    if (access(OPENVPN_CONF_DIR, F_OK) != 0) {
        gtk_status_icon_set_tooltip_text(tray_icon, "ERROR: OpenVPN directory does not exist");
        g_print("ERROR: OpenVPN directory does not exist: %s\n", OPENVPN_CONF_DIR);
        return;
    }

    if (chdir(OPENVPN_CONF_DIR) != 0) {
        gtk_status_icon_set_tooltip_text(tray_icon, "ERROR: Unable to change to OpenVPN directory");
        g_print("ERROR: Unable to change to OpenVPN directory: %s\n", OPENVPN_CONF_DIR);
        return;
    }

    char glob_pattern[256];
    snprintf(glob_pattern, sizeof(glob_pattern), "%s*.conf", OPENVPN_CONF_DIR);
    glob(glob_pattern, 0, NULL, &glob_result);

    vpn_count = 0;

    for (i = 0; i < glob_result.gl_pathc && vpn_count < MAX_VPNS; i++) {
        char *filename = strrchr(glob_result.gl_pathv[i], '/') + 1;
        size_t len = strlen(filename) - 5;
        if (len > MAX_VPN_NAME_LEN - 1) {
            len = MAX_VPN_NAME_LEN - 1;
        }

        strncpy(vpn_labels[vpn_count], filename, len);
        vpn_labels[vpn_count][len] = '\0';

        char command[256];
        snprintf(command, sizeof(command), "systemctl is-active openvpn@%s > /dev/null 2>&1", vpn_labels[vpn_count]);

        int return_code = system(command);

        if (WIFEXITED(return_code) && WEXITSTATUS(return_code) == 0) {
            vpn_states[vpn_count] = 1;
            g_print("VPN %s is ON\n", vpn_labels[vpn_count]);
        } else {
            vpn_states[vpn_count] = 0;
            g_print("VPN %s is OFF\n", vpn_labels[vpn_count]);
        }

        vpn_count++;
    }

    globfree(&glob_result);

    update_icon(tray_icon);
}

void turn_on_vpn(const char *vpn_name) {
    char command[256];
    snprintf(command, sizeof(command), "systemctl start openvpn@%s", vpn_name);
    system(command);
    g_print("Turned ON VPN: %s\n", vpn_name);
}

void turn_off_vpn(const char *vpn_name) {
    char command[256];
    snprintf(command, sizeof(command), "systemctl stop openvpn@%s", vpn_name);
    system(command);
    g_print("Turned OFF VPN: %s\n", vpn_name);
}

void turn_on_all_vpns(void) {
    for (int i = 0; i < vpn_count; i++) {
        turn_on_vpn(vpn_labels[i]);
    }
}

void turn_off_all_vpns(void) {
    for (int i = 0; i < vpn_count; i++) {
        turn_off_vpn(vpn_labels[i]);
    }
}

gboolean refresh_vpn_list(gpointer tray_icon) {
    fetch_vpn_list(GTK_STATUS_ICON(tray_icon));
    return TRUE;
}

void on_vpn_toggle(GtkCheckMenuItem *item, gpointer data) {
    int vpn_index = GPOINTER_TO_INT(data);
    gboolean active = gtk_check_menu_item_get_active(item);

    if (active) {
        turn_on_vpn(vpn_labels[vpn_index]);
        vpn_states[vpn_index] = 1;
    } else {
        turn_off_vpn(vpn_labels[vpn_index]);
        vpn_states[vpn_index] = 0;
    }

    update_icon((GtkStatusIcon *)g_object_get_data(G_OBJECT(item), "tray_icon"));

    g_print("VPN %s toggled to %s\n", vpn_labels[vpn_index], active ? "ON" : "OFF");
}

void show_preferences_dialog(GtkStatusIcon *tray_icon) {
    GtkWidget *dialog;
    GtkWidget *content_area;
    GtkWidget *entry;
    GtkWidget *label;
    GtkWidget *grid;

    dialog = gtk_dialog_new_with_buttons("Preferences", NULL, GTK_DIALOG_MODAL,
                                         "_OK", GTK_RESPONSE_OK, "_Cancel", GTK_RESPONSE_CANCEL, NULL);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_add(GTK_CONTAINER(content_area), grid);

    label = gtk_label_new("VPN list update interval (seconds):");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    entry = gtk_entry_new();
    char interval_str[10];
    snprintf(interval_str, sizeof(interval_str), "%d", update_interval);
    gtk_entry_set_text(GTK_ENTRY(entry), interval_str);
    gtk_grid_attach(GTK_GRID(grid), entry, 1, 0, 1, 1);

    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *new_interval = gtk_entry_get_text(GTK_ENTRY(entry));
        update_interval = atoi(new_interval);
        g_print("VPN list update interval updated to: %d seconds\n", update_interval);

        g_timeout_add_seconds(update_interval, refresh_vpn_list, tray_icon);
    }

    gtk_widget_destroy(dialog);
}

GtkWidget* create_vpn_list(GtkStatusIcon *tray_icon) {
    GtkWidget *menu = gtk_menu_new();

    for (int i = 0; i < vpn_count; i++) {
        GtkWidget *vpn_item = gtk_check_menu_item_new_with_label(vpn_labels[i]);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(vpn_item), vpn_states[i]);
        g_signal_connect(vpn_item, "toggled", G_CALLBACK(on_vpn_toggle), GINT_TO_POINTER(i));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), vpn_item);
        g_object_set_data(G_OBJECT(vpn_item), "tray_icon", tray_icon);
    }

    GtkWidget *separator = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);

    GtkWidget *turn_all_on_item = gtk_menu_item_new_with_label("Turn all VPNs on");
    g_signal_connect(turn_all_on_item, "activate", G_CALLBACK(turn_on_all_vpns), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), turn_all_on_item);

    GtkWidget *turn_all_off_item = gtk_menu_item_new_with_label("Turn all VPNs off");
    g_signal_connect(turn_all_off_item, "activate", G_CALLBACK(turn_off_all_vpns), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), turn_all_off_item);

    gtk_widget_show_all(menu);
    
    return menu;
}

void on_tray_icon_left_click(GtkStatusIcon *tray_icon) {
    g_print("Left-click detected\n");
    GtkWidget *menu = create_vpn_list(tray_icon);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
}

void on_tray_icon_right_click(GtkStatusIcon *tray_icon, guint button, guint activate_time) {
    g_print("Right-click detected\n");
    GtkWidget *menu = create_right_click_menu(tray_icon);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
}

GtkWidget* create_right_click_menu(GtkStatusIcon *tray_icon) {
    GtkWidget *menu = gtk_menu_new();

    GtkWidget *reload_item = gtk_menu_item_new_with_label("Reload");
    g_signal_connect(reload_item, "activate", G_CALLBACK(g_print), "Reload clicked\n");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), reload_item);

    GtkWidget *preferences_item = gtk_menu_item_new_with_label("Preferences");
    g_signal_connect(preferences_item, "activate", G_CALLBACK(show_preferences_dialog), tray_icon);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), preferences_item);

    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(quit_item, "activate", G_CALLBACK(gtk_main_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);

    gtk_widget_show_all(menu);

    return menu;
}

int main(int argc, char *argv[]) {
    GtkStatusIcon *tray_icon;

    gtk_init(&argc, &argv);
	    gtk_init(&argc, &argv);

    // Initialize the pixbufs
    load_icons();

    // Create the tray icon and set the initial state
    tray_icon = gtk_status_icon_new();
    update_icon(tray_icon);  // Set initial icon

    gtk_status_icon_set_visible(tray_icon, TRUE);

    fetch_vpn_list(tray_icon);

    g_signal_connect(G_OBJECT(tray_icon), "activate", G_CALLBACK(on_tray_icon_left_click), NULL);
    g_signal_connect(G_OBJECT(tray_icon), "popup-menu", G_CALLBACK(on_tray_icon_right_click), NULL);

    g_timeout_add_seconds(update_interval, refresh_vpn_list, tray_icon);

    gtk_main();

    cleanup_icons();

    return 0;
}

