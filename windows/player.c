#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#define ALBUM_ART_SIZE 280
#define APP_NAME "Reproductor App"
#define DEFAULT_WIDTH 980
#define DEFAULT_HEIGHT 700

static const gchar *audio_extensions[] = {
    ".mp3", ".flac", ".wav", ".ogg", ".m4a", ".opus", ".wma", ".aac", NULL
};

static const gchar *theme_names[] = {
    "Azul Oscuro", "Purpura Oscuro", "Verde Oscuro", "Rojo Oscuro", "Carbon", NULL
};

typedef struct {
    gchar *filepath;
    gchar *title;
    gchar *artist;
    gchar *album;
    gdouble duration;
    GdkPixbuf *album_art;
    gboolean metadata_loaded;
} Song;

typedef struct {
    gdouble timestamp;
    gchar *text;
} LyricLine;

typedef struct Player {
    GList *songs;
    gint current_index;

    GstElement *pipeline;
    guint bus_watch_id;
    gboolean is_playing;
    gboolean is_paused;
    gboolean seeking;
    guint tick_id;
    gulong seek_handler_id;

    GtkWidget *window;
    GtkWidget *open_btn;
    GtkWidget *theme_combo;

    GtkWidget *album_art_image;
    GtkWidget *title_label;
    GtkWidget *artist_label;
    GtkWidget *album_label;
    GtkWidget *time_label;

    GtkWidget *seek_scale;
    GtkWidget *volume_scale;
    GtkWidget *vol_percent_label;

    GtkWidget *play_pause_btn;
    GtkWidget *stop_btn;
    GtkWidget *prev_btn;
    GtkWidget *next_btn;

    GtkWidget *playlist_box;

    GtkCssProvider *css_provider;

    GList *folders;

    GtkWidget *lyrics_btn;
    GtkWidget *lyrics_window;
    GtkWidget *lyrics_label;
    GList *lyrics_lines;
    gint lyrics_current;

    gdouble volume;
    gint current_theme;
} Player;

static const gchar *theme_css[] = {
    "window, .background { background: #0f0f23; }\n"
    "label { color: #e0e0e0; }\n"
    "label#time-label { color: #a0a0b0; font-size: 12px; }\n"
    "label#info-label { color: #a0a0b0; font-size: 11px; }\n"
    "label#title-label { color: #ffffff; font-size: 16px; font-weight: bold; }\n"
    "button { background: #1a1a3e; color: #e0e0e0; border: 1px solid #2a2a5e; border-radius: 4px; padding: 6px 12px; }\n"
    "button:hover { background: #2a2a5e; }\n"
    "button#control-btn { padding: 8px 14px; font-size: 16px; min-width: 40px; }\n"
    "button#menu-btn { padding: 6px 12px; font-size: 14px; }\n"
    "scale trough { background: #2a2a5e; min-height: 6px; border-radius: 3px; }\n"
    "scale highlight { background: #4a90d9; }\n"
    "scale slider { background: #6ab0ff; border: 1px solid #4a90d9; border-radius: 10px; min-width: 16px; min-height: 16px; }\n"
    "list { background: #0f0f23; color: #e0e0e0; }\n"
    "list row { padding: 8px 12px; border-bottom: 1px solid #1a1a3e; }\n"
    "list row:selected { background: #1a3a6e; }\n"
    "list row:hover { background: #1a1a3e; }\n"
    "scrolledwindow { border: 1px solid #2a2a5e; border-radius: 4px; }\n"
    "headerbar { background: #0a0a1e; color: #e0e0e0; border: none; }\n"
    "box#art-frame { background: #1a1a3e; border-radius: 8px; padding: 4px; }\n",

    "window, .background { background: #1a0a2e; }\n"
    "label { color: #e0d0f0; }\n"
    "label#time-label { color: #a090b0; font-size: 12px; }\n"
    "label#info-label { color: #a090b0; font-size: 11px; }\n"
    "label#title-label { color: #ffffff; font-size: 16px; font-weight: bold; }\n"
    "button { background: #2d1b4e; color: #e0d0f0; border: 1px solid #4a2a7e; border-radius: 4px; padding: 6px 12px; }\n"
    "button:hover { background: #3d2b5e; }\n"
    "button#control-btn { padding: 8px 14px; font-size: 16px; min-width: 40px; }\n"
    "button#menu-btn { padding: 6px 12px; font-size: 14px; }\n"
    "scale trough { background: #3d2b5e; min-height: 6px; border-radius: 3px; }\n"
    "scale highlight { background: #8a5cce; }\n"
    "scale slider { background: #a070e0; border: 1px solid #8a5cce; border-radius: 10px; min-width: 16px; min-height: 16px; }\n"
    "list { background: #1a0a2e; color: #e0d0f0; }\n"
    "list row { padding: 8px 12px; border-bottom: 1px solid #2d1b4e; }\n"
    "list row:selected { background: #3d2b5e; }\n"
    "list row:hover { background: #2d1b4e; }\n"
    "scrolledwindow { border: 1px solid #4a2a7e; border-radius: 4px; }\n"
    "headerbar { background: #12081e; color: #e0d0f0; border: none; }\n"
    "box#art-frame { background: #2d1b4e; border-radius: 8px; padding: 4px; }\n",

    "window, .background { background: #0a1a0a; }\n"
    "label { color: #d0e8d0; }\n"
    "label#time-label { color: #90a890; font-size: 12px; }\n"
    "label#info-label { color: #90a890; font-size: 11px; }\n"
    "label#title-label { color: #ffffff; font-size: 16px; font-weight: bold; }\n"
    "button { background: #0d2b0d; color: #d0e8d0; border: 1px solid #1a4a1a; border-radius: 4px; padding: 6px 12px; }\n"
    "button:hover { background: #1a3a1a; }\n"
    "button#control-btn { padding: 8px 14px; font-size: 16px; min-width: 40px; }\n"
    "button#menu-btn { padding: 6px 12px; font-size: 14px; }\n"
    "scale trough { background: #1a4a1a; min-height: 6px; border-radius: 3px; }\n"
    "scale highlight { background: #3aaa3a; }\n"
    "scale slider { background: #50cc50; border: 1px solid #3aaa3a; border-radius: 10px; min-width: 16px; min-height: 16px; }\n"
    "list { background: #0a1a0a; color: #d0e8d0; }\n"
    "list row { padding: 8px 12px; border-bottom: 1px solid #0d2b0d; }\n"
    "list row:selected { background: #1a4a1a; }\n"
    "list row:hover { background: #0d2b0d; }\n"
    "scrolledwindow { border: 1px solid #1a4a1a; border-radius: 4px; }\n"
    "headerbar { background: #061206; color: #d0e8d0; border: none; }\n"
    "box#art-frame { background: #0d2b0d; border-radius: 8px; padding: 4px; }\n",

    "window, .background { background: #1a0a0a; }\n"
    "label { color: #f0d0d0; }\n"
    "label#time-label { color: #b09090; font-size: 12px; }\n"
    "label#info-label { color: #b09090; font-size: 11px; }\n"
    "label#title-label { color: #ffffff; font-size: 16px; font-weight: bold; }\n"
    "button { background: #2b0d0d; color: #f0d0d0; border: 1px solid #4a1a1a; border-radius: 4px; padding: 6px 12px; }\n"
    "button:hover { background: #3a1a1a; }\n"
    "button#control-btn { padding: 8px 14px; font-size: 16px; min-width: 40px; }\n"
    "button#menu-btn { padding: 6px 12px; font-size: 14px; }\n"
    "scale trough { background: #4a1a1a; min-height: 6px; border-radius: 3px; }\n"
    "scale highlight { background: #cc4040; }\n"
    "scale slider { background: #e06060; border: 1px solid #cc4040; border-radius: 10px; min-width: 16px; min-height: 16px; }\n"
    "list { background: #1a0a0a; color: #f0d0d0; }\n"
    "list row { padding: 8px 12px; border-bottom: 1px solid #2b0d0d; }\n"
    "list row:selected { background: #4a1a1a; }\n"
    "list row:hover { background: #2b0d0d; }\n"
    "scrolledwindow { border: 1px solid #4a1a1a; border-radius: 4px; }\n"
    "headerbar { background: #120606; color: #f0d0d0; border: none; }\n"
    "box#art-frame { background: #2b0d0d; border-radius: 8px; padding: 4px; }\n",

    "window, .background { background: #1a1a1a; }\n"
    "label { color: #d0d0d0; }\n"
    "label#time-label { color: #909090; font-size: 12px; }\n"
    "label#info-label { color: #909090; font-size: 11px; }\n"
    "label#title-label { color: #ffffff; font-size: 16px; font-weight: bold; }\n"
    "button { background: #2d2d2d; color: #d0d0d0; border: 1px solid #404040; border-radius: 4px; padding: 6px 12px; }\n"
    "button:hover { background: #3a3a3a; }\n"
    "button#control-btn { padding: 8px 14px; font-size: 16px; min-width: 40px; }\n"
    "button#menu-btn { padding: 6px 12px; font-size: 14px; }\n"
    "scale trough { background: #404040; min-height: 6px; border-radius: 3px; }\n"
    "scale highlight { background: #8090a0; }\n"
    "scale slider { background: #a0b0c0; border: 1px solid #8090a0; border-radius: 10px; min-width: 16px; min-height: 16px; }\n"
    "list { background: #1a1a1a; color: #d0d0d0; }\n"
    "list row { padding: 8px 12px; border-bottom: 1px solid #2d2d2d; }\n"
    "list row:selected { background: #404040; }\n"
    "list row:hover { background: #2d2d2d; }\n"
    "scrolledwindow { border: 1px solid #404040; border-radius: 4px; }\n"
    "headerbar { background: #121212; color: #d0d0d0; border: none; }\n"
    "box#art-frame { background: #2d2d2d; border-radius: 8px; padding: 4px; }\n"
};

static gboolean is_audio_file(const gchar *filename) {
    gchar *lower = g_utf8_strdown(filename, -1);
    for (gint i = 0; audio_extensions[i]; i++) {
        if (g_str_has_suffix(lower, audio_extensions[i])) {
            g_free(lower);
            return TRUE;
        }
    }
    g_free(lower);
    return FALSE;
}

static Song *song_new(const gchar *filepath) {
    Song *s = g_new0(Song, 1);
    s->filepath = g_strdup(filepath);
    gchar *basename = g_path_get_basename(filepath);
    s->title = g_strdup(basename);
    s->artist = g_strdup("");
    s->album = g_strdup("");
    s->duration = 0.0;
    s->album_art = NULL;
    s->metadata_loaded = FALSE;
    g_free(basename);
    return s;
}

static void song_free(Song *s) {
    if (!s) return;
    g_free(s->filepath);
    g_free(s->title);
    g_free(s->artist);
    g_free(s->album);
    if (s->album_art) g_object_unref(s->album_art);
    g_free(s);
}

static GList *scan_directory(const gchar *dirpath) {
    GList *files = NULL;
    GDir *dir = g_dir_open(dirpath, 0, NULL);
    if (!dir) return NULL;

    const gchar *name;
    while ((name = g_dir_read_name(dir)) != NULL) {
        gchar *fullpath = g_build_filename(dirpath, name, NULL);
        if (g_file_test(fullpath, G_FILE_TEST_IS_DIR)) {
            GList *sub = scan_directory(fullpath);
            files = g_list_concat(files, sub);
        } else if (is_audio_file(name)) {
            files = g_list_append(files, g_strdup(fullpath));
        }
        g_free(fullpath);
    }
    g_dir_close(dir);
    return files;
}

static void apply_theme(Player *player, gint index);
static void player_load_metadata(Player *player, Song *song);
static void player_load_song(Player *player, gint index);
static void load_lyrics(Player *player, const gchar *audio_path);

static gchar *get_config_file_path(void) {
    return g_strdup("player-theme.txt");
}

static void save_config(gint theme_index, gdouble volume, GList *folders) {
    gchar *path = get_config_file_path();
    GString *buf = g_string_new(NULL);
    g_string_append_printf(buf, "theme=%d\n", theme_index);
    g_string_append_printf(buf, "volume=%.2f\n", volume);
    for (GList *l = folders; l; l = l->next) {
        gchar *folder = (gchar *)l->data;
        g_string_append_printf(buf, "folder=%s\n", folder);
    }
    g_file_set_contents(path, buf->str, -1, NULL);
    g_string_free(buf, TRUE);
    g_free(path);
}

static gint load_config(GList **folders_out, gdouble *volume_out) {
    gchar *path = get_config_file_path();
    gchar *content = NULL;
    gint theme_index = 0;
    if (folders_out) *folders_out = NULL;
    if (volume_out) *volume_out = 0.8;

    if (g_file_get_contents(path, &content, NULL, NULL) && content) {
        gchar **lines = g_strsplit(content, "\n", -1);
        if (lines && lines[0]) {
            if (strchr(lines[0], '=') == NULL) {
                theme_index = atoi(lines[0]);
                if (theme_index < 0 || theme_index >= 5) theme_index = 0;
                if (folders_out) {
                    for (gint i = 1; lines[i]; i++) {
                        gchar *trimmed = g_strstrip(lines[i]);
                        if (strlen(trimmed) > 0) {
                            *folders_out = g_list_append(*folders_out, g_strdup(trimmed));
                        }
                    }
                }
            } else {
                for (gint i = 0; lines[i]; i++) {
                    gchar *line = g_strstrip(lines[i]);
                    if (strlen(line) == 0 || line[0] == '#') continue;
                    if (g_str_has_prefix(line, "theme=")) {
                        theme_index = atoi(line + 6);
                        if (theme_index < 0 || theme_index >= 5) theme_index = 0;
                    } else if (g_str_has_prefix(line, "volume=")) {
                        if (volume_out) {
                            *volume_out = CLAMP(atof(line + 7), 0.01, 1.0);
                        }
                    } else if (g_str_has_prefix(line, "folder=")) {
                        if (folders_out) {
                            *folders_out = g_list_append(*folders_out, g_strdup(line + 7));
                        }
                    }
                }
            }
        }
        g_strfreev(lines);
        g_free(content);
    }
    g_free(path);
    return theme_index;
}

static GdkPixbuf *create_placeholder_art(void) {
    GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                                        ALBUM_ART_SIZE, ALBUM_ART_SIZE);
    if (!pixbuf) return NULL;

    gint rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    gint width = gdk_pixbuf_get_width(pixbuf);
    gint height = gdk_pixbuf_get_height(pixbuf);

    for (gint y = 0; y < height; y++) {
        guint8 *row = gdk_pixbuf_get_pixels(pixbuf) + y * rowstride;
        for (gint x = 0; x < width; x++) {
            guint8 *p = row + x * 4;
            gdouble dx = (gdouble)x / width - 0.5;
            gdouble dy = (gdouble)y / height - 0.5;
            gdouble dist = sqrt(dx * dx + dy * dy);
            if (dist < 0.4) {
                p[0] = 80 + (guint8)(50 * (1.0 - dist / 0.4));
                p[1] = 80 + (guint8)(50 * (1.0 - dist / 0.4));
                p[2] = 120 + (guint8)(80 * (1.0 - dist / 0.4));
                p[3] = 255;
            } else if (dist < 0.45) {
                p[0] = 120; p[1] = 120; p[2] = 160; p[3] = 255;
            } else {
                p[0] = 40; p[1] = 40; p[2] = 60; p[3] = 255;
            }
        }
    }
    return pixbuf;
}

static GdkPixbuf *sample_to_pixbuf(GstSample *sample) {
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if (!buffer) return NULL;

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) return NULL;

    GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
    GdkPixbuf *pixbuf = NULL;

    GError *err = NULL;
    if (gdk_pixbuf_loader_write(loader, map.data, map.size, &err)) {
        gdk_pixbuf_loader_close(loader, NULL);
        GdkPixbuf *loaded = gdk_pixbuf_loader_get_pixbuf(loader);
        if (loaded) {
            pixbuf = gdk_pixbuf_scale_simple(loaded,
                ALBUM_ART_SIZE, ALBUM_ART_SIZE, GDK_INTERP_BILINEAR);
        }
    } else {
        g_warning("Failed to decode album art: %s", err->message);
        g_clear_error(&err);
        gdk_pixbuf_loader_close(loader, NULL);
    }

    gst_buffer_unmap(buffer, &map);
    g_object_unref(loader);
    return pixbuf;
}

static void player_load_metadata(Player *player, Song *song) {
    if (!song || song->metadata_loaded) return;

    gchar *uri = g_filename_to_uri(song->filepath, NULL, NULL);
    if (!uri) return;

    GError *err = NULL;
    GstDiscoverer *discoverer = gst_discoverer_new(3 * GST_SECOND, &err);
    if (!discoverer) {
        if (err) g_warning("Failed to create discoverer: %s", err->message);
        g_clear_error(&err);
        g_free(uri);
        return;
    }

    GstDiscovererInfo *info = gst_discoverer_discover_uri(discoverer, uri, &err);
    if (err) {
        g_warning("Discovery error for %s: %s", song->filepath, err->message);
        g_clear_error(&err);
        g_object_unref(discoverer);
        g_free(uri);
        return;
    }

    if (!info) {
        g_object_unref(discoverer);
        g_free(uri);
        return;
    }

    GstDiscovererResult result = gst_discoverer_info_get_result(info);
    if (result == GST_DISCOVERER_OK) {
        GstTagList *tags = (GstTagList *)gst_discoverer_info_get_tags(info);
        if (tags) {
            gchar *tag_str = NULL;

            if (gst_tag_list_get_string(tags, GST_TAG_TITLE, &tag_str) && tag_str) {
                g_free(song->title);
                song->title = tag_str;
                tag_str = NULL;
            }

            if (gst_tag_list_get_string(tags, GST_TAG_ARTIST, &tag_str) && tag_str) {
                g_free(song->artist);
                song->artist = tag_str;
                tag_str = NULL;
            }

            if (gst_tag_list_get_string(tags, GST_TAG_ALBUM, &tag_str) && tag_str) {
                g_free(song->album);
                song->album = tag_str;
                tag_str = NULL;
            }

            GstSample *sample = NULL;
            if (gst_tag_list_get_sample(tags, GST_TAG_IMAGE, &sample)) {
                if (song->album_art) g_object_unref(song->album_art);
                song->album_art = sample_to_pixbuf(sample);
                gst_sample_unref(sample);
            } else if (gst_tag_list_get_sample(tags, GST_TAG_PREVIEW_IMAGE, &sample)) {
                if (song->album_art) g_object_unref(song->album_art);
                song->album_art = sample_to_pixbuf(sample);
                gst_sample_unref(sample);
            }

        }

        song->duration = (gdouble)gst_discoverer_info_get_duration(info) / GST_SECOND;
    } else if (result == GST_DISCOVERER_TIMEOUT) {
        g_warning("Discovery timed out for: %s", song->filepath);
    }

    gst_discoverer_info_unref(info);
    g_object_unref(discoverer);
    g_free(uri);

    song->metadata_loaded = TRUE;
}

static GList *parse_lrc(const gchar *filepath) {
    gchar *content = NULL;
    if (!g_file_get_contents(filepath, &content, NULL, NULL)) return NULL;

    GList *lines = NULL;
    gchar **raw = g_strsplit(content, "\n", -1);
    for (gint i = 0; raw[i]; i++) {
        gchar *line = g_strstrip(raw[i]);
        if (strlen(line) < 4 || line[0] != '[') continue;
        gchar *close = strchr(line + 1, ']');
        if (!close) continue;

        gint tag_len = (gint)(close - line - 1);
        if (tag_len < 4) continue;

        gchar time_str[32];
        g_strlcpy(time_str, line + 1, tag_len + 1);

        gdouble sec = 0.0;
        gint m = 0, s = 0, ms = 0;
        if (sscanf(time_str, "%d:%d.%d", &m, &s, &ms) >= 2) {
            gdouble div = (ms >= 100) ? 1000.0 : 100.0;
            sec = m * 60.0 + s + ms / div;
        } else if (sscanf(time_str, "%d:%d,%d", &m, &s, &ms) >= 2) {
            gdouble div = (ms >= 100) ? 1000.0 : 100.0;
            sec = m * 60.0 + s + ms / div;
        } else if (sscanf(time_str, "%d:%d", &m, &s) >= 2) {
            sec = m * 60.0 + s;
        } else {
            continue;
        }

        gchar *text = g_strstrip(close + 1);
        if (strlen(text) == 0) continue;

        LyricLine *ll = g_new0(LyricLine, 1);
        ll->timestamp = sec;
        ll->text = g_strdup(text);
        lines = g_list_append(lines, ll);
    }
    g_strfreev(raw);
    g_free(content);
    return lines;
}

static void free_lyrics_lines(GList *lines) {
    for (GList *l = lines; l; l = l->next) {
        LyricLine *ll = (LyricLine *)l->data;
        g_free(ll->text);
        g_free(ll);
    }
    g_list_free(lines);
}

static gchar *build_lyrics_markup(GList *lines, gint current) {
    GString *buf = g_string_new(NULL);
    gint idx = 0;
    for (GList *l = lines; l; l = l->next) {
        LyricLine *ll = (LyricLine *)l->data;
        if (idx == current) {
            g_string_append_printf(buf, "<b><big>%s</big></b>\n", ll->text);
        } else {
            g_string_append_printf(buf, "%s\n", ll->text);
        }
        idx++;
    }
    return g_string_free(buf, FALSE);
}

static void load_lyrics(Player *player, const gchar *audio_path) {
    if (player->lyrics_lines) {
        free_lyrics_lines(player->lyrics_lines);
        player->lyrics_lines = NULL;
    }
    player->lyrics_current = -1;

    if (!audio_path) {
        if (player->lyrics_label) {
            gtk_label_set_markup(GTK_LABEL(player->lyrics_label), "Sin letra disponible");
        }
        return;
    }

    gchar *base = g_strdup(audio_path);
    gchar *dot = strrchr(base, '.');
    if (dot) *dot = '\0';
    gchar *lrc_path = g_strconcat(base, ".lrc", NULL);

    player->lyrics_lines = parse_lrc(lrc_path);
    if (player->lyrics_lines) {
        gchar *markup = build_lyrics_markup(player->lyrics_lines, -1);
        if (player->lyrics_label) {
            gtk_label_set_markup(GTK_LABEL(player->lyrics_label), markup);
        }
        g_free(markup);
    } else {
        if (player->lyrics_label) {
            gtk_label_set_markup(GTK_LABEL(player->lyrics_label), "Sin letra disponible");
        }
    }

    g_free(lrc_path);
    g_free(base);
}

static void update_lyrics(Player *player) {
    if (!player->lyrics_lines || !player->lyrics_window ||
        !gtk_widget_get_visible(player->lyrics_window)) return;

    gint64 pos = 0;
    GstFormat fmt = GST_FORMAT_TIME;
    if (!gst_element_query_position(player->pipeline, fmt, &pos)) return;
    gdouble sec = (gdouble)pos / GST_SECOND;

    gint new_idx = -1;
    gint idx = 0;
    for (GList *l = player->lyrics_lines; l; l = l->next) {
        LyricLine *ll = (LyricLine *)l->data;
        if (sec >= ll->timestamp) new_idx = idx;
        idx++;
    }

    if (new_idx != player->lyrics_current) {
        player->lyrics_current = new_idx;
        gchar *markup = build_lyrics_markup(player->lyrics_lines, new_idx);
        gtk_label_set_markup(GTK_LABEL(player->lyrics_label), markup);
        g_free(markup);
    }
}

static void player_update_playlist_row(Player *player, gint index) {
    GtkListBoxRow *row = gtk_list_box_get_row_at_index(
        GTK_LIST_BOX(player->playlist_box), index);
    if (!row) return;

    Song *song = (Song *)g_list_nth_data(player->songs, index);
    if (!song) return;

    GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
    if (!label) return;

    gchar *display;
    if (song->metadata_loaded && strlen(song->artist) > 0) {
        display = g_strdup_printf("%s  -  %s", song->artist, song->title);
    } else {
        gchar *base = g_path_get_basename(song->filepath);
        display = g_strdup(base);
        g_free(base);
    }

    gtk_label_set_text(GTK_LABEL(label), display);
    g_free(display);
}

static gboolean update_position(gpointer data) {
    Player *player = (Player *)data;

    if (!player->pipeline || !player->is_playing) {
        return G_SOURCE_CONTINUE;
    }

    gint64 pos = 0, dur = 0;
    GstFormat fmt = GST_FORMAT_TIME;

    if (!gst_element_query_position(player->pipeline, fmt, &pos)) {
        return G_SOURCE_CONTINUE;
    }
    if (!gst_element_query_duration(player->pipeline, fmt, &dur)) {
        return G_SOURCE_CONTINUE;
    }

    if (dur > 0) {
        gdouble fraction = CLAMP((gdouble)pos / dur, 0.0, 1.0);

        if (!player->seeking) {
            g_signal_handler_block(player->seek_scale, player->seek_handler_id);
            gtk_range_set_value(GTK_RANGE(player->seek_scale), fraction);
            g_signal_handler_unblock(player->seek_scale, player->seek_handler_id);
        }

        gint pos_sec = (gint)(pos / GST_SECOND);
        gint dur_sec = (gint)(dur / GST_SECOND);
        gchar time_str[64];
        g_snprintf(time_str, sizeof(time_str), "%d:%02d / %d:%02d",
                   pos_sec / 60, pos_sec % 60, dur_sec / 60, dur_sec % 60);
        gtk_label_set_text(GTK_LABEL(player->time_label), time_str);
    }

    update_lyrics(player);

    return G_SOURCE_CONTINUE;
}

static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data) {
    Player *player = (Player *)data;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR: {
        GError *err = NULL;
        gchar *debug = NULL;
        gst_message_parse_error(msg, &err, &debug);
        g_warning("GStreamer error: %s", err->message);

        GtkWidget *err_dialog = gtk_message_dialog_new(
            player->window ? GTK_WINDOW(player->window) : NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
            "Error de reproduccion:\n%s", err->message);
        g_signal_connect(err_dialog, "response",
                         G_CALLBACK(gtk_widget_destroy), NULL);
        gtk_window_present(GTK_WINDOW(err_dialog));

        g_clear_error(&err);
        g_free(debug);

        if (player->is_playing) {
            gst_element_set_state(player->pipeline, GST_STATE_NULL);
            player->is_playing = FALSE;
            player->is_paused = FALSE;
            gtk_button_set_label(GTK_BUTTON(player->play_pause_btn), "\u25b6");
        }
        break;
    }
    case GST_MESSAGE_EOS: {
        player->is_playing = FALSE;
        player->is_paused = FALSE;
        gtk_button_set_label(GTK_BUTTON(player->play_pause_btn), "\u25b6");
        gtk_range_set_value(GTK_RANGE(player->seek_scale), 0.0);

        gint total = (gint)g_list_length(player->songs);
        if (player->songs && player->current_index < total - 1) {
            player->current_index++;
            player_load_song(player, player->current_index);
            player_update_playlist_row(player, player->current_index);
        }
        break;
    }
    case GST_MESSAGE_TAG: {
        GstTagList *tags = NULL;
        gst_message_parse_tag(msg, &tags);
        if (tags) {
            if (player->current_index >= 0) {
                Song *song = (Song *)g_list_nth_data(player->songs, player->current_index);
                if (song) {
                    GstSample *sample = NULL;
                    if (gst_tag_list_get_sample(tags, GST_TAG_IMAGE, &sample)) {
                        if (song->album_art) g_object_unref(song->album_art);
                        song->album_art = sample_to_pixbuf(sample);
                        gst_sample_unref(sample);
                        if (song->album_art) {
                            gtk_image_set_from_pixbuf(
                                GTK_IMAGE(player->album_art_image), song->album_art);
                        }
                    } else if (gst_tag_list_get_sample(tags, GST_TAG_PREVIEW_IMAGE, &sample)) {
                        if (song->album_art) g_object_unref(song->album_art);
                        song->album_art = sample_to_pixbuf(sample);
                        gst_sample_unref(sample);
                        if (song->album_art) {
                            gtk_image_set_from_pixbuf(
                                GTK_IMAGE(player->album_art_image), song->album_art);
                        }
                    }
                }
            }
            gst_tag_list_unref(tags);
        }
        break;
    }
    default:
        break;
    }

    return G_SOURCE_CONTINUE;
}

static void player_load_song(Player *player, gint index) {
    if (!player->songs || index < 0 || index >= (gint)g_list_length(player->songs))
        return;

    Song *song = (Song *)g_list_nth_data(player->songs, index);
    if (!song) return;

    player->current_index = index;

    gchar *uri = g_filename_to_uri(song->filepath, NULL, NULL);
    if (!uri) return;

    gst_element_set_state(player->pipeline, GST_STATE_NULL);

    g_object_set(G_OBJECT(player->pipeline), "uri", uri, NULL);
    g_free(uri);

    player_load_metadata(player, song);
    load_lyrics(player, song->filepath);

    if (song->album_art) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(player->album_art_image), song->album_art);
    } else {
        GdkPixbuf *placeholder = create_placeholder_art();
        gtk_image_set_from_pixbuf(GTK_IMAGE(player->album_art_image), placeholder);
        if (placeholder) g_object_unref(placeholder);
    }

    gchar *display_title = g_strdup(song->title);
    gchar *display_artist = (strlen(song->artist) > 0) ? g_strdup(song->artist) : g_strdup("Artista desconocido");
    gchar *display_album = (strlen(song->album) > 0) ? g_strdup(song->album) : g_strdup("Album desconocido");

    gtk_label_set_text(GTK_LABEL(player->title_label), display_title);
    gtk_label_set_text(GTK_LABEL(player->artist_label), display_artist);
    gtk_label_set_text(GTK_LABEL(player->album_label), display_album);

    g_free(display_title);
    g_free(display_artist);
    g_free(display_album);

    gtk_range_set_value(GTK_RANGE(player->seek_scale), 0.0);
    gtk_label_set_text(GTK_LABEL(player->time_label), "0:00 / 0:00");

    gst_element_set_state(player->pipeline, GST_STATE_PLAYING);
    player->is_playing = TRUE;
    player->is_paused = FALSE;
    gtk_button_set_label(GTK_BUTTON(player->play_pause_btn), "\u23f8");
}

static void player_play_pause(Player *player) {
    if (!player->songs || g_list_length(player->songs) == 0) return;

    if (!player->is_playing && !player->is_paused) {
        if (player->current_index >= 0 &&
            player->current_index < (gint)g_list_length(player->songs)) {
            player_load_song(player, player->current_index);
        } else if (player->songs) {
            player->current_index = 0;
            player_load_song(player, 0);
        }
        return;
    }

    if (player->is_playing) {
        gst_element_set_state(player->pipeline, GST_STATE_PAUSED);
        player->is_playing = FALSE;
        player->is_paused = TRUE;
        gtk_button_set_label(GTK_BUTTON(player->play_pause_btn), "\u25b6");
    } else if (player->is_paused) {
        gst_element_set_state(player->pipeline, GST_STATE_PLAYING);
        player->is_playing = TRUE;
        player->is_paused = FALSE;
        gtk_button_set_label(GTK_BUTTON(player->play_pause_btn), "\u23f8");
    }
}

static void player_stop(Player *player) {
    if (!player->pipeline) return;

    gst_element_set_state(player->pipeline, GST_STATE_NULL);
    player->is_playing = FALSE;
    player->is_paused = FALSE;
    gtk_button_set_label(GTK_BUTTON(player->play_pause_btn), "\u25b6");
    gtk_range_set_value(GTK_RANGE(player->seek_scale), 0.0);
    gtk_label_set_text(GTK_LABEL(player->time_label), "0:00 / 0:00");
}

static void player_next(Player *player) {
    if (!player->songs) return;
    gint len = g_list_length(player->songs);
    if (len == 0) return;

    gint next = player->current_index + 1;
    if (next >= len) next = 0;

    player->current_index = next;
    player_load_song(player, next);
    player_update_playlist_row(player, next);
}

static void player_prev(Player *player) {
    if (!player->songs) return;
    gint len = g_list_length(player->songs);
    if (len == 0) return;

    gint prev = player->current_index - 1;
    if (prev < 0) prev = len - 1;

    player->current_index = prev;
    player_load_song(player, prev);
    player_update_playlist_row(player, prev);
}

static void player_seek(Player *player, gdouble fraction) {
    if (!player->pipeline || (!player->is_playing && !player->is_paused)) return;

    gint64 dur = 0;
    GstFormat fmt = GST_FORMAT_TIME;
    if (!gst_element_query_duration(player->pipeline, fmt, &dur) || dur <= 0)
        return;

    gint64 seek_pos = (gint64)(fraction * dur);
    gst_element_seek_simple(player->pipeline, GST_FORMAT_TIME,
                            GST_SEEK_FLAG_FLUSH, seek_pos);
}

static void player_refresh_playlist(Player *player) {
    GtkListBox *list = GTK_LIST_BOX(player->playlist_box);

    GList *children = gtk_container_get_children(GTK_CONTAINER(list));
    for (GList *c = children; c; c = c->next) {
        gtk_widget_destroy(GTK_WIDGET(c->data));
    }
    g_list_free(children);

    for (GList *l = player->songs; l; l = l->next) {
        Song *song = (Song *)l->data;
        gchar *base = g_path_get_basename(song->filepath);
        GtkWidget *label = gtk_label_new(base);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        GtkWidget *row = gtk_list_box_row_new();
        gtk_container_add(GTK_CONTAINER(row), label);
        gtk_list_box_insert(list, row, -1);
        gtk_widget_show_all(row);
        g_free(base);
    }
}

static gboolean is_folder_tracked(Player *player, const gchar *dirpath) {
    for (GList *l = player->folders; l; l = l->next) {
        if (g_strcmp0((gchar *)l->data, dirpath) == 0) return TRUE;
    }
    return FALSE;
}

static void on_open_folder(GtkButton *btn, Player *player) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Seleccionar carpeta de musica", GTK_WINDOW(player->window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "_Cancelar", GTK_RESPONSE_CANCEL,
        "_Abrir", GTK_RESPONSE_ACCEPT, NULL);

    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList *new_folders = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        for (GSList *f = new_folders; f; f = f->next) {
            gchar *dirpath = (gchar *)f->data;

            if (!is_folder_tracked(player, dirpath)) {
                player->folders = g_list_append(player->folders, g_strdup(dirpath));
            }

            GList *new_files = scan_directory(dirpath);
            for (GList *nf = new_files; nf; nf = nf->next) {
                gchar *fp = (gchar *)nf->data;
                gboolean found = FALSE;
                for (GList *existing = player->songs; existing; existing = existing->next) {
                    Song *es = (Song *)existing->data;
                    if (g_strcmp0(es->filepath, fp) == 0) {
                        found = TRUE;
                        break;
                    }
                }
                if (!found) {
                    Song *s = song_new(fp);
                    player->songs = g_list_append(player->songs, s);
                }
                g_free(fp);
            }
            g_list_free(new_files);
            g_free(dirpath);
        }
        g_slist_free(new_folders);

        player_refresh_playlist(player);

        if (player->songs && player->current_index < 0) {
            player->current_index = 0;
        }

        save_config(player->current_theme, player->volume, player->folders);
    }

    gtk_widget_destroy(dialog);
}

static void on_row_activated(GtkListBox *box, GtkListBoxRow *row, Player *player) {
    gint index = gtk_list_box_row_get_index(row);
    if (index < 0 || index >= (gint)g_list_length(player->songs)) return;

    if (index == player->current_index && player->is_playing) {
        return;
    }

    player->current_index = index;
    player_load_song(player, index);
    player_update_playlist_row(player, index);
}

static void on_play_pause(GtkButton *btn, Player *player) {
    player_play_pause(player);
}

static void on_stop(GtkButton *btn, Player *player) {
    player_stop(player);
}

static void on_next(GtkButton *btn, Player *player) {
    player_next(player);
}

static void on_prev(GtkButton *btn, Player *player) {
    player_prev(player);
}

static void on_seek_changed(GtkRange *range, Player *player) {
    if (player->seeking) {
        gdouble fraction = gtk_range_get_value(range);
        player_seek(player, fraction);
    }
}

static gboolean on_seek_pressed(GtkWidget *widget, GdkEventButton *event,
                                 Player *player) {
    player->seeking = TRUE;
    return FALSE;
}

static gboolean on_seek_released(GtkWidget *widget, GdkEventButton *event,
                                  Player *player) {
    if (player->seeking) {
        player->seeking = FALSE;
        gdouble fraction = gtk_range_get_value(GTK_RANGE(widget));
        player_seek(player, fraction);
    }
    return FALSE;
}

static void on_volume_changed(GtkRange *range, Player *player) {
    player->volume = gtk_range_get_value(range);
    if (player->pipeline) {
        gdouble actual = pow(player->volume, 2.0);
        g_object_set(G_OBJECT(player->pipeline), "volume", actual, NULL);
    }
    gint pct = (gint)(player->volume * 100.0 + 0.5);
    gchar *text = g_strdup_printf("%d%%", pct);
    gtk_label_set_text(GTK_LABEL(player->vol_percent_label), text);
    g_free(text);
    save_config(player->current_theme, player->volume, player->folders);
}

static void on_theme_changed(GtkComboBox *combo, Player *player) {
    gint index = gtk_combo_box_get_active(combo);
    if (index >= 0 && index < 5) {
        player->current_theme = index;
        apply_theme(player, index);
        save_config(index, player->volume, player->folders);
    }
}

static void on_lyrics_destroy(GtkWidget *widget, Player *player) {
    (void)widget;
    player->lyrics_window = NULL;
    player->lyrics_label = NULL;
}

static void on_lyrics_clicked(GtkButton *btn, Player *player) {
    if (!player->lyrics_window) {
        player->lyrics_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(player->lyrics_window), "Letras");
        gtk_window_set_transient_for(GTK_WINDOW(player->lyrics_window),
                                     GTK_WINDOW(player->window));
        gtk_window_set_default_size(GTK_WINDOW(player->lyrics_window), 400, 500);
        gtk_window_set_position(GTK_WINDOW(player->lyrics_window),
                                GTK_WIN_POS_CENTER_ON_PARENT);
        g_signal_connect(player->lyrics_window, "destroy",
                         G_CALLBACK(on_lyrics_destroy), player);

        GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
            GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(player->lyrics_window), sw);

        player->lyrics_label = gtk_label_new("Sin letra disponible");
        gtk_label_set_xalign(GTK_LABEL(player->lyrics_label), 0.0);
        gtk_label_set_yalign(GTK_LABEL(player->lyrics_label), 0.0);
        gtk_widget_set_halign(player->lyrics_label, GTK_ALIGN_START);
        gtk_widget_set_valign(player->lyrics_label, GTK_ALIGN_START);
        gtk_label_set_line_wrap(GTK_LABEL(player->lyrics_label), TRUE);
        gtk_container_add(GTK_CONTAINER(sw), player->lyrics_label);

        if (player->lyrics_lines) {
            gchar *markup = build_lyrics_markup(player->lyrics_lines, player->lyrics_current);
            gtk_label_set_markup(GTK_LABEL(player->lyrics_label), markup);
            g_free(markup);
        }
    }

    if (gtk_widget_get_visible(player->lyrics_window)) {
        gtk_widget_hide(player->lyrics_window);
    } else {
        gtk_widget_show_all(player->lyrics_window);
    }
}

static gboolean on_window_delete(GtkWidget *widget, GdkEvent *event,
                                  gpointer data) {
    Player *player = (Player *)data;

    if (player->tick_id > 0)
        g_source_remove(player->tick_id);
    player->tick_id = 0;

    if (player->bus_watch_id > 0)
        g_source_remove(player->bus_watch_id);
    player->bus_watch_id = 0;

    if (player->pipeline) {
        gst_element_set_state(player->pipeline, GST_STATE_NULL);
        g_object_unref(player->pipeline);
        player->pipeline = NULL;
    }

    if (player->lyrics_lines) {
        free_lyrics_lines(player->lyrics_lines);
        player->lyrics_lines = NULL;
    }

    if (player->lyrics_window) {
        gtk_widget_destroy(player->lyrics_window);
        player->lyrics_window = NULL;
    }

    for (GList *l = player->songs; l; l = l->next) {
        song_free((Song *)l->data);
    }
    g_list_free(player->songs);
    player->songs = NULL;

    for (GList *l = player->folders; l; l = l->next) {
        g_free(l->data);
    }
    g_list_free(player->folders);
    player->folders = NULL;

    if (player->css_provider) {
        g_object_unref(player->css_provider);
    }

    gtk_main_quit();
    return FALSE;
}

static void apply_theme(Player *player, gint index) {
    if (index < 0 || index >= 5) return;

    GdkScreen *screen = gdk_screen_get_default();
    if (!screen) return;

    if (player->css_provider) {
        gtk_style_context_remove_provider_for_screen(screen,
            GTK_STYLE_PROVIDER(player->css_provider));
        g_object_unref(player->css_provider);
        player->css_provider = NULL;
    }

    player->css_provider = gtk_css_provider_new();
    GError *err = NULL;
    gtk_css_provider_load_from_data(player->css_provider, theme_css[index], -1, &err);

    if (err) {
        g_warning("CSS error: %s", err->message);
        g_clear_error(&err);
    }

    gtk_style_context_add_provider_for_screen(screen,
        GTK_STYLE_PROVIDER(player->css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

static void on_github_clicked(GtkWidget *btn, gpointer data) {
    (void)btn;
    (void)data;
#ifdef G_OS_WIN32
    ShellExecuteW(NULL, L"open", L"https://github.com/nestoree/reproductor_mp3",
                  NULL, NULL, SW_SHOWNORMAL);
#else
    GError *err = NULL;
    if (!g_app_info_launch_default_for_uri(
            "https://github.com/nestoree/reproductor_mp3", NULL, &err)) {
        g_printerr("Error al abrir URL: %s\n", err->message);
        g_error_free(err);
    }
#endif
}

static const char github_svg[] =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" "
    "viewBox=\"0 0 16 16\">"
    "<path fill=\"#e0e0e0\" d=\"M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17."
    "55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-."
    "82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2."
    "33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-"
    "2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27s1."
    "36.09 2 .27c1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1."
    "27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.9"
    "3-.01 2.2 0 .21.15.46.55.38A8.01 8.01 0 0 0 16 8c0-4.42-3.58-8-8-8\"/>"
    "</svg>";

static GtkWidget *make_github_button(void) {
    GdkPixbuf *pixbuf = NULL;
    GBytes *bytes = g_bytes_new_static(github_svg, sizeof(github_svg) - 1);
    GInputStream *stream = g_memory_input_stream_new_from_bytes(bytes);
    GError *err = NULL;
    pixbuf = gdk_pixbuf_new_from_stream_at_scale(stream, 16, 16, TRUE, NULL, &err);
    if (err) {
        g_printerr("Error loading SVG: %s\n", err->message);
        g_error_free(err);
    }
    g_object_unref(stream);
    g_bytes_unref(bytes);

    GtkWidget *btn = gtk_button_new();
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    if (pixbuf) {
        GtkWidget *image = gtk_image_new_from_pixbuf(pixbuf);
        gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
        g_object_unref(pixbuf);
    }

    GtkWidget *label = gtk_label_new("GitHub");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(btn), hbox);
    return btn;
}

static void build_ui(Player *player) {
    player->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(player->window), APP_NAME);
    gtk_window_set_default_size(GTK_WINDOW(player->window), DEFAULT_WIDTH, DEFAULT_HEIGHT);
    gtk_window_set_position(GTK_WINDOW(player->window), GTK_WIN_POS_CENTER);
    g_signal_connect(player->window, "delete-event", G_CALLBACK(on_window_delete), player);

    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), APP_NAME);

    player->open_btn = gtk_button_new_with_label("\U0001f4c1 Abrir Carpeta");
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), player->open_btn);
    g_signal_connect(player->open_btn, "clicked", G_CALLBACK(on_open_folder), player);

    player->theme_combo = gtk_combo_box_text_new();
    for (gint i = 0; theme_names[i]; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(player->theme_combo),
                                       theme_names[i]);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(player->theme_combo), player->current_theme);
    g_signal_connect(player->theme_combo, "changed", G_CALLBACK(on_theme_changed), player);
    player->lyrics_btn = gtk_button_new_with_label("\U0001f4dc Letras");
    g_signal_connect(player->lyrics_btn, "clicked", G_CALLBACK(on_lyrics_clicked), player);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), player->lyrics_btn);

    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), player->theme_combo);

    gtk_window_set_titlebar(GTK_WINDOW(player->window), header);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(player->window), main_box);

    GtkWidget *content_area = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_container_add(GTK_CONTAINER(main_box), content_area);

    GtkWidget *margin_left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(margin_left, 12, -1);
    gtk_box_pack_start(GTK_BOX(content_area), margin_left, FALSE, FALSE, 0);

    GtkWidget *left_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_box_pack_start(GTK_BOX(content_area), left_panel, TRUE, TRUE, 0);

    GtkWidget *art_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(art_frame, "art-frame");
    gtk_widget_set_size_request(art_frame, ALBUM_ART_SIZE + 8, ALBUM_ART_SIZE + 8);

    player->album_art_image = gtk_image_new();
    GdkPixbuf *placeholder = create_placeholder_art();
    if (placeholder) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(player->album_art_image), placeholder);
        g_object_unref(placeholder);
    }
    gtk_box_pack_start(GTK_BOX(art_frame), player->album_art_image, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(left_panel), art_frame, FALSE, FALSE, 0);

    player->title_label = gtk_label_new("Titulo");
    gtk_widget_set_name(player->title_label, "title-label");
    gtk_label_set_ellipsize(GTK_LABEL(player->title_label), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start(GTK_BOX(left_panel), player->title_label, FALSE, FALSE, 2);

    player->artist_label = gtk_label_new("Artista");
    gtk_widget_set_name(player->artist_label, "info-label");
    gtk_label_set_ellipsize(GTK_LABEL(player->artist_label), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start(GTK_BOX(left_panel), player->artist_label, FALSE, FALSE, 2);

    player->album_label = gtk_label_new("Album");
    gtk_widget_set_name(player->album_label, "info-label");
    gtk_label_set_ellipsize(GTK_LABEL(player->album_label), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start(GTK_BOX(left_panel), player->album_label, FALSE, FALSE, 2);

    GtkWidget *vspacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(left_panel), vspacer, TRUE, TRUE, 0);

    player->seek_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 1.0, 0.01);
    gtk_scale_set_draw_value(GTK_SCALE(player->seek_scale), FALSE);
    gtk_widget_set_size_request(player->seek_scale, -1, 20);

    player->seek_handler_id = g_signal_connect(player->seek_scale, "value-changed",
        G_CALLBACK(on_seek_changed), player);
    g_signal_connect(player->seek_scale, "button-press-event",
        G_CALLBACK(on_seek_pressed), player);
    g_signal_connect(player->seek_scale, "button-release-event",
        G_CALLBACK(on_seek_released), player);

    gtk_box_pack_start(GTK_BOX(left_panel), player->seek_scale, FALSE, FALSE, 0);

    player->time_label = gtk_label_new("0:00 / 0:00");
    gtk_widget_set_name(player->time_label, "time-label");
    gtk_box_pack_start(GTK_BOX(left_panel), player->time_label, FALSE, FALSE, 2);

    GtkWidget *controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_set_center_widget(GTK_BOX(controls), NULL);

    player->prev_btn = gtk_button_new_with_label("\u23ee");
    gtk_widget_set_name(player->prev_btn, "control-btn");
    g_signal_connect(player->prev_btn, "clicked", G_CALLBACK(on_prev), player);

    player->play_pause_btn = gtk_button_new_with_label("\u25b6");
    gtk_widget_set_name(player->play_pause_btn, "control-btn");
    g_signal_connect(player->play_pause_btn, "clicked", G_CALLBACK(on_play_pause), player);

    player->stop_btn = gtk_button_new_with_label("\u23f9");
    gtk_widget_set_name(player->stop_btn, "control-btn");
    g_signal_connect(player->stop_btn, "clicked", G_CALLBACK(on_stop), player);

    player->next_btn = gtk_button_new_with_label("\u23ed");
    gtk_widget_set_name(player->next_btn, "control-btn");
    g_signal_connect(player->next_btn, "clicked", G_CALLBACK(on_next), player);

    gtk_box_pack_start(GTK_BOX(controls), player->prev_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls), player->play_pause_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls), player->stop_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls), player->next_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_panel), controls, FALSE, FALSE, 4);

    GtkWidget *vol_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *vol_label = gtk_label_new("Vol:");
    gtk_box_pack_start(GTK_BOX(vol_box), vol_label, FALSE, FALSE, 0);

    player->volume_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.01, 1.0, 0.01);
    gtk_scale_set_draw_value(GTK_SCALE(player->volume_scale), FALSE);
    gtk_range_set_value(GTK_RANGE(player->volume_scale), player->volume);
    gtk_widget_set_size_request(player->volume_scale, 130, 20);
    g_signal_connect(player->volume_scale, "value-changed",
        G_CALLBACK(on_volume_changed), player);

    gtk_box_pack_start(GTK_BOX(vol_box), player->volume_scale, TRUE, TRUE, 0);

    gint vol_pct = (gint)(player->volume * 100.0 + 0.5);
    gchar *vol_text = g_strdup_printf("%d%%", vol_pct);
    player->vol_percent_label = gtk_label_new(vol_text);
    g_free(vol_text);
    gtk_widget_set_size_request(player->vol_percent_label, 50, -1);
    gtk_label_set_xalign(GTK_LABEL(player->vol_percent_label), 1.0);
    gtk_box_pack_start(GTK_BOX(vol_box), player->vol_percent_label, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(left_panel), vol_box, FALSE, FALSE, 0);

    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(content_area), separator, FALSE, FALSE, 0);

    GtkWidget *right_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(content_area), right_panel, TRUE, TRUE, 0);

    player->playlist_box = gtk_list_box_new();
    gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(player->playlist_box), TRUE);
    g_signal_connect(player->playlist_box, "row-activated",
        G_CALLBACK(on_row_activated), player);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), player->playlist_box);
    gtk_box_pack_start(GTK_BOX(right_panel), scroll, TRUE, TRUE, 0);

    GtkWidget *github_btn = make_github_button();
    gtk_widget_set_name(github_btn, "menu-btn");
    g_signal_connect(github_btn, "clicked", G_CALLBACK(on_github_clicked), player->window);
    gtk_box_pack_end(GTK_BOX(right_panel), github_btn, FALSE, FALSE, 4);

    GtkWidget *margin_right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(margin_right, 12, -1);
    gtk_box_pack_start(GTK_BOX(content_area), margin_right, FALSE, FALSE, 0);

    apply_theme(player, player->current_theme);

    gtk_widget_show_all(player->window);
}

#ifdef G_OS_WIN32
static char *g_win32_exe_dir = NULL;

static void setup_windows_paths(void) {
    if (g_getenv("GST_PLUGIN_PATH")) return;

    wchar_t wbuf[MAX_PATH];
    GetModuleFileNameW(NULL, wbuf, MAX_PATH);
    char *utf8 = g_utf16_to_utf8(wbuf, -1, NULL, NULL, NULL);
    if (!utf8) return;

    g_win32_exe_dir = g_path_get_dirname(utf8);
    g_free(utf8);
    if (!g_win32_exe_dir) return;

    gchar *tmp;

    tmp = g_build_filename(g_win32_exe_dir, "lib", "gstreamer-1.0", NULL);
    g_setenv("GST_PLUGIN_PATH", tmp, FALSE);
    g_free(tmp);

    tmp = g_build_filename(g_win32_exe_dir, "share", "glib-2.0", "schemas", NULL);
    g_setenv("GSETTINGS_SCHEMA_DIR", tmp, FALSE);
    g_free(tmp);

    gchar *pixbuf_base = g_build_filename(g_win32_exe_dir, "lib", "gdk-pixbuf-2.0", NULL);
    GDir *pdir = g_dir_open(pixbuf_base, 0, NULL);
    if (pdir) {
        const char *ver = g_dir_read_name(pdir);
        if (ver) {
            tmp = g_build_filename(pixbuf_base, ver, "loaders.cache", NULL);
            g_setenv("GDK_PIXBUF_MODULE_FILE", tmp, FALSE);
            g_free(tmp);
        }
        g_dir_close(pdir);
    }
    g_free(pixbuf_base);

    const char *old_path = g_getenv("PATH");
    tmp = g_strdup_printf("%s;%s", g_win32_exe_dir, old_path ? old_path : "");
    g_setenv("PATH", tmp, TRUE);
    g_free(tmp);
}
#endif

int main(int argc, char *argv[]) {
#ifdef G_OS_WIN32
    setup_windows_paths();
#endif
    gst_init(&argc, &argv);

#ifdef G_OS_WIN32
    {
        const char *gst_path = g_getenv("GST_PLUGIN_PATH");
        if (gst_path) {
            GstRegistry *reg = gst_registry_get();
            if (reg) gst_registry_scan_path(reg, gst_path);
        }
    }
#endif

    if (!gtk_init_check(&argc, &argv)) {
        g_printerr("Failed to initialize GTK\n");
        return 1;
    }

    Player *player = g_new0(Player, 1);
    player->current_index = -1;
    player->volume = 0.8;
    player->seeking = FALSE;
    player->is_playing = FALSE;
    player->is_paused = FALSE;
    player->pipeline = gst_element_factory_make("playbin", "player");
    if (!player->pipeline) {
        g_printerr("Failed to create GStreamer pipeline\n");
        g_free(player);
        return 1;
    }

    player->current_theme = load_config(&player->folders, &player->volume);

    gdouble actual_vol = pow(player->volume, 2.0);
    g_object_set(G_OBJECT(player->pipeline), "volume", actual_vol, NULL);

    GstBus *bus = gst_element_get_bus(player->pipeline);
    player->bus_watch_id = gst_bus_add_watch(bus, bus_callback, player);
    gst_object_unref(bus);

    build_ui(player);

    for (GList *l = player->folders; l; l = l->next) {
        gchar *dirpath = (gchar *)l->data;
        GList *new_files = scan_directory(dirpath);
        for (GList *nf = new_files; nf; nf = nf->next) {
            gchar *fp = (gchar *)nf->data;
            gboolean found = FALSE;
            for (GList *existing = player->songs; existing; existing = existing->next) {
                Song *es = (Song *)existing->data;
                if (g_strcmp0(es->filepath, fp) == 0) {
                    found = TRUE;
                    break;
                }
            }
            if (!found) {
                Song *s = song_new(fp);
                player->songs = g_list_append(player->songs, s);
            }
            g_free(fp);
        }
        g_list_free(new_files);
    }

    player_refresh_playlist(player);

    if (player->songs && player->current_index < 0) {
        player->current_index = 0;
    }

    player->tick_id = g_timeout_add(250, update_position, player);

    gtk_main();

    g_free(player);
    return 0;
}
