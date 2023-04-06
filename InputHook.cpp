#include "CPatch.h"
#include "MinHook/MinHook.h"
#include <SDL.h>
#include <SDL_syswm.h>
#include <dinput.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <thread>
#include <unordered_map>
#include "ini.h"
#include "mgs2sos.h"

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
void* gclVariables  = (void*)0xA01F34;

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
        inputL1         = pressed;
        inputL1Pressure = pressed * 255;
        break;
    case IN_R1:
        inputR1         = pressed;
        inputR1Pressure = pressed * 255;
        break;
    case IN_L2:
        inputL2         = pressed;
        inputL2Pressure = pressed * 255;
        break;
    case IN_R2:
        inputR2         = pressed;
        inputR2Pressure = pressed * 255;
        break;
    case IN_TRIANGLE:
        inputTriangle         = pressed;
        inputTrianglePressure = pressed * 255;
        break;
    case IN_CIRCLE:
        inputCircle         = pressed;
        inputCirclePressure = pressed * 255;
        break;
    case IN_CROSS:
        inputCross         = pressed;
        inputCrossPressure = pressed * 255;
        break;
    case IN_SQUARE:
        inputSquare         = pressed;
        inputSquarePressure = pressed * 255;
        break;
    case IN_SELECT: inputSelect = pressed; break;
    case IN_L3: inputL3 = pressed; break;
    case IN_R3: inputR3 = pressed; break;
    case IN_START: inputStart = pressed; break;
    case IN_DPUP:
        inputDpadUp         = pressed;
        inputDpadUpPressure = pressed * 255;
        break;
    case IN_DPRIGHT:
        inputDpadRight         = pressed;
        inputDpadRightPressure = pressed * 255;
        break;
    case IN_DPDOWN:
        inputDpadDown         = pressed;
        inputDpadDownPressure = pressed * 255;
        break;
    case IN_DPLEFT:
        inputDpadLeft         = pressed;
        inputDpadLeftPressure = pressed * 255;
        break;
    }
}

inline const char* GetStageName1() {
    return (const char*)(*(DWORD*)gclVariables + 0x1C);
}

inline const char* GetStageName2() {
    return (const char*)(*(DWORD*)gclVariables + 0x2C);
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

        /*// Feather L2/R2 pressure
        inputL2Pressure = inputL2Pressure * 0.7f;
        inputR2Pressure = inputR2Pressure * 0.7f;
        */

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

SDL_Keycode MoveForwardKey;
SDL_Keycode MoveBackwardKey;
SDL_Keycode MoveLeftKey;
SDL_Keycode MoveRightKey;
SDL_Keycode UseKey;
SDL_Keycode PunchKey;
SDL_Keycode FireKey;
SDL_Keycode CrouchKey;
SDL_Keycode ConfirmKey;
SDL_Keycode CancelKey;
SDL_Keycode CodecKey;
SDL_Keycode SelectItemKey;
SDL_Keycode SelectWeaponKey;

bool enableSos = false;

void SDLEventLoop() {
    for (;;) {
        if (gWindow) {
            SDL_Event e;
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
                        case SDLK_UP: ProcessButton(IN_DPUP, e.key.state == SDL_PRESSED); break;
                        case SDLK_s:
                        case SDLK_DOWN: ProcessButton(IN_DPDOWN, e.key.state == SDL_PRESSED); break;
                        case SDLK_a:
                        case SDLK_LEFT: ProcessButton(IN_DPLEFT, e.key.state == SDL_PRESSED); break;
                        case SDLK_d:
                        case SDLK_RIGHT: ProcessButton(IN_DPRIGHT, e.key.state == SDL_PRESSED); break;
                        case SDLK_RETURN:
                            if (stageId == 60) {
                                ProcessButton(IN_CROSS, e.key.state == SDL_PRESSED);
                            }
                            break;
                        case SDLK_SPACE: ProcessButton(IN_TRIANGLE, e.key.state == SDL_PRESSED); break;
                        case SDLK_TAB: ProcessButton(IN_SELECT, e.key.state == SDL_PRESSED); break;
                        case SDLK_LCTRL: ProcessButton(IN_SQUARE, e.key.state == SDL_PRESSED); break;
                        case SDLK_q: ProcessButton(IN_L2, e.key.state == SDL_PRESSED); break;
                        case SDLK_e: ProcessButton(IN_R2, e.key.state == SDL_PRESSED); break;
                        case SDLK_LSHIFT:
                            if (stageId != 60)
                                ProcessButton(IN_CIRCLE, e.key.state == SDL_PRESSED);
                            break;
                        case SDLK_LALT:
                            if (stageId != 60)
                                ProcessButton(IN_CROSS, e.key.state == SDL_PRESSED);
                            break;
                        case SDLK_ESCAPE:
                            if (stageId == 60) {
                                ProcessButton(IN_CIRCLE, e.key.state == SDL_PRESSED);
                            } else {
                                ProcessButton(IN_START, e.key.state == SDL_PRESSED);
                            }
                            break;
                        }
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    switch (e.button.button) {
                    case SDL_BUTTON_LEFT: ProcessButton(IN_SQUARE, e.button.state == SDL_PRESSED); break;
                    case SDL_BUTTON_RIGHT: ProcessButton(IN_L1, e.button.state == SDL_PRESSED); break;
                    case SDL_BUTTON_MIDDLE: ProcessButton(IN_R1, e.button.state == SDL_PRESSED); break;
                    }
                    break;
                case SDL_MOUSEMOTION:
                    if (gMouseEnabled) {
                        if (isFirstPerson) {
                            int inputX = 127 + (e.motion.xrel * 3);
                            int inputY = 127 + (e.motion.yrel * 3);

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
                        } else {
                            if (enableSos) {
                                mouseMovement_X = e.motion.xrel;
                                mouseMovement_Y = e.motion.yrel;
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
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
}

std::string defaultConfigFile = "[Controls]\n"
                                "MoveForward = W\n"
                                "MoveBackward = S\n"
                                "MoveLeft = A\n"
                                "MoveRight = D\n"
                                "Use = F\n"
                                "Punch = Left Shift\n"
                                "Fire = Left Ctrl\n"
                                "Crouch = Left Alt\n"
                                "Confirm = Return\n"
                                "Cancel = Escape\n"
                                "Codec = Tab\n"
                                "SelectItem = Q\n"
                                "SelectWeapon = E\n"
                                "\n"
                                "[mgs2sos]\n"
                                "Enable = true\n"
                                "Sensitivity = 0.6\n"
                                "InvertX = false\n"
                                "InvertY = false\n";

void LoadConfig() {
    try {
        inih::INIReader ini("MGS2Input.ini");
        const auto& controlsMoveForward  = ini.Get<std::string>("Controls", "MoveForward");
        const auto& controlsMoveBackward = ini.Get<std::string>("Controls", "MoveBackward");
        const auto& controlsMoveLeft     = ini.Get<std::string>("Controls", "MoveLeft");
        const auto& controlsMoveRight    = ini.Get<std::string>("Controls", "MoveRight");
        const auto& controlsUse          = ini.Get<std::string>("Controls", "Use");
        const auto& controlsPunch        = ini.Get<std::string>("Controls", "Punch");
        const auto& controlsFire         = ini.Get<std::string>("Controls", "Fire");
        const auto& controlsCrouch       = ini.Get<std::string>("Controls", "Crouch");
        const auto& controlsConfirm      = ini.Get<std::string>("Controls", "Confirm");
        const auto& controlsCancel       = ini.Get<std::string>("Controls", "Cancel");
        const auto& controlsCodec        = ini.Get<std::string>("Controls", "Codec");
        const auto& controlsSelectItem   = ini.Get<std::string>("Controls", "SelectItem");
        const auto& controlsSelectWeapon = ini.Get<std::string>("Controls", "SelectWeapon");

        const auto& mgs2sosEnable      = ini.Get<bool>("mgs2sos", "Enable");
        const auto& mgs2sosSensitivity = ini.Get<float>("mgs2sos", "Sensitivity");
        const auto& mgs2sosInvertX     = ini.Get<bool>("mgs2sos", "InvertX");
        const auto& mgs2sosInvertY     = ini.Get<bool>("mgs2sos", "InvertY");

        MoveForwardKey  = SDL_GetKeyFromName(controlsMoveForward.c_str());
        MoveBackwardKey = SDL_GetKeyFromName(controlsMoveBackward.c_str());
        MoveLeftKey     = SDL_GetKeyFromName(controlsMoveLeft.c_str());
        MoveRightKey    = SDL_GetKeyFromName(controlsMoveRight.c_str());
        UseKey          = SDL_GetKeyFromName(controlsUse.c_str());
        PunchKey        = SDL_GetKeyFromName(controlsPunch.c_str());
        FireKey         = SDL_GetKeyFromName(controlsFire.c_str());
        CrouchKey       = SDL_GetKeyFromName(controlsCrouch.c_str());
        ConfirmKey      = SDL_GetKeyFromName(controlsConfirm.c_str());
        CancelKey       = SDL_GetKeyFromName(controlsCancel.c_str());
        CodecKey        = SDL_GetKeyFromName(controlsCodec.c_str());
        SelectItemKey   = SDL_GetKeyFromName(controlsSelectItem.c_str());
        SelectWeaponKey = SDL_GetKeyFromName(controlsSelectWeapon.c_str());

        enableSos   = mgs2sosEnable;
        Sensitivity = mgs2sosSensitivity;
        Invert_X    = mgs2sosInvertX;
        Invert_Y    = mgs2sosInvertY;
    }
    catch (std::runtime_error& ex) {
        char buf[256];
        sprintf(buf, "Failed to parse config file:\n%s", ex.what());

        ShowWindow(currentWindow, SW_HIDE);

        MessageBoxA(NULL, buf, "Error", MB_OK|MB_ICONERROR);

        ExitProcess(1);
    }
}

void InstallOffset(DWORD address, void* funcPtr) {
    WriteProcessMemory(GetCurrentProcess(), (LPVOID)address, &funcPtr, sizeof(funcPtr), NULL);
}

DWORD InstallHook(DWORD address, void* funcPtr, bool jump = false) {
    BYTE buf[10];
    memset(buf, 0, sizeof(buf));

    DWORD target = (DWORD)funcPtr;

    if (jump) {
        buf[0] = 0xE9;
    } else {
        buf[0] = 0xE8;
    }

    *(DWORD*)(buf + 1) = (DWORD)(target - (address + 5));

    WriteProcessMemory(GetCurrentProcess(), (LPVOID)address, buf, 5, NULL);

    return target;
}

void WriteBytes(DWORD address, uint8_t* buf, size_t size) {
    WriteProcessMemory(GetCurrentProcess(), (LPVOID)address, buf, size, NULL);
}

void InitializeSOS() {
    InstallOffset(0x00883E8D, &camera_handler);
    InstallOffset(0x00884934, &camera_handler);
    InstallOffset(0x008849B9, &camera_handler);
    InstallHook(0x00884A2E, &camera_handler);
    InstallHook(0x004E7702, &check_if_snake_is_next_to_wall_controls);
    InstallHook(0x008CF1AA, &convert_stick_to_button);
    InstallHook(0x004BE585, &check_if_snake_needs_to_jump);
    InstallOffset(0x0087D9D4, &camera_transitions_handler);
    InstallOffset(0x0099CA04, &start_paddemo);
    InstallOffset(0x004C9798, &paddemo_handler);
    InstallHook(0x004B05EF, &diff_direction_for_swimming);
    InstallHook(0x004B05F8, &diff_direction_for_swimming);
    InstallOffset(0x0099C32C, &chara_func_0xDFADB);
    InstallHook(0x004EB239, &get_camera_direction_fix_for_hanging);
    InstallHook(0x004EB242, &diff_direction_for_hanging1);
    InstallHook(0x004EB253, &diff_direction_for_hanging2);
    InstallHook(0x004C2448, &set_direction_for_PL_SubjectMove_wrap_with_set_stick_direction);
    InstallHook(0x004BA74D, &is_rightstick_used);
    InstallHook(0x004BA8EF, &is_rightstick_used);
    InstallHook(0x004BB764, &is_rightstick_used);
    InstallHook(0x004BC8B4, &is_rightstick_used);
    InstallHook(0x004BC8B4, &get_blade_direction);
    InstallHook(0x004BAD07, &get_blade_direction);
    InstallHook(0x004BAFDB, &get_blade_direction);
    InstallHook(0x004BB055, &get_blade_direction);
    InstallHook(0x004BB864, &get_blade_direction);
    InstallHook(0x004BC8DE, &get_blade_direction);
    InstallHook(0x004BA551, &check_if_special_blade_attack);
    InstallHook(0x004BBB65, &diff_direction_for_blade_protection);
    uint8_t b1[2] { 0x90, 0x90 };
    WriteBytes(0x004BBB72, b1, 2);
    uint8_t b2[4] { 0x66, 0x89, 0x46, 0x6A };
    WriteBytes(0x004BBB74, b2, 4);
    InstallHook(0x004E8B04, &raiden_event_msg_motion);
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

    LoadConfig();

    if (enableSos) {
        InitializeSOS();
    }

    std::thread sdlThread(SDLEventLoop), inputThread(UpdateInput);
    sdlThread.detach();
    inputThread.detach();
}