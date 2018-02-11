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

#define uv8_prepare_cb() \
		uv8_handle* fuck = (uv8_handle*)handle->data; \
		Locker locker(fuck->iso); \
		Isolate::Scope(fuck->iso); \
		fuck->iso->Enter(); \
		HandleScope handle_scope(fuck->iso); \
		v8::Context::Scope cScope(StrongPersistentTL(_Context)); \
		StrongPersistentTL(_Context)->Enter(); \
		Handle<Object> goddammit = StrongPersistentTL(fuck->ref); 

#define uv8_cleanup_cb() \
	StrongPersistentTL(_Context)->Exit(); \
	fuck->iso->Exit(); \
	Unlocker unlocker(fuck->iso);

#define uv8_unfinished(cx) \
	ThrowError(cx, "Unfinished")

#define uv8_unfinished_args() \
	ThrowError(args.GetIsolate(), "Unfinished")

#define uv8_efunc(name) \
	void name(const FunctionCallbackInfo<Value> &args)

#define ThrowOnUV(inarg) \
	if(inarg < 0) { \
		ThrowError(args.GetIsolate(), uv_strerror(inarg)); \
		return; \
	} 

#define ThrowArgsNotVal(amt) \
	if(args.Length() != amt) { \
		ThrowError(args.GetIsolate(), "Wrong amount of arguments."); \
		return; \
	} 

#define ThrowBadArg() \
	ThrowError(args.GetIsolate(), "Wrong type of arguments."); \
	return;

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
uv8_efunc(uv8_new_stream);
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

uv8_efunc(uv8_new_process);


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

/* uv.pipe */
uv8_efunc(uv8_new_pipe);
uv8_efunc(uv8_pipe_bind);
uv8_efunc(uv8_pipe_connect);
uv8_efunc(uv8_pipe_getsockname);
uv8_efunc(uv8_pipe_getpeername);
uv8_efunc(uv8_pipe_pending_instances);
uv8_efunc(uv8_pipe_pending_count);
uv8_efunc(uv8_pipe_pending_type);
uv8_efunc(uv8_pipe_chmod);
uv8_efunc(uv8_pipe_open);

/* utils */
void ThrowError(Isolate* this_, const char* error);
void uv8_c_connection_cb(uv_stream_t* handle, int status);
void uv8_c_connect_cb(uv_connect_t* req, int status);
void* uv8_malloc(Isolate* this_, size_t size);
void uv8_free(Isolate* this_, void* ptr);
/* binding */
Handle<ObjectTemplate> uv8_bind_misc(Isolate* this_);
Handle<ObjectTemplate> uv8_bind_fs(Isolate* this_);
//Handle<ObjectTemplate> uv8_bind_tcp(Isolate* this_);
void uv8_init_pipe(Isolate* this_, Handle<FunctionTemplate> constructor, Handle<FunctionTemplate> parent);
void uv8_init_tcp(Isolate* this_, Handle<FunctionTemplate> constructor, Handle<FunctionTemplate> parent);
Handle<ObjectTemplate> uv8_bind_loop(Isolate* this_);
//Handle<ObjectTemplate> uv8_bind_fs(Isolate* this_);
void uv8_init_fs(Isolate* this_, Handle<FunctionTemplate> constructor);
Handle<ObjectTemplate> uv8_bind_handle(Isolate* this_);
Handle<ObjectTemplate> uv8_bind_miniz(Isolate* this_);
Handle<ObjectTemplate> uv8_bind_req(Isolate* this_);
void uv8_init_timer(Isolate* this_, Handle<FunctionTemplate> constructor);
enum CheckFileOptions {
	LEAVE_OPEN_AFTER_CHECK,
	CLOSE_AFTER_CHECK
};
Maybe<uv_file> CheckFile(std::string search,
	CheckFileOptions opt);
void uv8_init_stream(Isolate* this_, Handle<FunctionTemplate> constructor);
void uv8_init_process(Isolate* this_, Handle<FunctionTemplate> constructor);
Handle<ObjectTemplate> uv8_get_stream(Isolate* this_);

struct uv8_handle {
	Isolate* iso;
	Persistent<Object> ref;
	Persistent<Function> onReadCB;
	Persistent<Function> onConnectionCB;
	Persistent<Function> onWriteCB;
	Persistent<Function> onConnectCB;
	Persistent<Function> onCloseCB;
};

extern bool* running;
void Watcher_run(void* arg);
struct uv8_cb_handle {
	Isolate* iso;
	Persistent<Function> ref;
	int argc;
	void* datum;
	size_t datum_size;
};

extern Persistent<Context> _Context;
uv_stream_t* get_stream(const FunctionCallbackInfo<Value> &args);
uv_stream_t* get_stream_from_ret(const FunctionCallbackInfo<Value> &args);

void uv8_gc_cb(const v8::WeakCallbackInfo<uv_stream_t**> &data);
void uv8_c_connect_cb(uv_connect_t* req, int status);
//Handle<ObjectTemplate> uv8_bind_timer(Isolate* this_);
Handle<ObjectTemplate> uv8_bind_tty(Isolate* this_);
void uv8_init_tty(Isolate* this_, Handle<FunctionTemplate> constructor, Handle<FunctionTemplate> parent);

void uv8_bind_all(Isolate* this_, Handle<ObjectTemplate> globalObject);

template <class TypeName>
inline v8::Local<TypeName> StrongPersistentTL(
	const v8::Persistent<TypeName>& persistent)
{
	return *reinterpret_cast<v8::Local<TypeName>*>(
		const_cast<v8::Persistent<TypeName>*>(&persistent));
}

const char* ToCString(const v8::String::Utf8Value& value);
Handle<Array> convert64To32(Isolate* iso, uint64_t in);