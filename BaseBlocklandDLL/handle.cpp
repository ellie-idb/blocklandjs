#pragma once
#include "uv8.h"

using namespace v8;

void uv8_close(const FunctionCallbackInfo<Value> &args) {

}

Handle<ObjectTemplate> uv8_bind_handle(Isolate* this_) {
	Handle<ObjectTemplate> handle = ObjectTemplate::New(this_);
	return handle;
}