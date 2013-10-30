#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "pebble_totp.h"
#include "config.h"

#define MY_UUID {0xA4, 0x1B, 0xB0, 0xE2,        \
            0xD2, 0x62, 0x4E, 0xDE,             \
            0xAA, 0xAD, 0xED, 0xBE,             \
            0xEF, 0xE3, 0x8A, 0x02}

PBL_APP_INFO(MY_UUID,
             "SimpleTOTP",
             "Peter Krempa",
             3, 0 /* App version */,
             RESOURCE_ID_IMAGE_MENU_ICON, APP_INFO_WATCH_FACE);


#ifndef COLOR_SCHEME_INVERT
# define COLOR_FG GColorBlack
# define COLOR_BG GColorWhite
#else
# define COLOR_FG GColorWhite
# define COLOR_BG GColorBlack
#endif


Window window;

struct MyText {
    GTextAlignment align;
    GRect rect;
    TextLayer layer;
    enum {
        MY_FONT_SMALL = 0,
        MY_FONT_MIDDLE,
        MY_FONT_LARGE,

        MY_FONT_LAST
    } font;
} text_layers[] = {
    {
        .align = GTextAlignmentLeft,
        .rect = {{0, 0}, {144, 14}},
        .font = MY_FONT_SMALL,
    },
    {
        .align = GTextAlignmentRight,
        .rect = {{0, 0}, {144, 14}},
        .font = MY_FONT_SMALL,
    },
    {
        .align = GTextAlignmentCenter,
        .rect = {{0, 58}, {144, 18}},
        .font = MY_FONT_MIDDLE,
    },
    {
        .align = GTextAlignmentCenter,
        .rect = {{0, 76}, {144, 49}},
        .font = MY_FONT_LARGE,
    },
};

static const short nlayers = sizeof(text_layers) / sizeof(text_layers[0]);

const char *fonts_used[MY_FONT_LAST] = { FONT_KEY_GOTHIC_14,
                                         FONT_KEY_GOTHIC_18_BOLD,
                                         FONT_KEY_ROBOTO_BOLD_SUBSET_49 };

Layer progress_layer;
short progress = 0;

PblTm oldt;
pebble_totp token;


void progress_callback(Layer *me, GContext* ctx)
{
    short i;

    graphics_context_set_stroke_color(ctx, COLOR_FG);

    if (progress)
        graphics_draw_line(ctx, GPoint(0, 16), GPoint(progress * 9, 16));

    /* WOW, a Meter??? */
    for (i = 0; i < 16; i++) {
        short x = i * 3;
        short thr = 2;

        if (i % 15 && i % 5)
            thr--;

        graphics_draw_line(ctx,
                           GPoint(x, 16),
                           GPoint(x, 16 + thr));
    }
}


void
update_watch(PblTm *t, PblTm *oldt, bool force)
{
    // Need to be static because they're used by the system later.
    static char time_text[] = "00:00";
    static char top_date_text[] = "YYYY-MM-DD";
    static char mid_date_text[] = "Day, Mon 00";

    char *time_format;

    /* update date */
    if (force || oldt->tm_mday != t->tm_mday) {
        string_format_time(top_date_text, sizeof(top_date_text), "%F", t);
        text_layer_set_text(&text_layers[1].layer, top_date_text);

        string_format_time(mid_date_text, sizeof(mid_date_text), "%a, %d %b", t);
        text_layer_set_text(&text_layers[2].layer, mid_date_text);
    }

    /* update time */
    if (force ||
        oldt->tm_min != t->tm_min) {

        if (clock_is_24h_style())
            time_format = "%R";
        else
            time_format = "%I:%M";

        string_format_time(time_text, sizeof(time_text), time_format, t);
        text_layer_set_text(&text_layers[3].layer, time_text);
    }
}


void
handle_init(AppContextRef ctx)
{
    GFont fonts[MY_FONT_LAST];
    unsigned char key[] = TOTP_SECRET;
    short i = 0;

    window_init(&window, "SimpleTOTP");
    window_stack_push(&window, false /* Animated */);
    window_set_background_color(&window, COLOR_BG);

    resource_init_current_app(&APP_RESOURCES);


    for (i = 0; i < MY_FONT_LAST; i++)
        fonts[i] = fonts_get_system_font(fonts_used[i]);

    for (i = 0; i < nlayers; i++) {
        struct MyText *t = &text_layers[i];

        text_layer_init(&t->layer, window.layer.frame);
        text_layer_set_text_color(&t->layer, COLOR_FG);
        text_layer_set_background_color(&t->layer, GColorClear);
        text_layer_set_text_alignment(&t->layer, t->align);
        layer_set_frame(&t->layer.layer, t->rect);
        text_layer_set_font(&t->layer, fonts[t->font]);
        layer_add_child(&window.layer, &t->layer.layer);
    }

    layer_init(&progress_layer, window.layer.frame);
    progress_layer.update_proc = &progress_callback;
    layer_add_child(&window.layer, &progress_layer);

    /* update watch right away */
    get_time(&oldt);
    update_watch(&oldt, NULL, true);

    /* and the "progress bar" as well */
    layer_mark_dirty(&progress_layer);

    pebble_totp_init(&token, key, sizeof(key), TOTP_INTERVAL);
    text_layer_set_text(&text_layers[0].layer, pebble_totp_get_code(&token));
}


void
handle_tick(AppContextRef ctx, PebbleTickEvent *t)
{
    short last_progress = progress;

    update_watch(t->tick_time, &oldt, false);
    oldt = *t->tick_time;

    if (pebble_totp_tick(&token, &progress))
        text_layer_set_text(&text_layers[0].layer, pebble_totp_get_code(&token));

    if (last_progress != progress)
        layer_mark_dirty(&progress_layer);
}


void pbl_main(void *params) {
    PebbleAppHandlers handlers = {
        .init_handler = &handle_init,

        .tick_info = {
            .tick_handler = &handle_tick,
            .tick_units = SECOND_UNIT
        }

    };
    app_event_loop(params, &handlers);
}
