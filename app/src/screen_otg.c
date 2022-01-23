#include "screen_otg.h"

#include "icon.h"
#include "options.h"
#include "util/log.h"

static void
sc_screen_otg_capture_mouse(struct sc_screen_otg *screen, bool capture) {
    if (SDL_SetRelativeMouseMode(capture)) {
        LOGE("Could not set relative mouse mode to %s: %s",
             capture ? "true" : "false", SDL_GetError());
        return;
    }

    screen->mouse_captured = capture;
}

static void
sc_screen_otg_render(struct sc_screen_otg *screen) {
    SDL_RenderClear(screen->renderer);
    if (screen->texture) {
        SDL_RenderCopy(screen->renderer, screen->texture, NULL, NULL);
    }
    SDL_RenderPresent(screen->renderer);
}

bool
sc_screen_otg_init(struct sc_screen_otg *screen,
                   const struct sc_screen_otg_params *params) {
    screen->keyboard = params->keyboard;
    screen->mouse = params->mouse;

    screen->mouse_captured = false;
    screen->mouse_capture_key_pressed = 0;

    const char *title = params->window_title;
    assert(title);

    int x = params->window_x != SC_WINDOW_POSITION_UNDEFINED
          ? params->window_x : (int) SDL_WINDOWPOS_UNDEFINED;
    int y = params->window_y != SC_WINDOW_POSITION_UNDEFINED
          ? params->window_y : (int) SDL_WINDOWPOS_UNDEFINED;
    int width = 256;
    int height = 256;

    uint32_t window_flags = 0;
    if (params->always_on_top) {
        window_flags |= SDL_WINDOW_ALWAYS_ON_TOP;
    }
    if (params->window_borderless) {
        window_flags |= SDL_WINDOW_BORDERLESS;
    }

    screen->window = SDL_CreateWindow(title, x, y, width, height, window_flags);
    if (!screen->window) {
        LOGE("Could not create window: %s", SDL_GetError());
        return false;
    }

    screen->renderer = SDL_CreateRenderer(screen->window, -1, 0);
    if (!screen->renderer) {
        LOGE("Could not create renderer: %s", SDL_GetError());
        goto error_destroy_window;
    }

    SDL_Surface *icon = scrcpy_icon_load();

    if (icon) {
        SDL_SetWindowIcon(screen->window, icon);

        screen->texture = SDL_CreateTextureFromSurface(screen->renderer, icon);
        scrcpy_icon_destroy(icon);
        if (!screen->texture) {
            goto error_destroy_renderer;
        }
    } else {
        screen->texture = NULL;
        LOGW("Could not load icon");
    }

    return true;

error_destroy_window:
    SDL_DestroyWindow(screen->window);
error_destroy_renderer:
    SDL_DestroyRenderer(screen->renderer);

    return false;
}

void
sc_screen_otg_destroy(struct sc_screen_otg *screen) {
    if (screen->texture) {
        SDL_DestroyTexture(screen->texture);
    }
    SDL_DestroyRenderer(screen->renderer);
    SDL_DestroyWindow(screen->window);
}

static inline bool
sc_screen_otg_is_mouse_capture_key(SDL_Keycode key) {
    return key == SDLK_LALT || key == SDLK_LGUI || key == SDLK_RGUI;
}

static void
sc_screen_otg_forward_event(struct sc_screen_otg *screen, SDL_Event *event) {
    switch 
}

void
sc_screen_otg_handle_event(struct sc_screen_otg *screen, SDL_Event *event) {
    switch (event->type) {
        case SDL_WINDOWEVENT:
            switch (event->window.event) {
                case SDL_WINDOWEVENT_EXPOSED:
                    sc_screen_otg_render(screen);
                    break;
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    sc_screen_otg_capture_mouse(screen, false);
                    break;
            }
            return;
        case SDL_KEYDOWN: {
            SDL_Keycode key = event->key.keysym.sym;
            if (sc_screen_otg_is_mouse_capture_key(key)) {
                if (!screen->mouse_capture_key_pressed) {
                    screen->mouse_capture_key_pressed = key;
                } else {
                    // Another mouse capture key has been pressed, cancel mouse
                    // (un)capture
                    screen->mouse_capture_key_pressed = 0;
                }
            }

            sc_screen_otg_process_key(screen, &event->key);
            break;
        }
        case SDL_KEYUP: {
            SDL_Keycode key = event->key.keysym.sym;
            SDL_Keycode cap = screen->mouse_capture_key_pressed;
            screen->mouse_capture_key_pressed = 0;
            if (key == cap) {
                // A mouse capture key has been pressed then released:
                // toggle the capture mouse mode
                sc_screen_otg_capture_mouse(screen, !screen->mouse_captured);
            } else {
                sc_screen_otg_process_key(screen, &event->key);
            }
            break;
        }
        case SDL_MOUSEWHEEL:
            if (screen->mouse_captured) {
                sc_screen_otg_process_mouse_wheel(screen, &event->wheel);
            }
            break;
        case SDL_MOUSEMOTION:
            if (screen->mouse_captured) {
                sc_screen_otg_process_mouse_motion(screen, &event->motion);
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (screen->mouse_captured) {
                sc_screen_otg_process_mouse_button(screen, &event->button);
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (screen->mouse_captured) {
                sc_screen_otg_process_mouse_button(screen, &event->button);
            } else {
                sc_screen_otg_capture_mouse(screen, true);
            }
            break;
    }

    sc_screen_otg_forward_event(screen, event);
}
