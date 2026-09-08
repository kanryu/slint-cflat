use slint_interpreter::{Compiler, ComponentHandle, Value};
use std::collections::VecDeque;
use std::ffi::CStr;
use std::os::raw::{c_char, c_void};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{mpsc, Arc, Mutex};
use std::thread;

// ===========================================================================
// 1. Message Definitions & Types
// ===========================================================================

pub const SLINT_WM_NULL: u32 = 0x0000;
pub const SLINT_WM_CREATE: u32 = 0x0001;
pub const SLINT_WM_DESTROY: u32 = 0x0002;
pub const SLINT_WM_CLOSE: u32 = 0x0010;
pub const SLINT_WM_QUIT: u32 = 0x0012;
pub const SLINT_WM_COMMAND: u32 = 0x0111;
pub const SLINT_WM_USER: u32 = 0x0400;

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct SlintMessage {
    pub msg: u32,
    pub wparam: usize,
    pub lparam: isize,
}

pub type SlintCallback = extern "C" fn(*mut c_void);

pub struct SlintContext {
    pub message_queue: Arc<Mutex<VecDeque<SlintMessage>>>,
    pub is_running: Arc<AtomicBool>,
    pub ui_thread: Mutex<Option<thread::JoinHandle<()>>>,
}

// ===========================================================================
// 2. Lifecycle Management APIs
// ===========================================================================

#[unsafe(no_mangle)]
pub unsafe extern "C" fn slint_c_create(code_ptr: *const c_char) -> *mut SlintContext {
    if code_ptr.is_null() {
        return std::ptr::null_mut();
    }
    let code_str = match unsafe { CStr::from_ptr(code_ptr) }.to_str() {
        Ok(s) => s.to_string(),
        Err(_) => return std::ptr::null_mut(),
    };

    let message_queue = Arc::new(Mutex::new(VecDeque::new()));
    let is_running = Arc::new(AtomicBool::new(false));

    let queue_clone = message_queue.clone();
    let is_running_clone = is_running.clone();

    let (init_tx, init_rx) = mpsc::channel::<bool>();

    let handle = thread::spawn(move || {
        let compiler = Compiler::default();
        let result = spin_on::spin_on(
            compiler.build_from_source(code_str, Default::default())
        );

        let definition = match result.components().next() {
            Some(def) => def,
            None => {
                let _ = init_tx.send(false);
                return;
            }
        };

        let instance = match definition.create() {
            Ok(inst) => inst,
            Err(_) => {
                let _ = init_tx.send(false);
                return;
            }
        };

        let weak_inst = instance.as_weak();
        let queue_for_cb = queue_clone.clone();

        let _ = instance.set_callback("request_quit", move |_| {
            if let Some(inst) = weak_inst.upgrade() {
                let _ = inst.hide();
            }
            if let Ok(mut q) = queue_for_cb.lock() {
                q.push_back(SlintMessage {
                    msg: SLINT_WM_CLOSE,
                    wparam: 0,
                    lparam: 0,
                });
            }
            Value::Void
        });

        if instance.show().is_err() {
            let _ = init_tx.send(false);
            return;
        }

        is_running_clone.store(true, Ordering::SeqCst);
        let _ = init_tx.send(true);

        let _ = instance.run();

        is_running_clone.store(false, Ordering::SeqCst);
        if let Ok(mut q) = queue_clone.lock() {
            q.push_back(SlintMessage {
                msg: SLINT_WM_QUIT,
                wparam: 0,
                lparam: 0,
            });
        }
    });

    match init_rx.recv() {
        Ok(true) => {
            let ctx = SlintContext {
                message_queue,
                is_running,
                ui_thread: Mutex::new(Some(handle)),
            };
            Box::into_raw(Box::new(ctx))
        }
        _ => {
            let _ = handle.join();
            std::ptr::null_mut()
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn slint_c_free(ctx_ptr: *mut SlintContext) {
    if !ctx_ptr.is_null() {
        let mut ctx = unsafe { Box::from_raw(ctx_ptr) };
        ctx.is_running.store(false, Ordering::SeqCst);

        let handle = ctx.ui_thread.get_mut().ok().and_then(|opt| opt.take());
        if let Some(h) = handle {
            let _ = h.join();
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn slint_c_is_running(ctx_ptr: *mut SlintContext) -> bool {
    if ctx_ptr.is_null() {
        return false;
    }
    unsafe { (*ctx_ptr).is_running.load(Ordering::Relaxed) }
}

// ===========================================================================
// 3. Message Passing APIs
// ===========================================================================

#[unsafe(no_mangle)]
pub unsafe extern "C" fn slint_c_post_message(
    ctx_ptr: *mut SlintContext,
    msg: u32,
    wparam: usize,
    lparam: isize,
) -> bool {
    if ctx_ptr.is_null() {
        return false;
    }
    let ctx = unsafe { &*ctx_ptr };
    if let Ok(mut queue) = ctx.message_queue.lock() {
        queue.push_back(SlintMessage { msg, wparam, lparam });
        true
    } else {
        false
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn slint_c_send_message(
    ctx_ptr: *mut SlintContext,
    msg: u32,
    wparam: usize,
    lparam: isize,
) -> isize {
    if ctx_ptr.is_null() {
        return 0;
    }
    let app_msg = SlintMessage { msg, wparam, lparam };
    unsafe { slint_c_dispatch_event(ctx_ptr, &app_msg as *const SlintMessage) as isize }
}

// ===========================================================================
// 4. Event Loop Driving APIs
// ===========================================================================

#[unsafe(no_mangle)]
pub unsafe extern "C" fn slint_c_poll_event(
    ctx_ptr: *mut SlintContext,
    out_msg: *mut SlintMessage,
) -> bool {
    if ctx_ptr.is_null() {
        return false;
    }
    let ctx = unsafe { &*ctx_ptr };

    if let Ok(mut queue) = ctx.message_queue.lock() {
        if let Some(msg) = queue.pop_front() {
            if !out_msg.is_null() {
                unsafe { *out_msg = msg; }
            }
            return true;
        }
    }

    false
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn slint_c_dispatch_event(
    ctx_ptr: *mut SlintContext,
    msg: *const SlintMessage,
) -> bool {
    if ctx_ptr.is_null() || msg.is_null() {
        return false;
    }
    let ctx = unsafe { &*ctx_ptr };
    let message = unsafe { *msg };

    match message.msg {
        SLINT_WM_CLOSE => {
            unsafe { slint_c_post_message(ctx_ptr, SLINT_WM_QUIT, 0, 0); }
            true
        }
        SLINT_WM_QUIT => {
            ctx.is_running.store(false, Ordering::SeqCst);
            false
        }
        _ => true,
    }
}