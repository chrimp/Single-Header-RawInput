#include "../include/RawInputCapture.h"

#include <conio.h>

// An example code
// Build with CMake and run example.exe

void callback(int key, int state) {
    printf("Key: %d, State: %d\n", key, state);
}

int main() {
    bool ret = SetRawInputCapture(RIDEV_INPUTSINK);
    if (!ret) abort();
    AddCallback(callback);
    StartRawInputThread();

    printf("Raw input capture started\n");

    while (_getch() != 'q') {
        Sleep(100);
    }
}