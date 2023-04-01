#include "CPatch.h"
#include "MinHook/MinHook.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include <Windows.h>
#include <dinput.h>
#include <stdio.h>
#include <thread>
#include <unordered_map>

void DebugLog(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[512];
    vsprintf(buf, fmt, args);
    OutputDebugStringA(buf);
    va_end(args);
}

BOOL& isFirstPerson = *(BOOL*)0x00F6DE80;
uint8_t& stageId    = *(uint8_t*)0x00A08198;

SDL_Window* gWindow;

typedef HWND(WINAPI* CreateWindowExA_func)(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
typedef BOOL(WINAPI* SetWindowTextA_func)(HWND hWnd, LPCSTR lpString);
typedef ATOM(WINAPI* RegisterClassExA_func)(const WNDCLASSEXA* wndClass);

CreateWindowExA_func CreateWindowExA_original   = nullptr;
SetWindowTextA_func SetWindowTextA_original     = nullptr;
RegisterClassExA_func RegisterClassExA_original = nullptr;
WNDPROC origWndProc                             = nullptr;
HWND currentWindow                              = nullptr;
POINT curMousePos, centerMousePos;

bool gMainInitialized = false;
bool gKeyboardEnabled = false, gMouseEnabled = false;

LRESULT WINAPI WndProc_Hook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if ((uMsg != WM_KEYDOWN && uMsg != WM_KEYUP && uMsg != WM_MOUSEMOVE)) {
        return CallWindowProcA(origWndProc, hWnd, uMsg, wParam, lParam);
    }

    if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) {
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
            return CallWindowProcA(origWndProc, hWnd, WM_KEYDOWN, VK_ESCAPE, lParam);
        }
    }

    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

BOOL WINAPI SetWindowTextA_Hook(HWND hWnd, LPCSTR lpString) {
    return SetWindowTextA_original(hWnd, "MGS2 - SDL Input Hook");
}

ATOM WINAPI RegisterClassExA_Hook(const WNDCLASSEXA* wndClass) {
    WNDCLASSEXA c = *wndClass;

    origWndProc = c.lpfnWndProc;

    c.lpfnWndProc = WndProc_Hook;

    return RegisterClassExA_original(&c);
}

HWND WINAPI CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    HWND hWnd = CreateWindowExA_original(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    if (!gWindow) {
        gWindow = SDL_CreateWindowFrom(hWnd);

        SDL_ShowCursor(SDL_FALSE);
        SDL_SetRelativeMouseMode(SDL_TRUE);

        gKeyboardEnabled = true;
        gMouseEnabled    = true;

        DebugLog("HWND: 0x%08X\n", hWnd);

        if (!gWindow) {
            char buf[256];
            sprintf(buf, "Failed to create an main SDL window: %s", SDL_GetError());
            MessageBoxA(hWnd, buf, "Error", MB_OK | MB_ICONERROR);
            ExitProcess(1);
        }
    }

    return hWnd;
}

// Digital Inputs
BOOL inputCross     = false;
BOOL inputCircle    = false;
BOOL inputTriangle  = false;
BOOL inputSquare    = false;
BOOL inputL1        = false;
BOOL inputL2        = false;
BOOL inputL3        = false;
BOOL inputR1        = false;
BOOL inputR2        = false;
BOOL inputR3        = false;
BOOL inputStart     = false;
BOOL inputSelect    = false;
BOOL inputDpadUp    = false;
BOOL inputDpadRight = false;
BOOL inputDpadDown  = false;
BOOL inputDpadLeft  = false;

// Analog Sticks
BYTE inputLeftStickX  = 127;
BYTE inputLeftStickY  = 127;
BYTE inputRightStickX = 127;
BYTE inputRightStickY = 127;

// Pressure Sensitive Inputs
BYTE inputCrossPressure     = 0;
BYTE inputCirclePressure    = 0;
BYTE inputTrianglePressure  = 0;
BYTE inputSquarePressure    = 0;
BYTE inputL1Pressure        = 0;
BYTE inputL2Pressure        = 0;
BYTE inputR1Pressure        = 0;
BYTE inputR2Pressure        = 0;
BYTE inputDpadUpPressure    = 0;
BYTE inputDpadRightPressure = 0;
BYTE inputDpadDownPressure  = 0;
BYTE inputDpadLeftPressure  = 0;
BOOL inputSlowPressure      = false;

// Rumble
BYTE outputRumble = 0;

enum InputButton { IN_L1, IN_R1, IN_L2, IN_R2, IN_TRIANGLE, IN_CIRCLE, IN_CROSS, IN_SQUARE, IN_L3, IN_R3, IN_START, IN_SELECT, IN_DPUP, IN_DPDOWN, IN_DPLEFT, IN_DPRIGHT };

inline void ProcessButton(InputButton button, bool pressed) {
    switch (button) {
    case IN_L1:
        if (pressed) {
            inputL1         = FALSE;
            inputL1Pressure = 255;
        } else {
            inputL1         = TRUE;
            inputL1Pressure = 0;
        }
        break;
    case IN_R1:
        if (pressed) {
            inputR1         = FALSE;
            inputR1Pressure = 255;
        } else {
            inputR1         = TRUE;
            inputR1Pressure = 0;
        }
        break;
    case IN_L2:
        if (pressed) {
            inputL2         = FALSE;
            inputL2Pressure = 255;
        } else {
            inputL2         = TRUE;
            inputL2Pressure = 0;
        }
        break;
    case IN_R2:
        if (pressed) {
            inputR2         = FALSE;
            inputR2Pressure = 255;
        } else {
            inputR2         = TRUE;
            inputR2Pressure = 0;
        }
        break;
    case IN_TRIANGLE:
        if (pressed) {
            inputTriangle         = FALSE;
            inputTrianglePressure = 255;
        } else {
            inputTriangle         = TRUE;
            inputTrianglePressure = 0;
        }
        break;
    case IN_CIRCLE:
        if (pressed) {
            inputCircle         = FALSE;
            inputCirclePressure = 255;
        } else {
            inputCircle         = TRUE;
            inputCirclePressure = 0;
        }
        break;
    case IN_CROSS:
        if (pressed) {
            inputCross         = FALSE;
            inputCrossPressure = 255;
        } else {
            inputCross         = TRUE;
            inputCrossPressure = 0;
        }
        break;
    case IN_SQUARE:
        if (pressed) {
            inputSquare         = FALSE;
            inputSquarePressure = 255;
        } else {
            inputSquare         = TRUE;
            inputSquarePressure = 0;
        }
        break;
    case IN_SELECT:
        if (pressed) {
            inputSelect = FALSE;
        } else {
            inputSelect = TRUE;
        }
        break;
    case IN_L3:
        if (pressed) {
            inputL3 = FALSE;
        } else {
            inputL3 = TRUE;
        }
        break;
    case IN_R3:
        if (pressed) {
            inputR3 = FALSE;
        } else {
            inputR3 = TRUE;
        }
        break;
    case IN_START:
        if (pressed) {
            inputStart = FALSE;
        } else {
            inputStart = TRUE;
        }
        break;
    case IN_DPUP:
        if (pressed) {
            inputDpadUp         = FALSE;
            inputDpadUpPressure = 255;
        } else {
            inputDpadUp         = TRUE;
            inputDpadUpPressure = 0;
        }
        break;
    case IN_DPRIGHT:
        if (pressed) {
            inputDpadRight         = FALSE;
            inputDpadRightPressure = 255;
        } else {
            inputDpadRight         = TRUE;
            inputDpadRightPressure = 0;
        }
        break;
    case IN_DPDOWN:
        if (pressed) {
            inputDpadDown         = FALSE;
            inputDpadDownPressure = 255;
        } else {
            inputDpadDown         = TRUE;
            inputDpadDownPressure = 0;
        }
        break;
    case IN_DPLEFT:
        if (pressed) {
            inputDpadLeft         = FALSE;
            inputDpadLeftPressure = 255;
        } else {
            inputDpadLeft         = TRUE;
            inputDpadLeftPressure = 0;
        }
        break;
    }
}

void UpdateInput() {
    BYTE gamePrimaryButtons = 255; // 0x00EDAC98
    BYTE gameL1             = 1;   // Bit 0
    BYTE gameR1             = 2;   // Bit 1
    BYTE gameL2             = 4;   // Bit 2
    BYTE gameR2             = 8;   // Bit 3
    BYTE gameTriangle       = 16;  // Bit 4
    BYTE gameCircle         = 32;  // Bit 5
    BYTE gameCross          = 64;  // Bit 6
    BYTE gameSquare         = 128; // Bit 7

    BYTE gameSecondaryButtons = 255; // 0x00EDAC99
    BYTE gameSelect           = 1;   // Bit 0
    BYTE gameL3               = 2;   // Bit 1
    BYTE gameR3               = 4;   // Bit 2
    BYTE gameStart            = 8;   // Bit 3
    BYTE gameDpadUp           = 16;  // Bit 4
    BYTE gameDpadRight        = 32;  // Bit 5
    BYTE gameDpadDown         = 64;  // Bit 6
    BYTE gameDpadLeft         = 128; // Bit 7

    BYTE gameRightStickX = 127; // 0x00EDAC9C
    BYTE gameRightStickY = 127; // 0x00EDAC9D
    BYTE gameLeftStickX  = 127; // 0x00EDAC9E
    BYTE gameLeftStickY  = 127; // 0x00EDAC9F

    // Disable buttons
    CPatch::Nop(0x008CF65E, 6);

    // Disable pressure sensitivity
    CPatch::Nop(0x008CF656, 2);

    // Disable left stick
    CPatch::Nop(0x008CF632, 5); // X-axis
    CPatch::Nop(0x008CF63D, 6); // Y-axis

    // Disable right stick
    CPatch::Nop(0x008CF637, 6); // X-axis
    CPatch::Nop(0x008CF62C, 6); // Y-axis

    // Center sticks
    CPatch::SetChar(0x00EDAC9E, 127); // Left X-axis
    CPatch::SetChar(0x00EDAC9F, 127); // Left Y-axis
    CPatch::SetChar(0x00EDAC9C, 127); // Right X-axis
    CPatch::SetChar(0x00EDAC9D, 127); // Right Y-axis

    // Set pressure sensitivity to 0
    CPatch::SetChar(0x00EDACA0, 0); // Dpad Left
    CPatch::SetChar(0x00EDACA1, 0); // Dpad Right
    CPatch::SetChar(0x00EDACA2, 0); // Dpad Up
    CPatch::SetChar(0x00EDACA3, 0); // Dpad Down
    CPatch::SetChar(0x00EDACA4, 0); // Triangle
    CPatch::SetChar(0x00EDACA5, 0); // Circle
    CPatch::SetChar(0x00EDACA6, 0); // Cross
    CPatch::SetChar(0x00EDACA7, 0); // Square
    CPatch::SetChar(0x00EDACA8, 0); // L2
    CPatch::SetChar(0x00EDACA9, 0); // R2
    CPatch::SetChar(0x00EDACAA, 0); // L1
    CPatch::SetChar(0x00EDACAB, 0); // R1

    // Fix pressure check
    CPatch::SetChar(0x00EDAC9A, 255);

    CPatch::SetChar(0x00EDAC98, 255);
    CPatch::SetChar(0x00EDAC99, 255);

    while (true) {
        // Reset input states
        gamePrimaryButtons   = 255;
        gameSecondaryButtons = 255;
        gameLeftStickX       = 127;
        gameLeftStickY       = 127;
        gameRightStickX      = 127;
        gameRightStickY      = 127;

        // Update button states
        if (inputCross) {
            gamePrimaryButtons = gamePrimaryButtons ^ gameCross;
        }
        if (inputCircle) {
            gamePrimaryButtons = gamePrimaryButtons ^ gameCircle;
        }
        if (inputTriangle) {
            gamePrimaryButtons = gamePrimaryButtons ^ gameTriangle;
        }
        if (inputSquare) {
            gamePrimaryButtons = gamePrimaryButtons ^ gameSquare;
        }
        if (inputL1) {
            gamePrimaryButtons = gamePrimaryButtons ^ gameL1;
        }
        if (inputL2) {
            gamePrimaryButtons = gamePrimaryButtons ^ gameL2;
        }
        if (inputL3) {
            gameSecondaryButtons = gameSecondaryButtons ^ gameL3;
        }
        if (inputR1) {
            gamePrimaryButtons = gamePrimaryButtons ^ gameR1;
        }
        if (inputR2) {
            gamePrimaryButtons = gamePrimaryButtons ^ gameR2;
        }
        if (inputR3) {
            gameSecondaryButtons = gameSecondaryButtons ^ gameR3;
        }
        if (inputStart) {
            gameSecondaryButtons = gameSecondaryButtons ^ gameStart;
        }
        if (inputSelect) {
            gameSecondaryButtons = gameSecondaryButtons ^ gameSelect;
        }
        if (inputDpadUp) {
            gameSecondaryButtons = gameSecondaryButtons ^ gameDpadUp;
        }
        if (inputDpadRight) {
            gameSecondaryButtons = gameSecondaryButtons ^ gameDpadRight;
        }
        if (inputDpadDown) {
            gameSecondaryButtons = gameSecondaryButtons ^ gameDpadDown;
        }
        if (inputDpadLeft) {
            gameSecondaryButtons = gameSecondaryButtons ^ gameDpadLeft;
        }

        CPatch::SetChar(0x00EDAC98, gamePrimaryButtons);
        CPatch::SetChar(0x00EDAC99, gameSecondaryButtons);

        // Feather L2/R2 pressure
        inputL2Pressure = inputL2Pressure * 0.7f;
        inputR2Pressure = inputR2Pressure * 0.7f;

        // Update pressure states
        CPatch::SetChar(0x00EDACA6, inputCrossPressure);
        CPatch::SetChar(0x00EDACA5, inputCirclePressure);
        CPatch::SetChar(0x00EDACA4, inputTrianglePressure);
        CPatch::SetChar(0x00EDACA7, inputSquarePressure);
        CPatch::SetChar(0x00EDACAA, inputL1Pressure);
        CPatch::SetChar(0x00EDACA8, inputL2Pressure);
        CPatch::SetChar(0x00EDACAB, inputR1Pressure);
        CPatch::SetChar(0x00EDACA9, inputR2Pressure);
        CPatch::SetChar(0x00EDACA2, inputDpadUpPressure);
        CPatch::SetChar(0x00EDACA0, inputDpadRightPressure);
        CPatch::SetChar(0x00EDACA1, inputDpadDownPressure);
        CPatch::SetChar(0x00EDACA3, inputDpadLeftPressure);

        if (inputSlowPressure) {
            CPatch::SetChar(0x00EDAC94, 253);
        } else {
            CPatch::SetChar(0x00EDAC9A, 255);
        }

        // Ramp sticks towards corners
        if (inputLeftStickX < 40) {
            inputLeftStickX = 0;
        }
        if (inputLeftStickY < 40) {
            inputLeftStickY = 0;
        }
        if (inputRightStickX < 40) {
            inputRightStickX = 0;
        }
        if (inputRightStickY < 40) {
            inputRightStickY = 0;
        }
        if (inputLeftStickX > 215) {
            inputLeftStickX = 255;
        }
        if (inputLeftStickY > 215) {
            inputLeftStickY = 255;
        }
        if (inputRightStickX > 215) {
            inputRightStickX = 255;
        }
        if (inputRightStickY > 215) {
            inputRightStickY = 255;
        }

        // Update stick states
        gameLeftStickX  = inputLeftStickX;
        gameLeftStickY  = inputLeftStickY;
        gameRightStickX = inputRightStickX;
        gameRightStickY = inputRightStickY;

        CPatch::SetChar(0x00EDAC9E, gameLeftStickX);
        CPatch::SetChar(0x00EDAC9F, gameLeftStickY);
        CPatch::SetChar(0x00EDAC9C, gameRightStickX);
        CPatch::SetChar(0x00EDAC9D, gameRightStickY);

        // Update Rumble
        BYTE* gameRumble = (BYTE*)0x00EDAC8D;

        if (*gameRumble >= 1 && *gameRumble <= 255) {
            outputRumble = *gameRumble;
        } else {
            outputRumble = 0;
        }

        // Polling rate 250Hz
        Sleep(4);
    }
}

void SDLEventLoop() {
    for (;;) {
        if (gWindow) {
            SDL_Event e;

            int button = 0;

            while (SDL_PollEvent(&e)) {
                switch (e.type) {
                case SDL_WINDOWEVENT:
                    switch (e.window.event) {
                    case SDL_WINDOWEVENT_FOCUS_GAINED: gKeyboardEnabled = true; break;
                    case SDL_WINDOWEVENT_FOCUS_LOST: gKeyboardEnabled = false; break;

                    case SDL_WINDOWEVENT_ENTER:
                        SDL_ShowCursor(SDL_FALSE);
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        gMouseEnabled = true;
                        break;

                    case SDL_WINDOWEVENT_LEAVE:
                        SDL_ShowCursor(SDL_TRUE);
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        gMouseEnabled = false;
                        break;
                    }
                    break;

                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    /*char buf[256];
                    sprintf(buf, "Key: 0x%02X (%s) - %s\n", e.key.keysym.scancode, SDL_GetKeyName(e.key.keysym.sym), e.key.state == SDL_PRESSED ? "pressed" : "released");
                    OutputDebugStringA(buf);
                    */

                    if (gKeyboardEnabled) {
                        switch (e.key.keysym.sym) {
                        case SDLK_w:
                        case SDLK_UP: ProcessButton(IN_DPUP, e.key.state != SDL_PRESSED); break;
                        case SDLK_s:
                        case SDLK_DOWN: ProcessButton(IN_DPDOWN, e.key.state != SDL_PRESSED); break;
                        case SDLK_a:
                        case SDLK_LEFT: ProcessButton(IN_DPLEFT, e.key.state != SDL_PRESSED); break;
                        case SDLK_d:
                        case SDLK_RIGHT: ProcessButton(IN_DPRIGHT, e.key.state != SDL_PRESSED); break;
                        case SDLK_RETURN:
                            if (stageId == 60) {
                                ProcessButton(IN_CROSS, e.key.state != SDL_PRESSED);
                            }
                            break;
                        case SDLK_TAB: ProcessButton(IN_SELECT, e.key.state != SDL_PRESSED); break;
                        case SDLK_LCTRL: ProcessButton(IN_R1, e.key.state != SDL_PRESSED); break;
                        case SDLK_ESCAPE:
                            if (stageId == 60) {
                                ProcessButton(IN_CIRCLE, e.key.state != SDL_PRESSED);
                            } else {
                                ProcessButton(IN_START, e.key.state != SDL_PRESSED);
                            }
                            break;
                        }
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    switch (e.button.button) {
                    case SDL_BUTTON_LEFT: button = 0; break;
                    case SDL_BUTTON_RIGHT: button = 1; break;
                    case SDL_BUTTON_MIDDLE: button = 2; break;
                    }

                    char buf2[256];
                    sprintf(buf2, "Mouse press: %d - %s\n", button, e.button.state == SDL_PRESSED ? "pressed" : "released");
                    OutputDebugStringA(buf2);

                    break;
                case SDL_MOUSEMOTION:
                    sprintf(buf2, "Mouse motion: %d - %d\n", e.motion.xrel, e.motion.yrel);
                    OutputDebugStringA(buf2);
                    if (gMouseEnabled) {
                        if (isFirstPerson) {
                            int inputX = 127 + (e.motion.xrel * 2);
                            int inputY = 127 + (e.motion.yrel * 2);

                            if (inputX < 0)
                                inputX = 0;
                            else if (inputX > 255)
                                inputX = 255;

                            if (inputY < 0)
                                inputY = 0;
                            else if (inputY > 255)
                                inputY = 255;

                            inputLeftStickX = (inputX & 0XFF);
                            inputLeftStickY = (inputY & 0XFF);
                            sprintf(buf2, "Left stick: %d - %d\n", inputLeftStickX, inputLeftStickY);
                            OutputDebugStringA(buf2);
                        } else {
                            int inputX = 127 + (e.motion.xrel * 2);
                            int inputY = 127 + (e.motion.yrel * 2);

                            if (inputX < 0)
                                inputX = 0;
                            else if (inputX > 255)
                                inputX = 255;

                            if (inputY < 0)
                                inputY = 0;
                            else if (inputY > 255)
                                inputY = 255;

                            inputRightStickX = (inputX & 0XFF);
                            inputRightStickY = (inputY & 0XFF);
                            sprintf(buf2, "Right stick: %d - %d\n", inputRightStickX, inputRightStickY);
                            OutputDebugStringA(buf2);
                        }
                    }
                    break;
                }
            }
        }
    }
}

extern "C" __declspec(dllexport) void InitializeASI() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        char buf[256];
        sprintf(buf, "Failed to initialize SDL: %s", SDL_GetError());
        MessageBoxA(NULL, buf, "Error", MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }

    MH_Initialize();

    MH_CreateHookApi(L"user32", "CreateWindowExA", &CreateWindowExA_Hook, (LPVOID*)&CreateWindowExA_original);
    MH_CreateHookApi(L"user32", "SetWindowTextA", &SetWindowTextA_Hook, (LPVOID*)&SetWindowTextA_original);
    MH_CreateHookApi(L"user32", "RegisterClassExA", &RegisterClassExA_Hook, (LPVOID*)&RegisterClassExA_original);

    MH_EnableHook(MH_ALL_HOOKS);

    std::thread sdlThread(SDLEventLoop), inputThread(UpdateInput);
    sdlThread.detach();
    inputThread.detach();
}