#if !defined(HANDMADE_H)
/* =============================================================================
    Handmade Hero
============================================================================= */

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

struct game_sound_output_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16 *Samples;
};

// 4 things: timing, input, bitmap buffer to use, sound buffer to use
internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset,
    int GreenOffset, game_sound_output_buffer *SoundBuffer, int ToneHz);

#define HANDMADE_H
#endif
