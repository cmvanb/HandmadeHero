#if !defined(HANDMADE_H)
/* =============================================================================
    Handmade Hero
============================================================================= */

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

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

struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_controller_input
{
    bool32 IsAnalog;
    real32 StartX;
    real32 StartY;
    real32 MinX;
    real32 MinY;
    real32 MaxX;
    real32 MaxY;
    real32 EndX;
    real32 EndY;

    union
    {
        game_button_state Buttons[6];
        struct
        {
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
        };
    };
};

struct game_input
{
    game_controller_input Controllers[4];
};

// 4 things: timing, input, bitmap buffer to use, sound buffer to use
internal void GameUpdateAndRender(game_input *Input,
    game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer);

#define HANDMADE_H
#endif
