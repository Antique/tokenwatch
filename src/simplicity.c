#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "pebble_totp.h"

#define MY_UUID {0xA4, 0x1B, 0xB0, 0xE2, \
                 0xD2, 0x62, 0x4E, 0xDE, \
                 0xAA, 0xAD, 0xED, 0xBE, \
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

TextLayer text_date_layer;
TextLayer text_time_layer;
TextLayer text_totp_layer;

Layer line_layer;

PblTm oldt;
pebble_totp token;

void line_layer_update_callback(Layer *me, GContext* ctx) {

  graphics_context_set_stroke_color(ctx, COLOR_FG);

  graphics_draw_line(ctx, GPoint(8, 97), GPoint(131, 97));
  graphics_draw_line(ctx, GPoint(8, 98), GPoint(131, 98));
}


void
update_watch(PblTm *t, PblTm *oldt, bool force)
{
    // Need to be static because they're used by the system later.
    static char time_text[] = "00:00";
    static char date_text[] = "Xxxxxxxxx 00";

    char *time_format;

    /* update date */
    if (force ||
        oldt->tm_mday != t->tm_mday) {
        string_format_time(date_text, sizeof(date_text), "%B %e", t);
        text_layer_set_text(&text_date_layer, date_text);
    }

    /* update time */
    if (force ||
        oldt->tm_min != t->tm_min) {

        if (clock_is_24h_style())
            time_format = "%R";
        else
            time_format = "%I:%M";

        string_format_time(time_text, sizeof(time_text), time_format, t);
        text_layer_set_text(&text_time_layer, time_text);
    }
}


void
handle_init(AppContextRef ctx)
{
    unsigned char key[] = "\xe8\x50\xc0\x16\x72\x56\x1e\xd8\x1b\xd8\x59\x88\x4d\xdc\x62\xdf\x8c\x83\x42\x31";

    window_init(&window, "SimpleTOTP");
    window_stack_push(&window, false /* Animated */);
    window_set_background_color(&window, COLOR_BG);

    resource_init_current_app(&APP_RESOURCES);

    text_layer_init(&text_date_layer, window.layer.frame);
    text_layer_set_text_color(&text_date_layer, COLOR_FG);
    text_layer_set_background_color(&text_date_layer, GColorClear);
    layer_set_frame(&text_date_layer.layer, GRect(8, 68, 144-8, 168-68));
    text_layer_set_font(&text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
    layer_add_child(&window.layer, &text_date_layer.layer);

    text_layer_init(&text_time_layer, window.layer.frame);
    text_layer_set_text_color(&text_time_layer, COLOR_FG);
    text_layer_set_background_color(&text_time_layer, GColorClear);
    layer_set_frame(&text_time_layer.layer, GRect(7, 92, 144-7, 168-92));
    text_layer_set_font(&text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
    layer_add_child(&window.layer, &text_time_layer.layer);

    text_layer_init(&text_totp_layer, window.layer.frame);
    text_layer_set_text_color(&text_totp_layer, COLOR_FG);
    text_layer_set_background_color(&text_totp_layer, GColorClear);
    layer_set_frame(&text_totp_layer.layer, GRect(20, 10, 144-20, 168-10));
    text_layer_set_font(&text_totp_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
    layer_add_child(&window.layer, &text_totp_layer.layer);

    layer_init(&line_layer, window.layer.frame);
    line_layer.update_proc = &line_layer_update_callback;
    layer_add_child(&window.layer, &line_layer);

    /* update watch right away */
    get_time(&oldt);
    update_watch(&oldt, NULL, true);

    pebble_totp_init(&token, key, sizeof(key), 60);
    text_layer_set_text(&text_totp_layer, pebble_totp_get_code(&token));
}


void
handle_tick(AppContextRef ctx, PebbleTickEvent *t)
{
    update_watch(t->tick_time, &oldt, false);
    oldt = *t->tick_time;

    if (pebble_totp_tick(&token))
        text_layer_set_text(&text_totp_layer, pebble_totp_get_code(&token));
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
