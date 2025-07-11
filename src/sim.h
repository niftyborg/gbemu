#ifndef SIM_H_
#define SIM_H_

#include <stddef.h>

#define WINDOW_TITLE "Game Boy Emulator"
#define TARGET_FPS 10
#define WIN_SCALE 5

struct dim {
    size_t h;
    size_t w;
};

void run_sim (void);
#endif // SIM_H_
