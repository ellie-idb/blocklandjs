#pragma once
#include "uv8.h"

using namespace v8;

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

}