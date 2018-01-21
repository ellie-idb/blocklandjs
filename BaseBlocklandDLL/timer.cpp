#pragma once
#include "uv8.h"

using namespace v8;

void uv8_new_timer(const FunctionCallbackInfo<Value> &args) {

}

void uv8_timer_start(const FunctionCallbackInfo<Value> &args) {

}

void uv8_timer_stop(const FunctionCallbackInfo<Value> &args) {

}

void uv8_timer_again(const FunctionCallbackInfo<Value> &args) {

}

void uv8_timer_set_repeat(const FunctionCallbackInfo<Value> &args) {

}

void uv8_timer_get_repeat(const FunctionCallbackInfo<Value> &args) {

}

Handle<ObjectTemplate> uv8_bind_timer(Isolate* this_) {
	Handle<ObjectTemplate> timer = ObjectTemplate::New(this_);
	return timer;
}