#pragma once
#include "uv8.h"

using namespace v8;

uv8_efunc(uv8_shutdown) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_listen) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_accept) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_read_start) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_read_stop) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_write) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_is_readable) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_is_writable) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_stream_set_blocking) {
	uv8_unfinished_args();
}

Handle<ObjectTemplate> uv8_bind_stream(Isolate* this_) {
	Handle<ObjectTemplate> stream = ObjectTemplate::New(this_);
	stream->Set(this_, "shutdown", FunctionTemplate::New(this_, uv8_shutdown));
	stream->Set(this_, "listen", FunctionTemplate::New(this_, uv8_listen));
	stream->Set(this_, "accept", FunctionTemplate::New(this_, uv8_accept));
	stream->Set(this_, "read_start", FunctionTemplate::New(this_, uv8_read_start));
	stream->Set(this_, "read_stop", FunctionTemplate::New(this_, uv8_read_stop));
	stream->Set(this_, "write", FunctionTemplate::New(this_, uv8_write));
	stream->Set(this_, "is_readable", FunctionTemplate::New(this_, uv8_is_readable));
	stream->Set(this_, "is_writable", FunctionTemplate::New(this_, uv8_is_writable));
	stream->Set(this_, "stream_set_blocking", FunctionTemplate::New(this_, uv8_stream_set_blocking));
	return stream;
}