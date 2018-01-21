#pragma once
#include "uv8.h"

using namespace v8;

void ThrowError(Isolate* this_, const char* error) {
	this_->ThrowException(String::NewFromUtf8(this_, error));
}

void uv8_bind_all(Isolate* this_, Handle<ObjectTemplate> globalObject) {
	Local<ObjectTemplate> libuv = ObjectTemplate::New(this_);
	//libuv->Set(this_, "fs", uv8_bind_fs(this_));
	uv8_init_fs(this_);
	uv8_init_timer(this_);
	uv8_init_tcp(this_);
	libuv->Set(this_, "fs", FunctionTemplate::New(this_, uv8_fs_new));
	libuv->Set(this_, "handle", uv8_bind_handle(this_));
	libuv->Set(this_, "loop", uv8_bind_loop(this_));
	libuv->Set(this_, "miniz", uv8_bind_miniz(this_));
	libuv->Set(this_, "misc", uv8_bind_misc(this_));
	libuv->Set(this_, "req", uv8_bind_req(this_));
	libuv->Set(this_, "stream", uv8_bind_stream(this_));
	libuv->Set(this_, "tcp", FunctionTemplate::New(this_, uv8_new_tcp));
	libuv->Set(this_, "timer", FunctionTemplate::New(this_, uv8_new_timer));
	//libuv->Set(this_, "tty", uv8_bind_tty(this_));

	globalObject->Set(this_, "uv", libuv);
}

Handle<Array> convert64To32(Isolate* iso, uint64_t in) {
	Handle<Array> conv = Array::New(iso, 2);
	uint32_t low = in;
	uint32_t high = (in >> 32);
	conv->Set(0, Integer::NewFromUnsigned(iso, high));
	conv->Set(1, Integer::NewFromUnsigned(iso, low));
	return conv;

}

const char* ToCString(const v8::String::Utf8Value& value)
{
	return *value ? *value : "<string conversion failed>";
}