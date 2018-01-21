#pragma once
#include "uv8.h"

using namespace v8;

void uv8_new_tcp(const FunctionCallbackInfo<Value> &args) {
	if (!args.IsConstructCall()) {
		ThrowError(args.GetIsolate(), "Expected to be called as a constructor");
		return;
	}
	
	
}

void uv8_tcp_open(const FunctionCallbackInfo<Value> &args) {

}

void uv8_tcp_nodelay(const FunctionCallbackInfo<Value> &args) {

}

void uv8_tcp_simultaneous_accepts(const FunctionCallbackInfo<Value> &args) {

}

void uv8_tcp_bind(const FunctionCallbackInfo<Value> &args) {

}

void uv8_tcp_getpeername(const FunctionCallbackInfo<Value> &args) {

}

void uv8_tcp_getsockname(const FunctionCallbackInfo<Value> &args) {

}

void uv8_tcp_connect(const FunctionCallbackInfo<Value> &args) {

}

Handle<ObjectTemplate> uv8_bind_tcp(Isolate* this_) {
	Handle<ObjectTemplate> tcp = ObjectTemplate::New(this_);
	return tcp;
}