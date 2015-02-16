#include "pebble.h"

#include "pebble_totp.h"
#include "config.h"

#define COLOR_BG GColorBlack
#define COLOR_FG GColorWhite


static Window *window;

static TextLayer *text_date_layer;
static TextLayer *text_time_layer;
static TextLayer *text_totp_layer;

static Layer *line_layer;

static GFont *small_font;
static GFont *bold_font;

static pebble_totp token;

static struct tm old_time;


void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, COLOR_FG);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}


void
update_time(struct tm *tick, struct tm *old)
{
    static char time_text[] = "00:00";
    static char date_text[] = "Xxxxxxxxxx 00";

    if (!tick) {
        time_t temp = time(NULL);
        tick = localtime(&temp);
    }

    if (!old || tick->tm_mday != old->tm_mday) {
        strftime(date_text, sizeof(date_text), "%B %e", tick);
        text_layer_set_text(text_date_layer, date_text);
    }

    if (clock_is_24h_style())
        strftime(time_text, sizeof(time_text), "%R", tick);
    else
        strftime(time_text, sizeof(time_text), "%I:%M", tick);

    text_layer_set_text(text_time_layer, time_text);

    old_time = *tick;
}


void
main_window_load(Window *window)
{
    unsigned char key[] = TOTP_SECRET;

    /* load custom fonts */
    small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21));
    bold_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49));

    Layer *window_layer = window_get_root_layer(window);

    /* create date layer */
    text_date_layer = text_layer_create(GRect(8, 68, 144-8, 100));
    text_layer_set_text_color(text_date_layer, COLOR_FG);
    text_layer_set_background_color(text_date_layer, COLOR_BG);
    text_layer_set_font(text_date_layer, small_font);
    layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

    /* create time layer */
    text_time_layer = text_layer_create(GRect(7, 92, 144-7, 70));
    text_layer_set_text_color(text_time_layer, COLOR_FG);
    text_layer_set_background_color(text_time_layer, COLOR_BG);
    text_layer_set_font(text_time_layer, bold_font);
    layer_add_child(window_layer, text_layer_get_layer(text_time_layer));

    /* create totp layer */
    text_totp_layer = text_layer_create(GRect(35, 10, 144-35, 50));
    text_layer_set_text_color(text_totp_layer, COLOR_FG);
    text_layer_set_background_color(text_totp_layer, COLOR_BG);
    text_layer_set_font(text_totp_layer, small_font);
    layer_add_child(window_layer, text_layer_get_layer(text_totp_layer));

    /* init token */
    pebble_totp_init(&token, key, sizeof(key), TOTP_INTERVAL);
    text_layer_set_text(text_totp_layer, pebble_totp_get_code(&token));

    /* create line frame */
    line_layer = layer_create(GRect(8, 97, 139, 2));
    layer_set_update_proc(line_layer, line_layer_update_callback);
    layer_add_child(window_layer, line_layer);
}


void
main_window_unload(Window *window)
{
    text_layer_destroy(text_date_layer);
    text_layer_destroy(text_time_layer);
    text_layer_destroy(text_totp_layer);
    layer_destroy(line_layer);
    fonts_unload_custom_font(small_font);
    fonts_unload_custom_font(bold_font);
}


void
handle_totp(struct tm *tick_time, TimeUnits units_changed)
{
    if (tick_time->tm_min != old_time.tm_min)
        update_time(tick_time, &old_time);

    if (pebble_totp_tick(&token))
        text_layer_set_text(text_totp_layer, pebble_totp_get_code(&token));
}


void
handle_init()
{
    WindowHandlers handlers = {
        .load = main_window_load,
        .unload = main_window_unload
    };

    /* create main window */
    window = window_create();
    window_set_window_handlers(window, handlers);
    window_set_background_color(window, COLOR_BG);
    window_stack_push(window, true /* Animated */);

    /* register timer handlers */
    tick_timer_service_subscribe(SECOND_UNIT, handle_totp);

    update_time(NULL, NULL);
}


void
handle_deinit()
{
    window_destroy(window);
    tick_timer_service_unsubscribe();
}


int
main(void)
{
  handle_init();
  app_event_loop();
  handle_deinit();
}
