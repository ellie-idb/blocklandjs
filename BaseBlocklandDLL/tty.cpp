#pragma once
#include "uv8.h"

using namespace v8;

Persistent<ObjectTemplate> uv8tty;

uv8_efunc(uv8_new_tty) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_tty_set_mode) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_tty_reset_mode) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_tty_get_winsize) {
	uv8_unfinished_args();
}

/*
Handle<ObjectTemplate> uv8_bind_tty(Isolate* this_) {
Handle<ObjectTemplate> tty = ObjectTemplate::New(this_);
return tty;
}
*/

void uv8_init_tty(Isolate* this_) {
	Handle<ObjectTemplate> tty = ObjectTemplate::New(this_);
	Handle<ObjectTemplate> ttymode = ObjectTemplate::New(this_);
	ttymode->Set(this_, "reset", FunctionTemplate::New(this_, uv8_tty_reset_mode));
	ttymode->Set(this_, "set", FunctionTemplate::New(this_, uv8_tty_set_mode));
	tty->Set(this_, "mode", ttymode);
	tty->Set(this_, "get_winsize", FunctionTemplate::New(this_, uv8_tty_get_winsize));

	uv8tty.Reset(this_, tty);
}
