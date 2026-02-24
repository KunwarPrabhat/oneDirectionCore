#include <gtk/gtk.h>
#include <csignal>

static GPid overlay_pid = 0;
static GtkWidget *status_label_widget = NULL;
static GtkWidget *fullscreen_rb_global = NULL;
static GtkWidget *pos_dropdown_global = NULL;
static GtkWidget *opacity_scale_global = NULL;
static GtkWidget *sens_scale_global = NULL;
static GtkWidget *preset_dropdown_global = NULL;
static GtkWidget *pollrate_scale_global = NULL;
static GtkWidget *dot_opacity_scale_global = NULL;
static GtkWidget *entity_scale_global = NULL;
static GtkWidget *separation_scale_global = NULL;
static GtkWidget *range_scale_global = NULL;
static GtkWidget *channel_dropdown_global = NULL;
static GtkWidget *hw_port_entry_global = NULL;

static void save_settings() {
    FILE *f = fopen("settings.cfg", "w");
    if (!f) return;
    fprintf(f, "opacity=%d\n", (int)gtk_range_get_value(GTK_RANGE(opacity_scale_global)));
    fprintf(f, "sens=%d\n", (int)gtk_range_get_value(GTK_RANGE(sens_scale_global)));
    fprintf(f, "poll=%d\n", (int)gtk_range_get_value(GTK_RANGE(pollrate_scale_global)));
    fprintf(f, "dot_opacity=%d\n", (int)gtk_range_get_value(GTK_RANGE(dot_opacity_scale_global)));
    fprintf(f, "max_entities=%d\n", (int)gtk_range_get_value(GTK_RANGE(entity_scale_global)));
    fprintf(f, "separation=%d\n", (int)gtk_range_get_value(GTK_RANGE(separation_scale_global)));
    fprintf(f, "range=%d\n", (int)gtk_range_get_value(GTK_RANGE(range_scale_global)));
    fprintf(f, "channels_idx=%d\n", gtk_drop_down_get_selected(GTK_DROP_DOWN(channel_dropdown_global)));
    fprintf(f, "pos=%d\n", gtk_drop_down_get_selected(GTK_DROP_DOWN(pos_dropdown_global)));
    fprintf(f, "preset=%d\n", gtk_drop_down_get_selected(GTK_DROP_DOWN(preset_dropdown_global)));
    fprintf(f, "fullscreen=%d\n", gtk_check_button_get_active(GTK_CHECK_BUTTON(fullscreen_rb_global)));
    fprintf(f, "hw_port=%s\n", gtk_editable_get_text(GTK_EDITABLE(hw_port_entry_global)));
    fclose(f);
}

static void load_settings() {
    FILE *f = fopen("settings.cfg", "r");
    if (!f) return;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        int val;
        if (sscanf(line, "opacity=%d", &val) == 1) gtk_range_set_value(GTK_RANGE(opacity_scale_global), val);
        else if (sscanf(line, "sens=%d", &val) == 1) gtk_range_set_value(GTK_RANGE(sens_scale_global), val);
        else if (sscanf(line, "poll=%d", &val) == 1) gtk_range_set_value(GTK_RANGE(pollrate_scale_global), val);
        else if (sscanf(line, "dot_opacity=%d", &val) == 1) gtk_range_set_value(GTK_RANGE(dot_opacity_scale_global), val);
        else if (sscanf(line, "max_entities=%d", &val) == 1) gtk_range_set_value(GTK_RANGE(entity_scale_global), val);
        else if (sscanf(line, "separation=%d", &val) == 1) gtk_range_set_value(GTK_RANGE(separation_scale_global), val);
        else if (sscanf(line, "range=%d", &val) == 1) gtk_range_set_value(GTK_RANGE(range_scale_global), val);
        else if (sscanf(line, "channels_idx=%d", &val) == 1) gtk_drop_down_set_selected(GTK_DROP_DOWN(channel_dropdown_global), val);
        else if (sscanf(line, "pos=%d", &val) == 1) gtk_drop_down_set_selected(GTK_DROP_DOWN(pos_dropdown_global), val);
        else if (sscanf(line, "preset=%d", &val) == 1) gtk_drop_down_set_selected(GTK_DROP_DOWN(preset_dropdown_global), val);
        else if (sscanf(line, "fullscreen=%d", &val) == 1) gtk_check_button_set_active(GTK_CHECK_BUTTON(fullscreen_rb_global), val);
        else if (strncmp(line, "hw_port=", 8) == 0) {
            char *p = line + 8;
            p[strcspn(p, "\r\n")] = 0;
            gtk_editable_set_text(GTK_EDITABLE(hw_port_entry_global), p);
        }
    }
    fclose(f);
}

static void on_start_clicked(GtkWidget* button, gpointer user_data) {
    (void)button; (void)user_data;

    if (overlay_pid != 0) return;

    
    gboolean is_fs = gtk_check_button_get_active(GTK_CHECK_BUTTON(fullscreen_rb_global));
    guint pos_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(pos_dropdown_global));

    char pos_arg[16];
    snprintf(pos_arg, sizeof(pos_arg), "--pos=%u", pos_idx);

    int opacity_val = (int)gtk_range_get_value(GTK_RANGE(opacity_scale_global));
    char opacity_arg[24];
    snprintf(opacity_arg, sizeof(opacity_arg), "--opacity=%d", opacity_val);

    int sens_val = (int)gtk_range_get_value(GTK_RANGE(sens_scale_global));
    char sens_arg[24];
    snprintf(sens_arg, sizeof(sens_arg), "--sensitivity=%d", sens_val);

    guint preset_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(preset_dropdown_global));
    const char* preset_str = (preset_idx == 1) ? "pubg" : "none";
    char preset_arg[32];
    snprintf(preset_arg, sizeof(preset_arg), "--preset=%s", preset_str);

    int poll_val = (int)gtk_range_get_value(GTK_RANGE(pollrate_scale_global));
    char poll_arg[32];
    snprintf(poll_arg, sizeof(poll_arg), "--pollrate=%d", poll_val);

    int dot_opacity_val = (int)gtk_range_get_value(GTK_RANGE(dot_opacity_scale_global));
    char dot_opacity_arg[32];
    snprintf(dot_opacity_arg, sizeof(dot_opacity_arg), "--dot-opacity=%d", dot_opacity_val);

    int max_ent_val = (int)gtk_range_get_value(GTK_RANGE(entity_scale_global));
    char max_ent_arg[32];
    snprintf(max_ent_arg, sizeof(max_ent_arg), "--max-entities=%d", max_ent_val);

    int sep_val = (int)gtk_range_get_value(GTK_RANGE(separation_scale_global));
    char sep_arg[32];
    snprintf(sep_arg, sizeof(sep_arg), "--separation=%d", sep_val);

    int rg_val = (int)gtk_range_get_value(GTK_RANGE(range_scale_global));
    char rg_arg[32];
    snprintf(rg_arg, sizeof(rg_arg), "--range=%d", rg_val);

    guint ch_idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(channel_dropdown_global));
    int ch_count = 2;
    if (ch_idx == 1) ch_count = 6;      
    else if (ch_idx == 2) ch_count = 8; 
    char ch_arg[32];
    snprintf(ch_arg, sizeof(ch_arg), "--channels=%d", ch_count);

    const char* hw_port_str = gtk_editable_get_text(GTK_EDITABLE(hw_port_entry_global));
    char hw_arg[128] = "";
    if (strlen(hw_port_str) > 0) {
        snprintf(hw_arg, sizeof(hw_arg), "--hw-port=%s", hw_port_str);
    }

    save_settings();

    char *argv_fs[] = {(char*)"./builddir/ODC-overlay", (char*)"--fullscreen", pos_arg, opacity_arg, sens_arg, preset_arg, poll_arg, dot_opacity_arg, max_ent_arg, sep_arg, rg_arg, ch_arg, (hw_arg[0] ? hw_arg : NULL), NULL};
    char *argv_radar[] = {(char*)"./builddir/ODC-overlay", pos_arg, opacity_arg, sens_arg, preset_arg, poll_arg, dot_opacity_arg, max_ent_arg, sep_arg, rg_arg, ch_arg, (hw_arg[0] ? hw_arg : NULL), NULL};
    char **spawn_argv = is_fs ? argv_fs : argv_radar;

    GError *error = NULL;
    if (!g_spawn_async(NULL, spawn_argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &overlay_pid, &error)) {
        
        spawn_argv[0] = (char*)"./ODC-overlay";
        if (!g_spawn_async(NULL, spawn_argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &overlay_pid, NULL)) {
            g_printerr("Failed to launch OSD: %s\n", error->message);
            g_error_free(error);
            return;
        }
    }

    if (status_label_widget)
        gtk_label_set_text(GTK_LABEL(status_label_widget), "Engine: Running");
}

static void on_stop_clicked(GtkWidget* button, gpointer user_data) {
    (void)button; (void)user_data;

    if (overlay_pid != 0) {
        kill(overlay_pid, SIGTERM);
        g_spawn_close_pid(overlay_pid);
        overlay_pid = 0;
    }

    if (status_label_widget)
        gtk_label_set_text(GTK_LABEL(status_label_widget), "Engine: Stopped");
}

static void on_exit_clicked(GtkWidget* button, gpointer user_data) {
    (void)button; (void)user_data;

    if (overlay_pid != 0) {
        kill(overlay_pid, SIGTERM);
        g_spawn_close_pid(overlay_pid);
    }
    exit(0);
}

static GtkWidget* make_section_label(const char* text) {
    GtkWidget *label = gtk_label_new(NULL);
    char *markup = g_markup_printf_escaped("<b>%s</b>", text);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_top(label, 12);
    return label;
}

static GtkWidget* make_description_label(const char* text) {
    GtkWidget *label = gtk_label_new(NULL);
    char *markup = g_markup_printf_escaped("<span size='small' alpha='70%%'>%s</span>", text);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(label, 4);
    return label;
}

static void on_activate(GtkApplication* app, gpointer user_data) {
    (void)user_data;

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "oneDirectionCore");
    gtk_window_set_default_size(GTK_WINDOW(window), 440, 520);

    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_window_set_child(GTK_WINDOW(window), scroll);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(main_box, 24);
    gtk_widget_set_margin_end(main_box, 24);
    gtk_widget_set_margin_top(main_box, 20);
    gtk_widget_set_margin_bottom(main_box, 20);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), main_box);

    
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='x-large' weight='bold'>oneDirectionCore</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(main_box), title);

    GtkWidget *subtitle = gtk_label_new("Spatial Audio Radar · v0.1");
    gtk_widget_set_halign(subtitle, GTK_ALIGN_START);
    gtk_widget_set_margin_bottom(subtitle, 12);
    gtk_box_append(GTK_BOX(main_box), subtitle);

    
    gtk_box_append(GTK_BOX(main_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    
    gtk_box_append(GTK_BOX(main_box), make_section_label("Engine"));

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(main_box), btn_row);

    GtkWidget *start_btn = gtk_button_new_with_label("Start");
    g_signal_connect(start_btn, "clicked", G_CALLBACK(on_start_clicked), NULL);
    gtk_box_append(GTK_BOX(btn_row), start_btn);

    GtkWidget *stop_btn = gtk_button_new_with_label("Stop");
    g_signal_connect(stop_btn, "clicked", G_CALLBACK(on_stop_clicked), NULL);
    gtk_box_append(GTK_BOX(btn_row), stop_btn);

    status_label_widget = gtk_label_new("Engine: Stopped");
    gtk_widget_set_halign(status_label_widget, GTK_ALIGN_START);
    gtk_widget_set_margin_top(status_label_widget, 4);
    gtk_box_append(GTK_BOX(main_box), status_label_widget);

    GtkWidget *poll_lbl = gtk_label_new("Overlay Polling Rate (Hz)");
    gtk_widget_set_halign(poll_lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_top(poll_lbl, 6);
    gtk_box_append(GTK_BOX(main_box), poll_lbl);

    GtkWidget *poll_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 30, 240, 10);
    gtk_range_set_value(GTK_RANGE(poll_scale), 144.0);
    gtk_scale_set_draw_value(GTK_SCALE(poll_scale), TRUE);
    gtk_box_append(GTK_BOX(main_box), poll_scale);
    gtk_box_append(GTK_BOX(main_box), make_description_label("Update rate of the radar. Higher = Smoother but more CPU."));
    pollrate_scale_global = poll_scale;

    
    gtk_box_append(GTK_BOX(main_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    
    gtk_box_append(GTK_BOX(main_box), make_section_label("Audio"));

    GtkWidget *ch_lbl = gtk_label_new("Audio Config (Input)");
    gtk_widget_set_halign(ch_lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_top(ch_lbl, 6);
    gtk_box_append(GTK_BOX(main_box), ch_lbl);

    const char *channels_opts[] = {"Stereo (2.0)", "Surround (5.1)", "Surround (7.1)", NULL};
    channel_dropdown_global = gtk_drop_down_new_from_strings(channels_opts);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(channel_dropdown_global), 2); 
    gtk_box_append(GTK_BOX(main_box), channel_dropdown_global);
    gtk_box_append(GTK_BOX(main_box), make_description_label("Match this to your system's speaker settings."));

    GtkWidget *preset_lbl = gtk_label_new("Sound Classifier Preset");
    gtk_widget_set_halign(preset_lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_top(preset_lbl, 6);
    gtk_box_append(GTK_BOX(main_box), preset_lbl);

    const char *presets[] = {"None (Raw Energy)", "PUBG (Experimental)", NULL};
    preset_dropdown_global = gtk_drop_down_new_from_strings(presets);
    gtk_box_append(GTK_BOX(main_box), preset_dropdown_global);
    gtk_box_append(GTK_BOX(main_box), make_description_label("Pattern matching for specific games like PUBG."));

    GtkWidget *sens_lbl = gtk_label_new("Sensitivity");
    gtk_widget_set_halign(sens_lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_top(sens_lbl, 6);
    gtk_box_append(GTK_BOX(main_box), sens_lbl);

    GtkWidget *sens_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 5);
    gtk_range_set_value(GTK_RANGE(sens_scale), 70.0);
    gtk_scale_set_draw_value(GTK_SCALE(sens_scale), TRUE);
    gtk_box_append(GTK_BOX(main_box), sens_scale);
    gtk_box_append(GTK_BOX(main_box), make_description_label("Detection threshold. Higher = picks up quieter sounds."));
    sens_scale_global = sens_scale;

    GtkWidget *sep_lbl = gtk_label_new("Wavelength Separation");
    gtk_widget_set_halign(sep_lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_top(sep_lbl, 6);
    gtk_box_append(GTK_BOX(main_box), sep_lbl);

    GtkWidget *sep_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 5);
    gtk_range_set_value(GTK_RANGE(sep_scale), 50.0);
    gtk_scale_set_draw_value(GTK_SCALE(sep_scale), TRUE);
    gtk_box_append(GTK_BOX(main_box), sep_scale);
    gtk_box_append(GTK_BOX(main_box), make_description_label("Angular sensitivity. Higher = more distinct dots."));
    separation_scale_global = sep_scale;

    GtkWidget *range_lbl = gtk_label_new("Radar Range (Zoom)");
    gtk_widget_set_halign(range_lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_top(range_lbl, 6);
    gtk_box_append(GTK_BOX(main_box), range_lbl);

    GtkWidget *range_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 5);
    gtk_range_set_value(GTK_RANGE(range_scale), 100.0);
    gtk_scale_set_draw_value(GTK_SCALE(range_scale), TRUE);
    gtk_box_append(GTK_BOX(main_box), range_scale);
    gtk_box_append(GTK_BOX(main_box), make_description_label("Scales blip distance from center. Higher = Zoom out."));
    range_scale_global = range_scale;

    
    gtk_box_append(GTK_BOX(main_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    
    gtk_box_append(GTK_BOX(main_box), make_section_label("Display"));

    GtkWidget *radar_rb = gtk_check_button_new_with_label("Gaming Radar (Mini-Map)");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(radar_rb), TRUE);
    gtk_box_append(GTK_BOX(main_box), radar_rb);

    GtkWidget *fullscreen_rb = gtk_check_button_new_with_label("Full Screen Overlay");
    gtk_check_button_set_group(GTK_CHECK_BUTTON(fullscreen_rb), GTK_CHECK_BUTTON(radar_rb));
    gtk_box_append(GTK_BOX(main_box), fullscreen_rb);
    fullscreen_rb_global = fullscreen_rb;

    GtkWidget *pos_lbl = gtk_label_new("Radar Position");
    gtk_widget_set_halign(pos_lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_top(pos_lbl, 6);
    gtk_box_append(GTK_BOX(main_box), pos_lbl);

    const char *positions[] = {"Top-Left", "Top-Center", "Top-Right", "Bottom-Left", "Bottom-Center", "Bottom-Right", NULL};
    GtkWidget *pos_dropdown = gtk_drop_down_new_from_strings(positions);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(pos_dropdown), 2); 
    gtk_box_append(GTK_BOX(main_box), pos_dropdown);
    pos_dropdown_global = pos_dropdown;

    GtkWidget *opacity_lbl = gtk_label_new("OSD Opacity");
    gtk_widget_set_halign(opacity_lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_top(opacity_lbl, 6);
    gtk_box_append(GTK_BOX(main_box), opacity_lbl);

    GtkWidget *opacity_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 5);
    gtk_range_set_value(GTK_RANGE(opacity_scale), 80.0);
    gtk_scale_set_draw_value(GTK_SCALE(opacity_scale), TRUE);
    gtk_box_append(GTK_BOX(main_box), opacity_scale);
    gtk_box_append(GTK_BOX(main_box), make_description_label("Transparency of the radar background and rings."));
    opacity_scale_global = opacity_scale;

    GtkWidget *dot_opacity_lbl = gtk_label_new("Entity Opacity (Dots)");
    gtk_widget_set_halign(dot_opacity_lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_top(dot_opacity_lbl, 6);
    gtk_box_append(GTK_BOX(main_box), dot_opacity_lbl);

    GtkWidget *dot_opacity_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 10, 100, 5);
    gtk_range_set_value(GTK_RANGE(dot_opacity_scale), 100.0);
    gtk_scale_set_draw_value(GTK_SCALE(dot_opacity_scale), TRUE);
    gtk_box_append(GTK_BOX(main_box), dot_opacity_scale);
    gtk_box_append(GTK_BOX(main_box), make_description_label("Transparency of the blip icons on the radar."));
    dot_opacity_scale_global = dot_opacity_scale;

    GtkWidget *entity_lbl = gtk_label_new("Max Entities (1-10)");
    gtk_widget_set_halign(entity_lbl, GTK_ALIGN_START);
    gtk_widget_set_margin_top(entity_lbl, 6);
    gtk_box_append(GTK_BOX(main_box), entity_lbl);

    GtkWidget *entity_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 10, 1);
    gtk_range_set_value(GTK_RANGE(entity_scale), 6.0);
    gtk_scale_set_draw_value(GTK_SCALE(entity_scale), TRUE);
    gtk_box_append(GTK_BOX(main_box), entity_scale);
    gtk_box_append(GTK_BOX(main_box), make_description_label("How many distinct sounds can be shown at once."));
    entity_scale_global = entity_scale;

    
    gtk_box_append(GTK_BOX(main_box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    
    GtkWidget *exit_btn = gtk_button_new_with_label("Exit App");
    gtk_widget_set_halign(exit_btn, GTK_ALIGN_END);
    gtk_widget_set_margin_top(exit_btn, 8);
    g_signal_connect(exit_btn, "clicked", G_CALLBACK(on_exit_clicked), NULL);
    gtk_box_append(GTK_BOX(main_box), exit_btn);

    gtk_window_present(GTK_WINDOW(window));
    load_settings();
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("com.onedirection.core", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
