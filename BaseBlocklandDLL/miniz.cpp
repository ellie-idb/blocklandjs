#pragma once
#include "uv8.h"

uv8_efunc(uv8_tinfl) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_tdefl) {
	uv8_unfinished_args();
}

Handle<ObjectTemplate> uv8_bind_miniz(Isolate* this_) {
	Handle<ObjectTemplate> miniz = ObjectTemplate::New(this_);
	return miniz;
}