#include "sim.h"

#include <raylib.h>

#include "util.h"
#include "jtest.h"

void
run_sim (void)
{
    // Game Boy screen: 160px across by 144px
    struct dim vdim = { .w = 160, .h = 144 };
    // physical screen
    struct dim pdim = { .w = vdim.w * WIN_SCALE * 2, .h = vdim.h * WIN_SCALE };

    struct string rom_data = {0};
    assert (read_file("./roms/tetris.gb", &rom_data) == 0
            && "Failed to load tetris rom");
    /* dump_rom(&rom_data); */

    InitWindow(pdim.w, pdim.h, WINDOW_TITLE);
    SetTargetFPS(TARGET_FPS);
    size_t curr_pos = 0;
    while (!WindowShouldClose()) {
        /* update_input (&ctx); */
        /* for (int i = 0; i < 10; i++) ops_dispatch(&ctx); */

        BeginDrawing();
        ClearBackground((Color) { .r = 0, .g = 0, .b = 0, .a = 255 });
        // REGISTER VALUES
        Vector2 text_pos = { .x = pdim.w/2, .y = 0 };
        Vector2 reg_spacing = { .x = 60, .y = 20 };
        for (size_t i = 0; i < GPR_LEN; i++) {
            size_t x_off = i % 6 * reg_spacing.x;
            if (i % 6 == 0) text_pos.y += reg_spacing.y;
            DrawText(TextFormat("%02d: 0", i), text_pos.x + x_off, text_pos.y, 14, WHITE);
        }
        // INSTRUCTIONS
        text_pos = (Vector2) { .x = pdim.w/2, .y = pdim.h/2 };
        for (size_t i = 0; curr_pos < rom_data.len && i < 20; curr_pos++, i++) {
            size_t x_off = i % 6 * reg_spacing.x;
            if (i % 6 == 0) text_pos.y += reg_spacing.y;
            DrawText(TextFormat("%02X", rom_data.str[curr_pos]), text_pos.x + x_off, text_pos.y, 14, WHITE);
        }
        EndDrawing();
        curr_pos++;
    }
    CloseWindow();
}
