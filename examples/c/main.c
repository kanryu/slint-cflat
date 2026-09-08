#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h> // For Sleep()

#include "slint_cflat.h"

int main(void) {
    // UI Markup definition: Window containing a button that calls 'request_quit()'
    const char *slint_ui_code =
        "import { Button, VerticalBox } from \"std-widgets.slint\";\n"
        "\n"
        "export component MainWindow inherits Window {\n"
        "    width: 380px;\n"
        "    height: 220px;\n"
        "    title: \"Non-blocking C Event Loop\";\n"
        "\n"
        "    callback request_quit();\n"
        "\n"
        "    VerticalBox {\n"
        "        alignment: center;\n"
        "        spacing: 16px;\n"
        "        padding: 20px;\n"
        "\n"
        "        Text {\n"
        "            text: \"Main Loop driven by C Language\";\n"
        "            font-size: 16px;\n"
        "            horizontal-alignment: center;\n"
        "        }\n"
        "\n"
        "        Button {\n"
        "            text: \"Close Window & Quit\";\n"
        "            clicked => {\n"
        "                root.request_quit();\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "}\n";

    printf("Initializing Slint UI in background...\n");
    SlintContext *ui = slint_c_create(slint_ui_code);
    if (!ui) {
        fprintf(stderr, "Error: Failed to initialize Slint context.\n");
        return 1;
    }

    printf("Starting C-side non-blocking event loop.\n");

    SlintMessage msg;
    int loop_counter = 0;

    // Main thread event loop (Win32 PeekMessage / DispatchMessage style)
    while (slint_c_is_running(ui)) {

        // 1. Poll incoming events from the queue without blocking
        while (slint_c_poll_event(ui, &msg)) {
            printf("[C Event] Message: 0x%04X (wparam: %zu, lparam: %zd)\n",
                   msg.msg, msg.wparam, msg.lparam);

            if (msg.msg == SLINT_WM_CLOSE) {
                printf("[C Event] Detected SLINT_WM_CLOSE. Initiating shutdown...\n");
            } else if (msg.msg == SLINT_WM_QUIT) {
                printf("[C Event] Detected SLINT_WM_QUIT. Exiting loop...\n");
            }

            // 2. Dispatch event to default handlers
            // Returns false when SLINT_WM_QUIT is processed to break the host loop
            if (!slint_c_dispatch_event(ui, &msg)) {
                break;
            }
        }

        // 3. Custom C logic running concurrently without being blocked by Slint
        loop_counter++;
        if (loop_counter % 100 == 0) {
            // Heartbeat output every ~1 second
            printf("[C Main Thread] Host logic running... (Tick: %d)\n", loop_counter);
        }

        // Yield CPU to prevent busy waiting (10 ms)
        Sleep(10);
    }

    printf("Cleaning up: Releasing Slint context...\n");
    slint_c_free(ui);

    printf("Application exited cleanly.\n");
    return 0;
}