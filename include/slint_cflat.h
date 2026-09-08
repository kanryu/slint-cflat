/**
 * @file slint_cflat.h
 * @brief Flat C API bindings for the Slint declarative GUI toolkit.
 *
 * This header provides an unmanaged, flat C-ABI interface for initializing Slint,
 * compiling markup at runtime, and driving the UI event loop via an asynchronous
 * or synchronous message-passing architecture inspired by Windows message loops.
 */

#ifndef SLINT_CFLAT_H
#define SLINT_CFLAT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 1. Message Identifiers (Windows-like event model)                           */
/* ========================================================================== */

/** No-operation message. Can be used for synchronization pings. */
#define SLINT_WM_NULL     0x0000

/** Dispatched when a window or component creation lifecycle begins. */
#define SLINT_WM_CREATE   0x0001

/** Dispatched when a window or component is being destroyed. */
#define SLINT_WM_DESTROY  0x0002

/**
 * Dispatched when a close request is triggered (e.g., window 'X' button clicked
 * or explicit Slint request_quit invocation).
 */
#define SLINT_WM_CLOSE    0x0010

/**
 * Termination signal. When received, the host polling loop must cleanly exit.
 */
#define SLINT_WM_QUIT     0x0012

/** Dispatched upon general command/action events (e.g., button clicks). */
#define SLINT_WM_COMMAND  0x0111

/** Beginning of user-defined private messages (0x0400 and above). */
#define SLINT_WM_USER     0x0400

/* ========================================================================== */
/* 2. Type Definitions                                                        */
/* ========================================================================== */

/**
 * @brief Standard message packet passed between Slint and the host application.
 *
 * Designed with fixed memory layout matching pointer sizes of target platforms.
 */
typedef struct SlintMessage {
    uint32_t  msg;     /**< Message ID (e.g., SLINT_WM_CLOSE, SLINT_WM_QUIT, or user custom IDs). */
    uintptr_t wparam;  /**< Unsigned pointer-sized parameter (word parameter). */
    intptr_t  lparam;  /**< Signed pointer-sized parameter (long parameter). */
} SlintMessage;

/**
 * @brief Generic function pointer callback signature.
 */
typedef void (*SlintCallback)(void *user_data);

/**
 * @brief Opaque application context handle representing an active Slint instance.
 */
typedef struct SlintContext SlintContext;

/* ========================================================================== */
/* 3. Lifecycle Management APIs                                               */
/* ========================================================================== */

/**
 * @brief Compiles Slint source code and launches the UI loop on a dedicated thread.
 *
 * This function parses the provided `.slint` markup, constructs the UI components,
 * makes the primary window visible, and launches an isolated Slint event loop.
 * It blocks the caller until initial compilation and window presentation succeed.
 *
 * @param code_ptr A null-terminated UTF-8 string containing raw `.slint` markup code.
 * @return A valid pointer to SlintContext on success, or NULL if compilation/presentation failed.
 *
 * @note Thread-Safety: Safe to call from the main application thread.
 */
SlintContext* slint_c_create(const char *code_ptr);

/**
 * @brief Gracefully terminates the UI thread and releases all context resources.
 *
 * Signals the Slint loop to abort if still running, joins the background UI thread,
 * and deallocates the context memory.
 *
 * @param ctx_ptr Pointer to an active SlintContext. Safe to pass NULL (no-op).
 */
void slint_c_free(SlintContext *ctx_ptr);

/**
 * @brief Queries whether the Slint window is still open and running.
 *
 * @param ctx_ptr Pointer to the SlintContext.
 * @return true if the UI loop is active; false if closed, terminated, or if ctx_ptr is NULL.
 */
bool slint_c_is_running(SlintContext *ctx_ptr);

/* ========================================================================== */
/* 4. Message Transmission APIs                                               */
/* ========================================================================== */

/**
 * @brief Asynchronously posts a message into the event queue without blocking.
 *
 * Analogous to the Windows `PostMessage` API. Appends the event into the inter-thread
 * message FIFO and returns immediately.
 *
 * @param ctx_ptr Pointer to the SlintContext.
 * @param msg     Message identifier.
 * @param wparam  Unsigned metadata parameter.
 * @param lparam  Signed metadata parameter.
 * @return true if successfully queued; false if ctx_ptr is NULL or queue lock failed.
 */
bool slint_c_post_message(SlintContext *ctx_ptr, uint32_t msg, uintptr_t wparam, intptr_t lparam);

/**
 * @brief Synchronously dispatches a message to the event pipeline.
 *
 * Analogous to the Windows `SendMessage` API. Immediately processes the message
 * and waits for completion.
 *
 * @param ctx_ptr Pointer to the SlintContext.
 * @param msg     Message identifier.
 * @param wparam  Unsigned metadata parameter.
 * @param lparam  Signed metadata parameter.
 * @return The result of the dispatch invocation (normally 1 for success, 0 on error/exit).
 */
intptr_t slint_c_send_message(SlintContext *ctx_ptr, uint32_t msg, uintptr_t wparam, intptr_t lparam);

/* ========================================================================== */
/* 5. Event Loop & Message Polling APIs                                       */
/* ========================================================================== */

/**
 * @brief Non-blocking event poll from the context message queue.
 *
 * Analogous to the Windows `PeekMessage` API with `PM_REMOVE`. Inspects the
 * internal FIFO queue and pops the next message if available.
 *
 * @param ctx_ptr Pointer to the SlintContext.
 * @param out_msg Destination pointer where the popped SlintMessage will be stored.
 * @return true if a message was retrieved; false if the queue is empty or ctx_ptr is NULL.
 */
bool slint_c_poll_event(SlintContext *ctx_ptr, SlintMessage *out_msg);

/**
 * @brief Dispatches a message to default lifecycle handlers.
 *
 * Analogous to the Windows `DispatchMessage` API. Evaluates standard system
 * messages (such as `SLINT_WM_CLOSE` and `SLINT_WM_QUIT`) and updates state flags.
 *
 * @param ctx_ptr Pointer to the SlintContext.
 * @param msg     Pointer to the SlintMessage to be evaluated.
 * @return true to indicate the host event loop should continue;
 *         false when `SLINT_WM_QUIT` was handled, signaling that the host loop must terminate.
 */
bool slint_c_dispatch_event(SlintContext *ctx_ptr, const SlintMessage *msg);

#ifdef __cplusplus
}
#endif

#endif /* SLINT_CFLAT_H */
