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
#include "libplatform/libplatform.h"

using namespace v8;
/* uv.loop */
void uv8_run(const FunctionCallbackInfo<Value> &args);
void uv8_walk(const FunctionCallbackInfo<Value> &args);

/* uv.req */
void uv8_cancel(const FunctionCallbackInfo<Value> &args);

/* uv.handle */
void uv8_close(const FunctionCallbackInfo<Value> &args);

/* uv.timer */
void uv8_new_timer(const FunctionCallbackInfo<Value> &args);
void uv8_timer_start(const FunctionCallbackInfo<Value> &args);
void uv8_timer_stop(const FunctionCallbackInfo<Value> &args);
void uv8_timer_again(const FunctionCallbackInfo<Value> &args);
void uv8_timer_set_repeat(const FunctionCallbackInfo<Value> &args);
void uv8_timer_get_repeat(const FunctionCallbackInfo<Value> &args);

/* uv.stream */
void uv8_shutdown(const FunctionCallbackInfo<Value> &args);
void uv8_listen(const FunctionCallbackInfo<Value> &args);
void uv8_accept(const FunctionCallbackInfo<Value> &args);
void uv8_read_start(const FunctionCallbackInfo<Value> &args);
void uv8_read_stop(const FunctionCallbackInfo<Value> &args);
void uv8_write(const FunctionCallbackInfo<Value> &args);
void uv8_is_readable(const FunctionCallbackInfo<Value> &args);
void uv8_is_writable(const FunctionCallbackInfo<Value> &args);
void uv8_stream_set_blocking(const FunctionCallbackInfo<Value> &args);

/* uv.tcp */
void uv8_new_tcp(const FunctionCallbackInfo<Value> &args);
void uv8_tcp_open(const FunctionCallbackInfo<Value> &args);
void uv8_tcp_nodelay(const FunctionCallbackInfo<Value> &args);
void uv8_tcp_simultaneous_accepts(const FunctionCallbackInfo<Value> &args);
void uv8_tcp_bind(const FunctionCallbackInfo<Value> &args);
void uv8_tcp_getpeername(const FunctionCallbackInfo<Value> &args);
void uv8_tcp_getsockname(const FunctionCallbackInfo<Value> &args);
void uv8_tcp_connect(const FunctionCallbackInfo<Value> &args);

/* uv.tty */
void uv8_new_tty(const FunctionCallbackInfo<Value> &args);
void uv8_tty_set_mode(const FunctionCallbackInfo<Value> &args);
void uv8_tty_reset_mode(const FunctionCallbackInfo<Value> &args);
void uv8_tty_get_winsize(const FunctionCallbackInfo<Value> &args);

/* uv.fs */
void uv8_fs_close(const FunctionCallbackInfo<Value> &args);
void uv8_fs_open(const FunctionCallbackInfo<Value> &args);
void uv8_fs_read(const FunctionCallbackInfo<Value> &args);
void uv8_fs_unlink(const FunctionCallbackInfo<Value> &args);
void uv8_fs_write(const FunctionCallbackInfo<Value> &args);
void uv8_fs_mkdir(const FunctionCallbackInfo<Value> &args);
void uv8_fs_mkdtemp(const FunctionCallbackInfo<Value> &args);
void uv8_fs_rmdir(const FunctionCallbackInfo<Value> &args);
void uv8_fs_scandir(const FunctionCallbackInfo<Value> &args);
void uv8_fs_scandir_next(const FunctionCallbackInfo<Value> &args);
void uv8_fs_stat(const FunctionCallbackInfo<Value> &args);
void uv8_fs_fstat(const FunctionCallbackInfo<Value> &args);
void uv8_fs_lstat(const FunctionCallbackInfo<Value> &args);
void uv8_fs_rename(const FunctionCallbackInfo<Value> &args);
void uv8_fs_fsync(const FunctionCallbackInfo<Value> &args);
void uv8_fs_fdatasync(const FunctionCallbackInfo<Value> &args);
void uv8_fs_ftruncate(const FunctionCallbackInfo<Value> &args);
void uv8_fs_sendfile(const FunctionCallbackInfo<Value> &args);
void uv8_fs_access(const FunctionCallbackInfo<Value> &args);
void uv8_fs_chmod(const FunctionCallbackInfo<Value> &args);
void uv8_fs_fchmod(const FunctionCallbackInfo<Value> &args);
void uv8_fs_utime(const FunctionCallbackInfo<Value> &args);
void uv8_fs_futime(const FunctionCallbackInfo<Value> &args);
void uv8_fs_link(const FunctionCallbackInfo<Value> &args);
void uv8_fs_symlink(const FunctionCallbackInfo<Value> &args);
void uv8_fs_readlink(const FunctionCallbackInfo<Value> &args);
void uv8_fs_chown(const FunctionCallbackInfo<Value> &args);
void uv8_fs_fchown(const FunctionCallbackInfo<Value> &args);

/* uv.misc */
void uv8_guess_handle(const FunctionCallbackInfo<Value> &args);
void uv8_version(const FunctionCallbackInfo<Value> &args);
void uv8_version_string(const FunctionCallbackInfo<Value> &args);
void uv8_get_process_title(const FunctionCallbackInfo<Value> &args);
void uv8_set_process_title(const FunctionCallbackInfo<Value> &args);
void uv8_resident_set_memory(const FunctionCallbackInfo<Value> &args);
void uv8_uptime(const FunctionCallbackInfo<Value> &args);
void uv8_getrusage(const FunctionCallbackInfo<Value> &args);
void uv8_cpu_info(const FunctionCallbackInfo<Value> &args);
void uv8_interface_addresses(const FunctionCallbackInfo<Value> &args);
void uv8_loadavg(const FunctionCallbackInfo<Value> &args);
void uv8_exepath(const FunctionCallbackInfo<Value> &args);
void uv8_cwd(const FunctionCallbackInfo<Value> &args);
void uv8_os_homedir(const FunctionCallbackInfo<Value> &args);
void uv8_get_total_memory(const FunctionCallbackInfo<Value> &args);
void uv8_hrtime(const FunctionCallbackInfo<Value> &args);
void uv8_update_time(const FunctionCallbackInfo<Value> &args);
void uv8_now(const FunctionCallbackInfo<Value> &args);

/* uv.miniz */
void uv8_tinfl(const FunctionCallbackInfo<Value> &args);
void uv8_tdefl(const FunctionCallbackInfo<Value> &args);
