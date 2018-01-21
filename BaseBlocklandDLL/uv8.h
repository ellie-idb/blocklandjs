#define WIN32_LEAN_AND_MEAN
#pragma once
#include <cstdint>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <chrono>
#include "Torque.h"
#include "include/v8-inspector.h"
#include "include/v8.h"
#include "include/uv.h"
#include "libplatform/libplatform.h"

using namespace v8;

#define uv8_unfinished(cx) \
	ThrowError(cx, "Unfinished")

#define uv8_unfinished_args() \
	ThrowError(args.GetIsolate(), "Unfinished")

#define uv8_efunc(name) \
	void name(const FunctionCallbackInfo<Value> &args)

/* uv.loop */
uv8_efunc(uv8_run);
uv8_efunc(uv8_walk);

/* uv.req */
uv8_efunc(uv8_cancel);

/* uv.handle */
uv8_efunc(uv8_close);

/* uv.timer */
uv8_efunc(uv8_new_timer);
uv8_efunc(uv8_timer_start);
uv8_efunc(uv8_timer_stop);
uv8_efunc(uv8_timer_again);
uv8_efunc(uv8_timer_get_repeat);
uv8_efunc(uv8_timer_set_repeat);

/* uv.stream */
uv8_efunc(uv8_shutdown);
uv8_efunc(uv8_listen);
uv8_efunc(uv8_accept);
uv8_efunc(uv8_read_start);
uv8_efunc(uv8_read_stop);
uv8_efunc(uv8_write);
uv8_efunc(uv8_is_readable);
uv8_efunc(uv8_is_writable);
uv8_efunc(uv8_stream_set_blocking);

/* uv.tcp */
uv8_efunc(uv8_new_tcp);
uv8_efunc(uv8_tcp_open);
uv8_efunc(uv8_tcp_nodelay);
uv8_efunc(uv8_tcp_simultaneous_accepts);
uv8_efunc(uv8_tcp_bind);
uv8_efunc(uv8_tcp_getpeername);
uv8_efunc(uv8_tcp_getsockname);
uv8_efunc(uv8_tcp_connect);

/* uv.tty */
uv8_efunc(uv8_new_tty);
uv8_efunc(uv8_tty_set_mode);
uv8_efunc(uv8_tty_reset_mode);
uv8_efunc(uv8_tty_get_winsize);

/* uv.fs */
uv8_efunc(uv8_fs_new);
uv8_efunc(uv8_fs_close);
uv8_efunc(uv8_fs_open);
uv8_efunc(uv8_fs_read);
uv8_efunc(uv8_fs_unlink);
uv8_efunc(uv8_fs_write);
uv8_efunc(uv8_fs_mkdir);
uv8_efunc(uv8_fs_mkdtemp);
uv8_efunc(uv8_fs_rmdir);
uv8_efunc(uv8_fs_scandir);
uv8_efunc(uv8_fs_scandir_next);
uv8_efunc(uv8_fs_stat);
uv8_efunc(uv8_fs_fstat);
uv8_efunc(uv8_fs_lstat);
uv8_efunc(uv8_fs_rename);
uv8_efunc(uv8_fs_fsync);
uv8_efunc(uv8_fs_fdatasync);
uv8_efunc(uv8_fs_ftruncate);
uv8_efunc(uv8_fs_sendfile);
uv8_efunc(uv8_fs_access);
uv8_efunc(uv8_fs_chmod);
uv8_efunc(uv8_fs_fchmod);
uv8_efunc(uv8_fs_utime);
uv8_efunc(uv8_fs_futime);
uv8_efunc(uv8_fs_link);
uv8_efunc(uv8_fs_symlink);
uv8_efunc(uv8_fs_readlink);
uv8_efunc(uv8_fs_chown);
uv8_efunc(uv8_fs_fchown);

/* uv.misc */
uv8_efunc(uv8_guess_handle);
uv8_efunc(uv8_version);
uv8_efunc(uv8_version_string);
uv8_efunc(uv8_get_process_title);
uv8_efunc(uv8_set_process_title);
uv8_efunc(uv8_resident_set_memory);
uv8_efunc(uv8_uptime);
uv8_efunc(uv8_getrusage);
uv8_efunc(uv8_cpu_info);
uv8_efunc(uv8_interface_addresses);
uv8_efunc(uv8_loadavg);
uv8_efunc(uv8_exepath);
uv8_efunc(uv8_cwd);
uv8_efunc(uv8_os_homedir);
uv8_efunc(uv8_chdir);
uv8_efunc(uv8_get_total_memory);
uv8_efunc(uv8_hrtime);
uv8_efunc(uv8_update_time);
uv8_efunc(uv8_now);

/* uv.miniz */
uv8_efunc(uv8_tinfl);
uv8_efunc(uv8_tdefl);

/* utils */
void ThrowError(Isolate* this_, const char* error);

/* binding */
Handle<ObjectTemplate> uv8_bind_misc(Isolate* this_);
//Handle<ObjectTemplate> uv8_bind_tcp(Isolate* this_);
void uv8_init_tcp(Isolate* this_);
Handle<ObjectTemplate> uv8_bind_loop(Isolate* this_);
//Handle<ObjectTemplate> uv8_bind_fs(Isolate* this_);
void uv8_init_fs(Isolate* this_);
Handle<ObjectTemplate> uv8_bind_handle(Isolate* this_);
Handle<ObjectTemplate> uv8_bind_miniz(Isolate* this_);
Handle<ObjectTemplate> uv8_bind_req(Isolate* this_);
Handle<ObjectTemplate> uv8_bind_stream(Isolate* this_);
void uv8_init_timer(Isolate* this_);
//Handle<ObjectTemplate> uv8_bind_timer(Isolate* this_);
Handle<ObjectTemplate> uv8_bind_tty(Isolate* this_);
void uv8_bind_all(Isolate* this_, Handle<ObjectTemplate> globalObject);

template <class TypeName>
inline v8::Local<TypeName> StrongPersistentTL(
	const v8::Persistent<TypeName>& persistent);

const char* ToCString(const v8::String::Utf8Value& value);
Handle<Array> convert64To32(Isolate* iso, uint64_t in);