#include "tools/InputSimulator.h"

#include <windows.h>

#include "gui/Overlay.h"

namespace tools {

namespace {

void SendKeyEvent(int vk, bool down) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = static_cast<WORD>(vk);
    input.ki.wScan = static_cast<WORD>(MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC));
    input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

}  // namespace

void InputSimulator::FocusGameWindow() {
    HWND window = gui::GetGameWindow();
    if (window) {
        SetForegroundWindow(window);
    }
}

void InputSimulator::KeyDown(int vk) { SendKeyEvent(vk, true); }
void InputSimulator::KeyUp(int vk) { SendKeyEvent(vk, false); }

}  // namespace tools
