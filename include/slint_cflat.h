/**
 * @file slint_cflat.h
 * @brief Flat C API bindings for the Slint declarative GUI framework.
 */

#ifndef SLINT_CFLAT_H
#define SLINT_CFLAT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* DLL import/export macros */
#if defined(_WIN32) && !defined(SLINT_CFLAT_STATIC)
  #if defined(SLINT_CFLAT_BUILD_DLL)
    #define SLINT_API __declspec(dllexport)
  #else
    #define SLINT_API __declspec(dllimport)
  #endif
#else
  #define SLINT_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 1. Message Identifiers (Windows-like event model)                           */
/* ========================================================================== */

#define SLINT_WM_NULL     0x0000
#define SLINT_WM_CREATE   0x0001
#define SLINT_WM_DESTROY  0x0002
#define SLINT_WM_CLOSE    0x0010
#define SLINT_WM_QUIT     0x0012
#define SLINT_WM_COMMAND  0x0111
#define SLINT_WM_USER     0x0400

/* ========================================================================== */
/* 2. Type Definitions (Must precede function prototypes)                     */
/* ========================================================================== */

/**
 * @brief Standard message packet passed between Slint and the host application.
 */
typedef struct SlintMessage {
    uint32_t  msg;     /**< Message ID. */
    uintptr_t wparam;  /**< Unsigned pointer-sized parameter. */
    intptr_t  lparam;  /**< Signed pointer-sized parameter. */
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

SLINT_API SlintContext* slint_c_create(const char *code_ptr);
SLINT_API void          slint_c_free(SlintContext *ctx_ptr);
SLINT_API bool          slint_c_is_running(SlintContext *ctx_ptr);

/* ========================================================================== */
/* 4. Message Transmission APIs                                               */
/* ========================================================================== */

SLINT_API bool          slint_c_post_message(SlintContext *ctx_ptr, uint32_t msg, uintptr_t wparam, intptr_t lparam);
SLINT_API intptr_t      slint_c_send_message(SlintContext *ctx_ptr, uint32_t msg, uintptr_t wparam, intptr_t lparam);

/* ========================================================================== */
/* 5. Event Loop & Message Polling APIs                                       */
/* ========================================================================== */

SLINT_API bool          slint_c_poll_event(SlintContext *ctx_ptr, SlintMessage *out_msg);
SLINT_API bool          slint_c_dispatch_event(SlintContext *ctx_ptr, const SlintMessage *msg);

#ifdef __cplusplus
}
#endif

#endif /* SLINT_CFLAT_H */