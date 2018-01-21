#pragma once
#include "uv8.h"

using namespace v8;
//And if you want me to come home, baby
//That's what I'm gonna do...
Persistent<ObjectTemplate> uvfs;


uv8_efunc(uv8_fs_new) {
	Handle<Object> fsnew = StrongPersistentTL(uvfs)->NewInstance();
	args.GetReturnValue().Set(fsnew);
}

uv8_efunc(uv8_fs_close) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_open) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_read) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_unlink) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_write) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_mkdir) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_mkdtemp) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_rmdir) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_scandir) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_scandir_next) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_stat) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_fstat) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_lstat) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_rename) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_fsync) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_fdatasync) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_ftruncate) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_sendfile) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_access) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_chmod) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_fchmod) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_utime) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_futime) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_link) {
	uv8_unfinished_args();
}
uv8_efunc(uv8_fs_symlink) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_readlink) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_chown) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_fs_fchown) {
	uv8_unfinished_args();
}

void uv8_init_fs(Isolate* this_) { //Needed to setup the ObjectTemplate
	Handle<ObjectTemplate> fs = ObjectTemplate::New(this_);
	fs->Set(this_, "close", FunctionTemplate::New(this_, uv8_fs_close));
	fs->Set(this_, "open", FunctionTemplate::New(this_, uv8_fs_open));
	fs->Set(this_, "read", FunctionTemplate::New(this_, uv8_fs_read));
	fs->Set(this_, "unlink", FunctionTemplate::New(this_, uv8_fs_unlink));
	fs->Set(this_, "write", FunctionTemplate::New(this_, uv8_fs_write));
	fs->Set(this_, "mkdir", FunctionTemplate::New(this_, uv8_fs_mkdir));
	fs->Set(this_, "mkdtemp", FunctionTemplate::New(this_, uv8_fs_mkdtemp));
	fs->Set(this_, "rmdir", FunctionTemplate::New(this_, uv8_fs_rmdir));
	fs->Set(this_, "scandir", FunctionTemplate::New(this_, uv8_fs_scandir));
	fs->Set(this_, "scandir_next", FunctionTemplate::New(this_, uv8_fs_scandir_next));
	fs->Set(this_, "stat", FunctionTemplate::New(this_, uv8_fs_stat));
	fs->Set(this_, "fstat", FunctionTemplate::New(this_, uv8_fs_fstat));
	fs->Set(this_, "lstat", FunctionTemplate::New(this_, uv8_fs_lstat));
	fs->Set(this_, "rename", FunctionTemplate::New(this_, uv8_fs_rename));
	fs->Set(this_, "fsync", FunctionTemplate::New(this_, uv8_fs_fsync));
	fs->Set(this_, "fdatasync", FunctionTemplate::New(this_, uv8_fs_fdatasync));
	fs->Set(this_, "ftruncate", FunctionTemplate::New(this_, uv8_fs_ftruncate));
	fs->Set(this_, "sendfile", FunctionTemplate::New(this_, uv8_fs_sendfile));
	fs->Set(this_, "access", FunctionTemplate::New(this_, uv8_fs_access));
	fs->Set(this_, "chmod", FunctionTemplate::New(this_, uv8_fs_chmod));
	fs->Set(this_, "fchmod", FunctionTemplate::New(this_, uv8_fs_fchmod));
	fs->Set(this_, "utime", FunctionTemplate::New(this_, uv8_fs_utime));
	fs->Set(this_, "futime", FunctionTemplate::New(this_, uv8_fs_futime));
	fs->Set(this_, "link", FunctionTemplate::New(this_, uv8_fs_link));
	fs->Set(this_, "symlink", FunctionTemplate::New(this_, uv8_fs_symlink));
	fs->Set(this_, "readlink", FunctionTemplate::New(this_, uv8_fs_readlink));
	fs->Set(this_, "chown", FunctionTemplate::New(this_, uv8_fs_chown));
	fs->Set(this_, "fchown", FunctionTemplate::New(this_, uv8_fs_fchown));

	uvfs.Reset(this_, fs);
}