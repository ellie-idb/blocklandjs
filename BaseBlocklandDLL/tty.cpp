#pragma once
#include "uv8.h"

using namespace v8;

void uv8_new_tty(const FunctionCallbackInfo<Value> &args) {

}

void uv8_tty_set_mode(const FunctionCallbackInfo<Value> &args) {

}

void uv8_tty_reset_mode(const FunctionCallbackInfo<Value> &args) {

}

void uv8_tty_get_winsize(const FunctionCallbackInfo<Value> &args) {

}

Handle<ObjectTemplate> uv8_bind_tty(Isolate* this_) {
	Handle<ObjectTemplate> tty = ObjectTemplate::New(this_);
	return tty;
}