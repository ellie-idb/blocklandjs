#pragma once
#include "uv8.h"

void uv8_tinfl(const FunctionCallbackInfo<Value> &args) {

}

void uv8_tdefl(const FunctionCallbackInfo<Value> &args) {

}

Handle<ObjectTemplate> uv8_bind_miniz(Isolate* this_) {
	Handle<ObjectTemplate> miniz = ObjectTemplate::New(this_);
	return miniz;
}