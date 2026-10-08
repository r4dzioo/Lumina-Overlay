#include "Overlay/App.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
    overlay::App app;
    return app.Run(instance, show_command);
}

