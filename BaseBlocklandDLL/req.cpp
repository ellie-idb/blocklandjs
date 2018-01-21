#pragma once
#include "uv8.h"

using namespace v8;

void uv8_cancel(const FunctionCallbackInfo<Value> &args) {

}

Handle<ObjectTemplate> uv8_bind_req(Isolate* this_) {
	Handle<ObjectTemplate> req = ObjectTemplate::New(this_);
	return req;
}