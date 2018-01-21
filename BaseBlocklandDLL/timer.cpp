#pragma once
#include "uv8.h"

using namespace v8;

Persistent<ObjectTemplate> uvtimer;

uv8_efunc(uv8_new_timer) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_timer_start) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_timer_stop) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_timer_again) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_timer_set_repeat) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_timer_get_repeat) {
	uv8_unfinished_args();
}

/*
Handle<ObjectTemplate> uv8_bind_timer(Isolate* this_) {
	Handle<ObjectTemplate> timer = ObjectTemplate::New(this_);
	return timer;
}
*/
void uv8_init_timer(Isolate* this_) {
	Handle<ObjectTemplate> timer = ObjectTemplate::New(this_);
	timer->Set(this_, "start", FunctionTemplate::New(this_, uv8_timer_start));
	timer->Set(this_, "stop", FunctionTemplate::New(this_, uv8_timer_stop));
	timer->Set(this_, "again", FunctionTemplate::New(this_, uv8_timer_again));
	timer->Set(this_, "set_repeat", FunctionTemplate::New(this_, uv8_timer_set_repeat));
	timer->Set(this_, "get_repeat", FunctionTemplate::New(this_, uv8_timer_get_repeat));

	uvtimer.Reset(this_, timer);
}