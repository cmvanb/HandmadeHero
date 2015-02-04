/*
    Handmade Hero
*/

#include <windows.h>
#include <stdint.h>
#include <xinput.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

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
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;

// WTF? Day 6...
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(0);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(0);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// End WTF

internal void Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");

    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
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
            bool WasDown = ((LParam & (1 << 30)) != 0);
            bool IsDown = ((LParam & (1 << 31)) == 0);

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

            int XOffset = 0;
            int YOffset = 0;

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

                        bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool LeftThumb = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
                        bool RightThumb = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
                        bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;

                        XOffset += StickX >> 12;
                        YOffset += StickY >> 12;

                        XINPUT_VIBRATION Vibration;

                        Vibration.wLeftMotorSpeed = 0;
                        Vibration.wRightMotorSpeed = 0;

                        if (AButton)
                        {
                            Vibration.wLeftMotorSpeed = 60000;
                            Vibration.wRightMotorSpeed = 60000;
                        }

                        XInputSetState(0, &Vibration);
                    }
                    else
                    {
                        // Controller is NOT plugged in.
                    }
                }


                RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);

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
