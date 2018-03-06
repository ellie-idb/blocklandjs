#pragma once
#include "uv8.h"

using namespace v8;
Persistent<FunctionTemplate> uvpipe;
Persistent<FunctionTemplate> uvstream;

uv_pipe_t* uv8_get_pipe(const FunctionCallbackInfo<Value> &args) {
	Handle<External> bleh = Handle<External>::Cast(args.This()->GetPrototype()->ToObject()->GetInternalField(0));
	return *(uv_pipe_t**)bleh->Value();
}

uv8_efunc(uv8_new_pipe) {
	if (args.Length() < 1) {
		ThrowError(args.GetIsolate(), "Wrong amount of args");
		return;
	}
	if (!args[0]->IsBoolean()) {
		ThrowBadArg();
	}

	Handle<Context> ctx = args.GetIsolate()->GetCurrentContext();
	Handle<Object> cons = StrongPersistentTL(uvpipe)->InstanceTemplate()->NewInstance();
	Handle<Object> str = StrongPersistentTL(uvstream)->GetFunction()->NewInstance(ctx).ToLocalChecked();
	Handle<External> wrapped = Handle<External>::Cast(str->GetInternalField(0));
	uv_stream_t** bb = (uv_stream_t**)wrapped->Value();
	uv_stream_t* aaa = *bb;
	cons->SetPrototype(ctx, str);
	int ret = uv_pipe_init(uv_default_loop(), (uv_pipe_t*)aaa, args[0]->Int32Value());
	ThrowOnUV(ret);
	args.GetReturnValue().Set(cons);
}

uv8_efunc(uv8_pipe_bind) {
	ThrowArgsNotVal(1);
	if (!args[0]->IsString()) {
		ThrowBadArg();
	}

	uv_pipe_t* ourpipe = uv8_get_pipe(args);
	String::Utf8Value c_pipe(args[0]->ToString());
	int ret = uv_pipe_bind(ourpipe, ToCString(c_pipe));
	ThrowOnUV(ret);
}

uv8_efunc(uv8_pipe_open) {
	ThrowArgsNotVal(1);
	if (!args[0]->IsInt32() || !args[0]->IsNumber()) {
		ThrowBadArg();
	}

	uv_pipe_t* ourpipe = uv8_get_pipe(args);
	int ret = uv_pipe_open(ourpipe, args[0]->Int32Value());
	ThrowOnUV(ret);
}

uv8_efunc(uv8_pipe_connect) {
	ThrowArgsNotVal(1);
	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	uv_connect_t req;
	uv_pipe_t* ourpipe = uv8_get_pipe(args);
	String::Utf8Value c_pipe(args[0]->ToString());
	uv_pipe_connect(&req, ourpipe, ToCString(c_pipe), uv8_c_connect_cb);
}

uv8_efunc(uv8_pipe_getsockname) {
	uv_pipe_t* ourpipe = uv8_get_pipe(args);
	char* buf = (char*)uv8_malloc(args.GetIsolate(), 300);
	size_t* size = (size_t*)300;
	tryagain:
	int ret = (uv_errno_t)uv_pipe_getsockname(ourpipe, buf, size);
	if (ret == UV_ENOBUFS) {
		uv8_free(args.GetIsolate(), buf);
		buf = (char*)uv8_malloc(args.GetIsolate(), *size);
		goto tryagain;
	}

	args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), buf));
}

uv8_efunc(uv8_pipe_getpeername) {
	uv_pipe_t* ourpipe = uv8_get_pipe(args);
	char* buf = (char*)uv8_malloc(args.GetIsolate(), 300);
	size_t* size = (size_t*)300;
tryagain:
	int ret = (uv_errno_t)uv_pipe_getpeername(ourpipe, buf, size);
	if (ret == UV_ENOBUFS) {
		uv8_free(args.GetIsolate(), buf);
		buf = (char*)uv8_malloc(args.GetIsolate(), *size);
		goto tryagain;
	}

	args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), buf));
}

uv8_efunc(uv8_pipe_pending_instances) {
	ThrowArgsNotVal(1);
	if (!args[0]->IsInt32() || !args[0]->IsNumber()) {
		ThrowBadArg();
	}

	uv_pipe_t* ourpipe = uv8_get_pipe(args);
	uv_pipe_pending_instances(ourpipe, args[0]->Int32Value());
}

uv8_efunc(uv8_pipe_pending_count) {
	uv_pipe_t* ourpipe = uv8_get_pipe(args);
	args.GetReturnValue().Set(Int32::New(args.GetIsolate(), uv_pipe_pending_count(ourpipe)));
}

uv8_efunc(uv8_pipe_pending_type) {
	uv_pipe_t* ourpipe = uv8_get_pipe(args);
	args.GetReturnValue().Set(Int32::New(args.GetIsolate(), uv_pipe_pending_type(ourpipe)));
}

uv8_efunc(uv8_pipe_chmod) {
	ThrowArgsNotVal(1);
	if (!args[0]->IsInt32() || !args[0]->IsNumber()) {
		ThrowBadArg();
	}

	uv_pipe_t* ourpipe = uv8_get_pipe(args);
	int ret = uv_pipe_chmod(ourpipe, args[0]->Int32Value());
	ThrowOnUV(ret);
}

void uv8_init_pipe(Isolate* this_, Handle<FunctionTemplate> constructor, Handle<FunctionTemplate> parent) {
	Handle<ObjectTemplate> pipe = constructor->InstanceTemplate();
	pipe->Set(this_, "bind", FunctionTemplate::New(this_, uv8_pipe_bind));
	pipe->Set(this_, "connect", FunctionTemplate::New(this_, uv8_pipe_connect));
	pipe->Set(this_, "getsockname", FunctionTemplate::New(this_, uv8_pipe_getsockname));
	pipe->Set(this_, "getpeername", FunctionTemplate::New(this_, uv8_pipe_getpeername));
	pipe->Set(this_, "pending_instances", FunctionTemplate::New(this_, uv8_pipe_pending_instances));
	pipe->Set(this_, "pending_count", FunctionTemplate::New(this_, uv8_pipe_pending_count));
	pipe->Set(this_, "pending_type", FunctionTemplate::New(this_, uv8_pipe_pending_type));
	pipe->Set(this_, "chmod", FunctionTemplate::New(this_, uv8_pipe_chmod));
	pipe->Set(this_, "open", FunctionTemplate::New(this_, uv8_pipe_open));

	uvpipe.Reset(this_, constructor);
	uvstream.Reset(this_, parent);
}