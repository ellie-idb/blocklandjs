#pragma once
#include "uv8.h"

using namespace v8;

void uv8_run(const FunctionCallbackInfo<Value> &args) {

}

void uv8_walk(const FunctionCallbackInfo<Value> &args) {

}

Handle<ObjectTemplate> uv8_bind_loop(Isolate* this_) {
	Handle<ObjectTemplate> loop = ObjectTemplate::New(this_);
	return loop;
}