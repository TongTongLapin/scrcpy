#ifndef SC_SCREEN_OTG_H
#define SC_SCREEN_OTG_H

#include "common.h"

#include <stdbool.h>

struct sc_screen_otg {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
};

struct sc_screen_otg_params {

};

#endif
