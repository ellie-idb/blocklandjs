#pragma once
#include "uv8.h"

using namespace v8;

uv8_efunc(uv8_close) {
	uv8_unfinished_args();
}

Handle<ObjectTemplate> uv8_bind_handle(Isolate* this_) {
	Handle<ObjectTemplate> handle = ObjectTemplate::New(this_);
	handle->Set(this_, "close", FunctionTemplate::New(this_, uv8_close));
	return handle;
}