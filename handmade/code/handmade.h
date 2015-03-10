#if !defined(HANDMADE_H)
/*
    Handmade Hero
*/

// Services that the platform layer provides to the game

// TODO: ...

// Services that the game provides to the platform layer
// May expand in future - sound in separate thread, etc.

struct game_offscreen_buffer
{
    // Pixels are always 32 bits wide, memory order BB GG RR XX
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

// 4 things: timing, input, bitmap buffer to use, sound buffer to use
internal void GameUpdateAndRender(game_offscreen_buffer *Buffer);

#define HANDMADE_H
#endif
