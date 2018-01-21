#pragma once
#include "uv8.h"

using namespace v8;

void uv8_fs_close(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_open(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_read(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_unlink(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_write(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_mkdir(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_mkdtemp(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_rmdir(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_scandir(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_scandir_next(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_stat(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_fstat(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_lstat(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_rename(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_fsync(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_fdatasync(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_ftruncate(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_sendfile(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_access(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_chmod(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_fchmod(const FunctionCallbackInfo<Value> &args) {

}
void uv8_fs_utime(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_futime(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_link(const FunctionCallbackInfo<Value> &args) {

}
void uv8_fs_symlink(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_readlink(const FunctionCallbackInfo<Value> &args) {

}
void uv8_fs_chown(const FunctionCallbackInfo<Value> &args) {

}

void uv8_fs_fchown(const FunctionCallbackInfo<Value> &args) {

}

Handle<ObjectTemplate> uv8_bind_fs(Isolate* this_) {
	Handle<ObjectTemplate> fs = ObjectTemplate::New(this_);
	return fs;
}