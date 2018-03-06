#pragma once
#include "uv8.h"

using namespace v8;

uv8_efunc(uv8_cancel) {
	uv8_unfinished_args();
}

Handle<ObjectTemplate> uv8_bind_req(Isolate* this_) {
	Handle<ObjectTemplate> req = ObjectTemplate::New(this_);
	return req;
}