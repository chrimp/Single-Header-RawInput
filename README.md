# Single Header RawInput

## Overview
This is a single-header file built with C and Win32 for easy implementation of keyboard raw input listener. Can be used in whatever forms of application (even for console applications) as long as it's built for Windows.

## What's this for?
Typically I use this to globally listen for keypress events in Windows. Normally one would need a valid Win32 window to register raw input device, but with this you can use raw input even with console applications without having to find HWND of the console window.

However I would not recommend using this for fast response on keypress events. In the loop this code uses GetMessage instead of PeekMessage, which saves CPU time a lot but also said to be a bit slower on events.

## How to use
```C
#include "RawInputCapture.h"

if (!SetRawInputCapture(0)) { // Check below for details, but basically it's RAWINPUTDEVICE's dwFlags.
    // Failed to allocate memory for callback array -- It's 128 bytes.
}

// You can add callback to receive key events.
// Should be void function, and should take two int arguments as well:
// first for key and second for event type (pressed, depressed, etc.)
Addcallback(callback); // Will not be added if the function is already listed as callback

// This library runs in a background thread, so you should start it.
StartRawInputThread(); // Automatically reallocate memory if necessary

// Also you can stop it whenever you need
StopRawInputThread();

// Remove callbacks anytime you want
RemoveCallback(callback); // Also will automatically free memory when array size is excessive
```

So there are five exposed functions:
- **SetRawInputCapture(DWORD flags)**  
    Pass a valid dwFlags. Refer to [RAWINPUTDEVICE](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawinputdevice#members) for all values. For listening keypress events regardless of the focus, use **RIDEV_INPUTSINK**.
    Note that usUsagePage is 0x01 and usUsage is 0x06 (for keyboards) so some of the values are not available. 

    Also allocates initial 16-long array for callbacks, it's 128 bytes.
- **StartRawInputThread()**  
    Starts the background loop thread for handling raw input. Does nothing if the thread is already running.
- **StopRawInputThread()**  
    Stops the thread. Also does nothing if the thread is not running.
- **AddCallback(void(\*callback)(int, int))**  
    Pass a function that matches void (int, int) signature. Subscribed functions will be called with two int arguments: first one for key and the second one for event type. Refer to [Virtual-Key Codes](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes) and [Keyboard Input Notifications](https://learn.microsoft.com/en-us/windows/win32/inputdev/keyboard-input-notifications) for all available keys and notification types.  
    
    This does nothing if passed function is already listed as callbacks. When internal array for listing callbacks is too small, this function automatically allocates another 128 bytes (16 functions).
- **RemoveCallback(void(\*callback)(int, int))**  
    Removes passed function from callback list. Does nothing if it's not listed as callbacks. If there are more than 16 empty slots in the callback array, this function automatically frees 128 bytes of memory.

Check out the **[Example]()** for details.

## How it works
Simply this creates an invisible Win32 window, and uses it to register raw inputs. It's built purely on Win32 and C99.

## Permission issue
When there is a program getting raw inputs with administrative privilege, it's known that applications with lower privilege will not get raw input events, especially when monitoring global key events (RIDEV_INPUTSINK), if the window is out-of-focus. Since this library creates a window that is virtually unfocusable, this may not work if there are programs running with higher permissions.