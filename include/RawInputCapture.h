#ifndef RAWINPUTCAPTURE_H
#define RAWINPUTCAPTURE_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static void (**callbacks)(int, int) = NULL;
static unsigned long currentCallbackCount;
static unsigned long callbackArrSize;
static volatile bool running = false;
static RAWINPUTDEVICE rid;

// Forward declarations
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void processKeyInput(LPARAM lParam);

static HWND createHiddenWindow() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = L"RawInputEveryWhere";

    if (!RegisterClass(&wc)) {
        fprintf(stderr, "Failed to register class: %lu\n", GetLastError());
        return NULL;
    }

    HWND hwnd = CreateWindowEx(0, wc.lpszClassName, L"RawInputEveryWhere", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, GetModuleHandle(NULL), 0);

    if (!hwnd) {
        fprintf(stderr, "Failed to create window: %lu\n", GetLastError());
        return NULL;
    }

    return hwnd;
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INPUT:
            processKeyInput(lParam);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

static void processKeyInput(LPARAM lParam) {
    UINT dwSize = 0;
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

    LPBYTE lpb = (LPBYTE)malloc(dwSize);
    if (lpb == NULL) return;

    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
        fprintf(stderr, "GetRawInputData did not return correct size !\n");
        free(lpb);
        return;
    }

    RAWINPUT* raw = (RAWINPUT*)lpb;
    if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        int key = raw->data.keyboard.VKey;
        int state = raw->data.keyboard.Message;
        for (unsigned long i = 0; i < currentCallbackCount; i++) {
            callbacks[i](key, state);
            printf("Key: %d, State: %d\n", key, state);
        }
    }

    free(lpb);
    return;
}

static DWORD WINAPI messageLoop(LPVOID lpParam) {
    HWND hwnd = createHiddenWindow();
    if (!hwnd) {
        fprintf(stderr, "Failed to create hidden window: %lu\n", GetLastError());
        return false;
    }

	rid.hwndTarget = hwnd;
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        fprintf(stderr, "Failed to register raw input devices: %lu\n", GetLastError());
        return false;
    }

    MSG msg;
    while (running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    ExitThread(0);
}

bool SetRawInputCapture(DWORD flags) {
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x06;
    rid.dwFlags = flags;

    callbacks = (void (**)(int, int)) malloc(16 * sizeof(void(*)(int, int)));
    callbackArrSize = 16;
    currentCallbackCount = 0;

    if (callbacks == NULL) {
        fprintf(stderr, "Failed to allocate memory for callbacks\n");
        return false;
    }

    return true;
}

static HANDLE thread;

void StartRawInputThread() {
    if (running) return;
    running = true;
    thread = CreateThread(NULL, 0, messageLoop, NULL, 0, NULL);
    if (thread == NULL) {
        fprintf(stderr, "Failed to create thread: %lu\n", GetLastError());
        return;
    }
    return;
}

void StopRawInputThread() {
    if (!running) return;
    running = false;
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    PostQuitMessage(0);
}

void AddCallback(void(*callback)(int, int)) {
    // Check if callback already exists
    for (unsigned long i = 0; i < currentCallbackCount; i++) {
        if (callbacks[i] == callback) {
            return;
        }
    }

    // Add callback to list
    if (currentCallbackCount + 1 > callbackArrSize) {
        void (**temp)(int, int) = realloc(callbacks, callbackArrSize + sizeof(void*) * 16);
        if (temp == NULL) {
            fprintf(stderr, "Failed to reallocate memory for callbacks\n");
            return;
        }
        free(callbacks);
        callbacks = temp;
        callbackArrSize += 16;
    }

    callbacks[currentCallbackCount] = callback;
    currentCallbackCount++;
}

void RemoveCallback(void* callback) {
    // Remove callback from list
    for (unsigned long i = 0; i < currentCallbackCount; i++) {
        if (callbacks[i] == callback) {
            callbacks[i] = NULL;
            break;
        }
    }

    // Shift all elements to the left (Should be done only one to the left)
    for (unsigned long i = 0; i < currentCallbackCount; i++) {
        if (callbacks[i] == NULL) {
            for (unsigned long j = i; j < currentCallbackCount; j++) {
                if (callbacks[j + 1] == NULL) break;
                callbacks[j] = callbacks[j + 1];
            }
        }
    }

    currentCallbackCount--;
    
    // Shrink array if necessary
    if (callbackArrSize - currentCallbackCount > 16) {
        void (**temp)(int, int) = realloc(callbacks, callbackArrSize - sizeof(void*) * 16);
        if (temp == NULL) {
            fprintf(stderr, "Failed to reallocate memory for callbacks\n");
            return;
        }
        free(callbacks);
        callbacks = temp;
        callbackArrSize -= 16;
    }

    return;
}

#endif // RAWINPUTCAPTURE_H