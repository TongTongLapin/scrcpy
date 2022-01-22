#include <scrcpy_otg.h>

#include <SDL2/SDL.h>

#include "screen_otg.h"

struct scrcpy_otg {
    struct sc_aoa aoa;
    struct sc_hid_keyboard keyboard_hid;
    struct sc_hid_mouse mouse_hid;

    struct sc_screen_hidonly screen_hidonly;
};

static void
sc_aoa_on_disconnected(struct sc_aoa *aoa, void *userdata) {
    (void) userdata;

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

    bool aoa_hid_initialized = false;
    bool hid_keyboard_initialized = false;
    bool hid_mouse_initialized = false;

    struct sc_hid_keyboard *keyboard = NULL;
    struct sc_hid_mouse *mouse = NULL;

    static const struct sc_aoa_callbacks cbs = {
        .on_disconnected = sc_aoa_on_disconnected,
    };
    bool ok = sc_aoa_init(&s->aoa, serial, NULL, cbs, NULL);
    if (!ok) {
        goto end;
    }
    aoa_hid_initialized = true;

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

    const char *window_title = options->window_title;

    struct sc_screen_hidonly_params params = {
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

    bool ret = event_loop(&s);

end:
    if (hid_keyboard_initialized) {
        if (hid_mouse_initialized) {
            
        }
        if (hid_keyboard_initialized) {

        }
    }
}
