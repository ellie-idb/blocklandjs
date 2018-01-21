#pragma once
#include "uv8.h"

using namespace v8;

void ThrowError(Isolate* this_, const char* error) {
	this_->ThrowException(String::NewFromUtf8(this_, error));
}

void uv8_bind_all(Isolate* this_, Handle<ObjectTemplate> globalObject) {
	Local<ObjectTemplate> libuv = ObjectTemplate::New(this_);
	libuv->Set(this_, "fs", uv8_bind_fs(this_));
	libuv->Set(this_, "handle", uv8_bind_handle(this_));
	libuv->Set(this_, "loop", uv8_bind_loop(this_));
	libuv->Set(this_, "miniz", uv8_bind_miniz(this_));
	libuv->Set(this_, "misc", uv8_bind_misc(this_));
	libuv->Set(this_, "req", uv8_bind_req(this_));
	libuv->Set(this_, "stream", uv8_bind_stream(this_));
	libuv->Set(this_, "tcp", uv8_bind_tcp(this_));
	libuv->Set(this_, "timer", uv8_bind_timer(this_));
	libuv->Set(this_, "tty", uv8_bind_tty(this_));

	globalObject->Set(this_, "uv", libuv);
}