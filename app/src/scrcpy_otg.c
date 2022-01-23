#include <scrcpy_otg.h>

#include <SDL2/SDL.h>

#include "events.h"
#include "screen_otg.h"
#include "util/log.h"

struct scrcpy_otg {
    struct sc_aoa aoa;
    struct sc_hid_keyboard keyboard;
    struct sc_hid_mouse mouse;

    struct sc_screen_otg screen_otg;
};

static void
sc_aoa_on_disconnected(struct sc_aoa *aoa, void *userdata) {
    (void) aoa;
    (void) userdata;

    SDL_Event event;
    event.type = EVENT_USB_DEVICE_DISCONNECTED;
    int ret = SDL_PushEvent(&event);
    if (ret < 0) {
        LOGE("Could not post USB disconnection event: %s", SDL_GetError());
    }
}

static bool
event_loop(struct scrcpy_otg *s) {
    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        switch (event.type) {
            case EVENT_USB_DEVICE_DISCONNECTED:
                LOGW("Device disconnected");
                return false;
            case SDL_QUIT:
                LOGD("User requested to quit");
                return true;
            default:
                sc_screen_otg_handle_event(&s->screen_otg, &event);
                break;
        }
    }
    return false;
}

bool
scrcpy_otg(struct scrcpy_options *options) {
    static struct scrcpy_otg scrcpy_otg;
    struct scrcpy_otg *s = &scrcpy_otg;

    const char *serial = options->serial;
    assert(serial);

    // Minimal SDL initialization
    if (SDL_Init(SDL_INIT_EVENTS)) {
        LOGC("Could not initialize SDL: %s", SDL_GetError());
        return false;
    }

    atexit(SDL_Quit);

    bool hid_keyboard_initialized = false;
    bool hid_mouse_initialized = false;
    bool aoa_started = false;

    struct sc_hid_keyboard *keyboard = NULL;
    struct sc_hid_mouse *mouse = NULL;

    static const struct sc_aoa_callbacks cbs = {
        .on_disconnected = sc_aoa_on_disconnected,
    };
    bool ok = sc_aoa_init(&s->aoa, serial, NULL, &cbs, NULL);
    if (!ok) {
        return false;
    }

    ok = sc_hid_keyboard_init(&s->keyboard, &s->aoa);
    if (!ok) {
        goto end;
    }
    hid_keyboard_initialized = true;
    keyboard = &s->keyboard;

    ok = sc_hid_mouse_init(&s->mouse, &s->aoa);
    if (!ok) {
        goto end;
    }
    hid_mouse_initialized = true;
    mouse = &s->mouse;

    ok = sc_aoa_start(&s->aoa);
    if (!ok) {
        goto end;
    }
    aoa_started = true;

    // TODO device name
    const char *window_title = options->window_title ? options->window_title
                                                     : "scrcpy";

    struct sc_screen_otg_params params = {
        .keyboard = keyboard,
        .mouse = mouse,
        .window_title = window_title,
        .always_on_top = options->always_on_top,
        .window_x = options->window_x,
        .window_y = options->window_y,
        .window_borderless = options->window_borderless,
    };

    if (!sc_screen_otg_init(&s->screen_otg, &params)) {
        return false;
    }

    bool ret = event_loop(s);
    LOGD("quit...");

end:
    if (hid_keyboard_initialized) {
        sc_hid_keyboard_destroy(&s->keyboard);
    }
    if (hid_mouse_initialized) {
        sc_hid_mouse_destroy(&s->mouse);
    }
    if (aoa_started) {
        sc_aoa_stop(&s->aoa);
        sc_aoa_join(&s->aoa);
    }
    sc_aoa_destroy(&s->aoa);

    return ret;
}
