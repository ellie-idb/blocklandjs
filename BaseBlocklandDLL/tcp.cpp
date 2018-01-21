#pragma once
#include "uv8.h"

using namespace v8;

Persistent<ObjectTemplate> uvtcp;

uv8_efunc(uv8_new_tcp) {
	Handle<Object> nwtcp = StrongPersistentTL(uvtcp)->NewInstance();
	args.GetReturnValue().Set(nwtcp);
}

uv8_efunc(uv8_tcp_open) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_tcp_nodelay) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_tcp_simultaneous_accepts) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_tcp_bind) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_tcp_getpeername) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_tcp_getsockname) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_tcp_connect) {
	uv8_unfinished_args();
}

/*
Handle<ObjectTemplate> uv8_bind_tcp(Isolate* this_) {
	Handle<ObjectTemplate> tcp = ObjectTemplate::New(this_);
	return tcp;
}
*/

void uv8_init_tcp(Isolate* this_) {
	Handle<ObjectTemplate> tcp = ObjectTemplate::New(this_);
	tcp->Set(this_, "open", FunctionTemplate::New(this_, uv8_tcp_open));
	tcp->Set(this_, "nodelay", FunctionTemplate::New(this_, uv8_tcp_nodelay));
	tcp->Set(this_, "simultaneous_accepts", FunctionTemplate::New(this_, uv8_tcp_simultaneous_accepts));
	tcp->Set(this_, "bind", FunctionTemplate::New(this_, uv8_tcp_bind));
	tcp->Set(this_, "getpeername", FunctionTemplate::New(this_, uv8_tcp_getpeername));
	tcp->Set(this_, "getsockname", FunctionTemplate::New(this_, uv8_tcp_getsockname));
	tcp->Set(this_, "connect", FunctionTemplate::New(this_, uv8_tcp_connect));

	uvtcp.Reset(this_, tcp);
}