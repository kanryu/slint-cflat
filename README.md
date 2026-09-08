# slint-cflat

**slint-cflat** is an unofficial flat C-API binding for the [Slint](https://slint.dev/) declarative GUI framework, providing a native C/C++ interface that has long been missing from the official Slint ecosystem.

With this lightweight and dependency-free flat API, developers can directly invoke the dynamic Slint Interpreter from standard C or C++, compile `.slint` UI markup on the fly, and display reactive, modern graphical interfaces in real time.

---

## Key Features

* **Flat C-ABI**: Pure `extern "C"` functions and Win32-style data structures (`SlintMessage`). Fully compatible with C99, C++, and any language capable of FFI/dynamic linking.
* **Dynamic Markup Loading**: Pass raw Slint markup strings at runtime without requiring ahead-of-time code generation toolchains.
* **Interactive Event Handling**: Register callbacks to intercept UI events, button clicks, and property notifications.
* **Non-Blocking Host Event Loop**: Rather than yielding total main-thread control to Slint's monolithic blocking loop, `slint-cflat` provides message polling (`slint_c_poll_event`) and dispatching APIs. This allows your C main thread to drive background computations, network operations, or access OS resources requiring main-thread execution concurrently while maintaining a smooth GUI.

---

## Quick Example (C99)

```c
#include <stdio.h>
#include <windows.h>
#include "slint_cflat.h"

int main(void) {
    const char *ui_code =
        "import { Button, VerticalBox } from \"std-widgets.slint\";\n"
        "export component MainWindow inherits Window {\n"
        "    width: 320px;\n"
        "    height: 180px;\n"
        "    title: \"slint-cflat Demo\";\n"
        "    callback request_quit();\n"
        "    VerticalBox {\n"
        "        alignment: center;\n"
        "        Text { text: \"Driven by C Main Loop\"; horizontal-alignment: center; }\n"
        "        Button { text: \"Quit\"; clicked => { root.request_quit(); } }\n"
        "    }\n"
        "}\n";

    SlintContext *ui = slint_c_create(ui_code);
    if (!ui) return 1;

    SlintMessage msg;
    while (slint_c_is_running(ui)) {
        // Poll and process UI messages without blocking C logic
        while (slint_c_poll_event(ui, &msg)) {
            if (!slint_c_dispatch_event(ui, &msg)) {
                break;
            }
        }

        // Execute concurrent C tasks here
        Sleep(10);
    }

    slint_c_free(ui);
    return 0;
}

```

---

## Architecture

`slint-cflat` spins up an isolated background thread dedicated to the Slint rendering pipeline, decoupling the UI loop from your host application logic:

```text
[ C Application Main Thread ]         [ Slint Isolated UI Thread ]
            │                                      │
            ├────── slint_c_create() ─────────────►│  (Compiles & shows window)
            │                                      │
  (PeekMessage Loop)                               │
            ├─── slint_c_poll_event() ◄────────────┤  (Pushes WM_CLOSE / Events)
            ├─── slint_c_dispatch_event()          │
            │                                      │
   [Custom Host Logic]                             │
            │                                      │
            └────── slint_c_free() ───────────────►│  (Joins thread & cleans up)

```

---

## License

For the purposes of GitHub repository distribution, this project is licensed under **GNU General Public License v3.0 (GPLv3)**.

In practice, this thin binding layer is intended to be treated functionally as an extension or part of Slint itself. You are free to use, link, and redistribute this wrapper under any licensing terms that you have agreed to with Slint (including the Slint Commercial License or alternative proprietary terms).