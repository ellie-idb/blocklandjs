#pragma once
#include "uv8.h"

using namespace v8;

uv8_efunc(uv8_run) {
	uv_thread_t thread;
	uv_thread_create(&thread, Watcher_run, (void*)running);
	//uv8_unfinished_args();
}

uv8_efunc(uv8_walk) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_loop_running) {
	args.GetReturnValue().Set(Boolean::New(args.GetIsolate(), *running));
}

Handle<ObjectTemplate> uv8_bind_loop(Isolate* this_) {
	Handle<ObjectTemplate> loop = ObjectTemplate::New(this_);
	loop->Set(this_, "run", FunctionTemplate::New(this_, uv8_run));
	loop->Set(this_, "isRunning", FunctionTemplate::New(this_, uv8_loop_running));
	//loop->Set(this_, "walk", FunctionTemplate::New(this_, uv8_walk));

	return loop;
}