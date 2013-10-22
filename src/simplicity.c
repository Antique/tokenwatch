#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "otp.h"
#include "sha1.h"

#define MY_UUID {0xA4, 0x1B, 0xB0, 0xE2, 0xD2, 0x62, 0x4E, 0xDE, 0xAA, 0xAD, 0xED, 0xBE, 0xEF, 0xE3, 0x8A, 0x02}
PBL_APP_INFO(MY_UUID, "Simplicity - TOTP", "Peter Krempa", 3, 0 /* App version */, RESOURCE_ID_IMAGE_MENU_ICON, APP_INFO_WATCH_FACE);

Window window;

TextLayer text_date_layer;
TextLayer text_time_layer;
TextLayer test;

Layer line_layer;

PblTm oldt;

void line_layer_update_callback(Layer *me, GContext* ctx) {

  graphics_context_set_stroke_color(ctx, GColorBlack);

  graphics_draw_line(ctx, GPoint(8, 97), GPoint(131, 97));
  graphics_draw_line(ctx, GPoint(8, 98), GPoint(131, 98));

}

//#define otp_value(asdgf,...) 0
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
    window_init(&window, "SimpleTOTP");
    window_stack_push(&window, false /* Animated */);
    window_set_background_color(&window, GColorWhite);

    resource_init_current_app(&APP_RESOURCES);

    text_layer_init(&text_date_layer, window.layer.frame);
    text_layer_set_text_color(&text_date_layer, GColorBlack);
    text_layer_set_background_color(&text_date_layer, GColorClear);
    layer_set_frame(&text_date_layer.layer, GRect(8, 68, 144-8, 168-68));
    text_layer_set_font(&text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
    layer_add_child(&window.layer, &text_date_layer.layer);

    text_layer_init(&text_time_layer, window.layer.frame);
    text_layer_set_text_color(&text_time_layer, GColorBlack);
    text_layer_set_background_color(&text_time_layer, GColorClear);
    layer_set_frame(&text_time_layer.layer, GRect(7, 92, 144-7, 168-92));
    text_layer_set_font(&text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
    layer_add_child(&window.layer, &text_time_layer.layer);

    text_layer_init(&test, window.layer.frame);
    text_layer_set_text_color(&text_time_layer, GColorBlack);
    text_layer_set_background_color(&test, GColorClear);
    layer_set_frame(&test.layer, GRect(8, 10, 144-8, 168-10));
    text_layer_set_font(&test, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
    layer_add_child(&window.layer, &test.layer);

    layer_init(&line_layer, window.layer.frame);
    line_layer.update_proc = &line_layer_update_callback;
    layer_add_child(&window.layer, &line_layer);

    /* update watch right away */
    get_time(&oldt);
    update_watch(&oldt, NULL, true);


    text_layer_set_text(&test, "123456");
}

/* pebble SDK doesn't treat unix time as it should, compensate for it */
static const uint64_t leap_second_offset = 8;
static const uint64_t timezone_offset = 0;

static bool
is_dst_in_CET(PblTm *t)
{
    /* DST SUCKS!@!@!!! */
    /* DST in CET is from
     * last sunday in March, 00:03:00, to
     * last sunday in October, 00:02:00, but this check will slip 1 hour */
    //TODO
#if 0
    if ((t->tm_mon >= 2 && t->tm_mon <= 9) &&


       )
        ble;
#endif
    return true;

    //return false;
}
static uint32_t
get_unix_time(PblTm *t)
{
    time_t pebble_time = time(NULL);

    /* compensate leap seconds */
    pebble_time += leap_second_offset;

    /* compensate DST and timezone */
    pebble_time -= timezone_offset;

    if (is_dst_in_CET(t))
        pebble_time -= 3600;

   return pebble_time;
}


void
handle_tick(AppContextRef ctx, PebbleTickEvent *t)
{
    static char token[20];
    unsigned char key[] = "\xe8\x50\xc0\x16\x72\x56\x1e\xd8\x1b\xd8\x59\x88\x4d\xdc\x62\xdf\x8c\x83\x42\x31";
    uint32_t unixtime;

    update_watch(t->tick_time, &oldt, false);
    oldt = *t->tick_time;
    unixtime = get_unix_time(t->tick_time);
    unixtime /= 60;

    snprintf(token, 20, "%.6u", otp_value(key, 20,  unixtime));
    text_layer_set_text(&test, token);
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