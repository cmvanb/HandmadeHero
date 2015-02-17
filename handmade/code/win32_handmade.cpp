/*
    Handmade Hero
*/

#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

// TODO: implement sin ourselves
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

typedef int32 bool32;

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};



// TODO: Global for now.
global_variable bool32 GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;



// Importing library calls in a way that makes them fail safe.
// XInput
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// DirectSound
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal void Win32LoadXInput(void)
{
    // TODO: Test on windows 8.
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");

    if (!XInputLibrary)
    {
        OutputDebugStringA("unable to load 1.3, trying 1.4\n");
        HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    }

    if (!XInputLibrary)
    {
        OutputDebugStringA("unable to load 1.4, trying 9.1.0\n");
        HMODULE XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }

    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
    else
    {
        OutputDebugStringA("failed to load xinput DLL\n");
    }
}

internal void Win32InitDirectSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    // Load the library
    HMODULE DirectSoundLibrary = LoadLibraryA("dsound.dll");

    if (DirectSoundLibrary)
    {
        // Get DirectSound object - cooperative
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DirectSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND DirectSound;

        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};

            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                // Create a primary buffer
                LPDIRECTSOUNDBUFFER PrimaryBuffer;

                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {
                    HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);

                    if (SUCCEEDED(Error))
                    {
                        // We have finally set the format
                        OutputDebugStringA("Primary buffer format was set.\n");
                    }
                    else
                    {
                        // TODO: Log error
                    }
                }
                else
                {
                    // TODO: Log error
                }
            }
            else
            {
                // TODO: Log error
            }

            // Create a secondary buffer
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;

            HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);

            if (SUCCEEDED(Error))
            {
                OutputDebugStringA("Secondary buffer created successfully.\n");
            }
        }
        else
        {
            // TODO: Log error
        }
    }
    else
    {
        // TODO: Log error
    }
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);

    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

internal void RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    uint8 *Row = (uint8 *)Buffer->Memory;

    for (int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;

        for (int X = 0; X < Buffer->Width; ++X)
        {
            // BB GG RR xx
            uint8 Blue = X + XOffset;
            uint8 Green = Y + YOffset;
            uint8 Red = 0;

            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer->Pitch;
    }
}

internal void Win32ResizeDIBSection(
    win32_offscreen_buffer *Buffer,
    int Width,
    int Height)
{
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height; // Negative for top-down DIB.
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;

    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    // TODO: probably clear to black here.

    Buffer->Pitch = Width * Buffer->BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(
win32_offscreen_buffer *Buffer,
    HDC DeviceContext,
    int WindowWidth,
    int WindowHeight)
{
    // TODO: Aspect ratio correction.

    StretchDIBits(
        DeviceContext,
        /*
        X, Y, Width, Height,
        X, Y, Width, Height,
        */
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
        DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_CLOSE:
        {
            // TODO: Handle with message to user?
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            // TODO: Handle as error.
            GlobalRunning = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = WParam;
            bool32 WasDown = ((LParam & (1 << 30)) != 0);
            bool32 IsDown = ((LParam & (1 << 31)) == 0);

            if (WasDown != IsDown)
            {
                if (VKCode == 'W')
                {
                }
                else if (VKCode == 'A')
                {
                }
                else if (VKCode == 'S')
                {
                }
                else if (VKCode == 'D')
                {
                }
                else if (VKCode == 'Q')
                {
                }
                else if (VKCode == 'E')
                {
                }
                else if (VKCode == VK_UP)
                {
                }
                else if (VKCode == VK_LEFT)
                {
                }
                else if (VKCode == VK_DOWN)
                {
                }
                else if (VKCode == VK_RIGHT)
                {
                }
                else if (VKCode == VK_ESCAPE)
                {
                    OutputDebugStringA("Escape: ");

                    if (IsDown)
                    {
                        OutputDebugStringA("IsDown");
                    }
                    if (WasDown)
                    {
                        OutputDebugStringA("WasDown");
                    }

                    OutputDebugStringA("\n");
                }
                else if (VKCode == VK_SPACE)
                {
                }
            }

            bool32 AltKeyDown = ((LParam & (1 << 29)) != 0);

            if ((VKCode == VK_F4) && AltKeyDown)
            {
                GlobalRunning = false;
            }
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;

            HDC DeviceContext = BeginPaint(Window, &Paint);

            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            win32_window_dimension Dimension = Win32GetWindowDimension(Window);

            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

            EndPaint(Window, &Paint);
        } break;

        default:
        {
            //OutputDebugStringA("default\n");
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
};

struct win32_sound_output
{
    int SamplesPerSecond;
    int ToneHerz;
    int16 ToneVolume;
    uint32 RunningSampleIndex;
    int WavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
    real32 tSine;
    int LatencySampleCount;
};

internal void Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
{
    // TODO: more strenous testing necessary
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;

    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
        ByteToLock,
        BytesToWrite,
        &Region1,
        &Region1Size,
        &Region2,
        &Region2Size,
        0)))
    {
        // TODO: assert region sizes are valid
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        int16 *SampleOut = (int16 *)Region1;

        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            real32 SineValue = sinf(SoundOutput->tSine);
            int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);

            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            SoundOutput->tSine += 2.0f * Pi32 * (1.0f / (real32)SoundOutput->WavePeriod);

            ++SoundOutput->RunningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        SampleOut = (int16 *)Region2;

        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            real32 SineValue = sinf(SoundOutput->tSine);
            int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);

            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            SoundOutput->tSine += 2.0f * Pi32 * (1.0f / (real32)SoundOutput->WavePeriod);

            ++SoundOutput->RunningSampleIndex;
        }

        GlobalSecondaryBuffer->Unlock(
            Region1,
            Region1Size,
            Region2,
            Region2Size);
    }
}

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode)
{
    Win32LoadXInput();

    // Initialize struct members to zero.
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND Window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0);

        if (Window)
        {
            HDC DeviceContext = GetDC(Window);

            // Graphics test:
            int XOffset = 0;
            int YOffset = 0;

            win32_sound_output SoundOutput = {};

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.ToneHerz = 256;
            SoundOutput.ToneVolume = 8000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHerz;
            SoundOutput.BytesPerSample = sizeof(int16) * 2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;

            Win32InitDirectSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample);

            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            GlobalRunning = true;

            while (GlobalRunning)
            {
                MSG Message;

                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
                {
                    XINPUT_STATE ControllerState;

                    if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                    {
                        // Controller is plugged in.
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool32 LeftThumb = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
                        bool32 RightThumb = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
                        bool32 LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool32 RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;

                        XOffset += StickX / 8192;
                        YOffset -= StickY / 8192;

                        XINPUT_VIBRATION Vibration;

                        Vibration.wLeftMotorSpeed = 0;
                        Vibration.wRightMotorSpeed = 0;

                        if (AButton)
                        {
                            Vibration.wLeftMotorSpeed = 60000;
                            Vibration.wRightMotorSpeed = 60000;
                        }

                        SoundOutput.ToneHerz = 320 + (int)(256.0f * ((real32)StickY / 32000.0f));
                        SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHerz;

                        XInputSetState(0, &Vibration);
                    }
                    else
                    {
                        // Controller is NOT plugged in.
                    }
                }

                RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);

                // DirectSound output test
                DWORD PlayCursor;
                DWORD WriteCursor;

                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
                    DWORD BytesToWrite;
                    DWORD TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize;

                    if (ByteToLock > TargetCursor)
                    {
                        BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                        BytesToWrite += TargetCursor;
                    }
                    else
                    {
                        BytesToWrite = TargetCursor - ByteToLock;
                    }

                    Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);
                }

                win32_window_dimension Dimension = Win32GetWindowDimension(Window);

                Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
            }
        }
        else
        {
            // TODO: Log error.
        }
    }
    else
    {
        // TODO: Log error.
    }

    return(0);
}
