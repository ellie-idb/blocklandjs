#pragma once
#include "uv8.h"

using namespace v8;

Persistent<ObjectTemplate> uv8stream;



void uv8_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = (char*)malloc(suggested_size);
	assert(buf->base);
	buf->len = suggested_size;
}

void uv8_c_connection_cb(uv_stream_t* handle, int status) {
	//Printf("Called!");
	
	uv8_handle* fuck = (uv8_handle*)handle->data;
	Locker locker(fuck->iso);
	Isolate::Scope(fuck->iso);
	fuck->iso->Enter();
	HandleScope handle_scope(fuck->iso);
	v8::Context::Scope cScope(StrongPersistentTL(_Context));
	StrongPersistentTL(_Context)->Enter();
	Handle<Object> goddammit = StrongPersistentTL(fuck->ref);
	MaybeLocal<Value> unsafe = goddammit->Get(StrongPersistentTL(_Context), String::NewFromUtf8(fuck->iso, "onConnection"));
	if (unsafe.IsEmpty()) {
		unsafe = goddammit->GetPrototype()->ToObject()->Get(StrongPersistentTL(_Context), String::NewFromUtf8(fuck->iso, "onConnection"));
	}

	if (unsafe.IsEmpty()) {
		Printf("Could not find an onConnection function twice!!");
		return;
	}

	Handle<Value> lol = unsafe.ToLocalChecked();
	Handle<Function> theActualShit = Handle<Function>::Cast(lol->ToObject(fuck->iso));
	Handle<Value> args[1];
	args[0] = Int32::New(fuck->iso, status);
	theActualShit->Call(StrongPersistentTL(_Context)->Global(), 1, args);
	StrongPersistentTL(_Context)->Exit();
	fuck->iso->Exit();
	Unlocker unlocker(fuck->iso);
}

void uv8_write_cb(uv_write_t* req, int status) {
	uv8_handle* fuck = (uv8_handle*)req->handle->data;
	Locker locker(fuck->iso);
	Isolate::Scope(fuck->iso);
	fuck->iso->Enter();
	HandleScope handle_scope(fuck->iso);
	v8::Context::Scope cScope(StrongPersistentTL(_Context));
	StrongPersistentTL(_Context)->Enter();
	Handle<Object> goddammit = StrongPersistentTL(fuck->ref);
	MaybeLocal<Value> unsafe = goddammit->Get(StrongPersistentTL(_Context), String::NewFromUtf8(fuck->iso, "onWrite"));
	if (unsafe.IsEmpty()) {
		unsafe = goddammit->GetPrototype()->ToObject()->Get(StrongPersistentTL(_Context), String::NewFromUtf8(fuck->iso, "onWrite"));
	}

	if (unsafe.IsEmpty()) {
		Printf("Could not find an onWrite function twice!!");
		return;
	}

	Handle<Value> lol = unsafe.ToLocalChecked();
	Handle<Function> theActualShit = Handle<Function>::Cast(lol->ToObject(fuck->iso));
	Handle<Value> args[1];
	args[0] = Int32::New(fuck->iso, status);
	theActualShit->Call(StrongPersistentTL(_Context)->Global(), 1, args);
	StrongPersistentTL(_Context)->Exit();
	fuck->iso->Exit();
	Unlocker unlocker(fuck->iso);
	free(req);
}

void uv8_c_read_cb(uv_stream_t* stream, ssize_t read, const uv_buf_t* buf) {
	uv8_handle* fuck = (uv8_handle*)stream->data;
	Locker locker(fuck->iso);
	Isolate::Scope(fuck->iso);
	fuck->iso->Enter();
	HandleScope handle_scope(fuck->iso);
	v8::Context::Scope cScope(StrongPersistentTL(_Context));
	StrongPersistentTL(_Context)->Enter();
	Handle<Object> goddammit = StrongPersistentTL(fuck->ref);
	MaybeLocal<Value> unsafe = goddammit->Get(StrongPersistentTL(_Context), String::NewFromUtf8(fuck->iso, "onRead"));
	if (unsafe.IsEmpty()) {
		unsafe = goddammit->GetPrototype()->ToObject()->Get(StrongPersistentTL(_Context), String::NewFromUtf8(fuck->iso, "onRead"));
	}

	if (unsafe.IsEmpty()) {
		Printf("Could not find an onRead function twice!!");
		return;
	}

	Handle<Value> lol = unsafe.ToLocalChecked();
	Handle<Function> theActualShit = Handle<Function>::Cast(lol->ToObject(fuck->iso));
	Handle<Value> args[2];
	args[0] = Int32::New(fuck->iso, read);
	if (read < 0) {
		args[1] = String::NewFromUtf8(fuck->iso, "");
	}
	else {
		args[1] = String::NewFromUtf8(fuck->iso, buf->base);
	}

	if (read == UV_EOF) {
		args[0] = Int32::New(fuck->iso, UV_EOF);
		args[1] = v8::Undefined(fuck->iso);
	}
	theActualShit->Call(StrongPersistentTL(_Context)->Global(), 2, args);
	StrongPersistentTL(_Context)->Exit();
	fuck->iso->Exit();
	Unlocker unlocker(fuck->iso);
	free((void*)buf);
}

void uv8_c_shutdown_cb(uv_shutdown_t* req, int status) {
	//It's done. Shut it down;
	uv_stream_t* this_ = req->handle;
	if (this_->type == uv_handle_type::UV_TCP) {
		//Yooooo
		uv8_handle* hand = (uv8_handle*)this_->data;
		//hand->ref.Get(hand->iso) = v8::Undefined(hand->iso)->ToObject(hand->iso);
		hand->iso->AdjustAmountOfExternalAllocatedMemory(-(int64_t)sizeof(*hand));
		hand->iso->AdjustAmountOfExternalAllocatedMemory(-(int64_t)sizeof(*this_));
		free(hand);
	}
	//then free tcp.
	free(this_);
	//then the request
	free(req);
}

uv8_efunc(uv8_new_stream) {
	Handle<Object> stream = StrongPersistentTL(uv8stream)->NewInstance();
	args.GetReturnValue().Set(stream);
	//uv8_unfinished_args();
}

uv8_efunc(uv8_shutdown) {
	uv_stream_t* this_ = get_stream(args);
	uv_shutdown_t* req = (uv_shutdown_t*)malloc(sizeof(*req));
	uv_shutdown(req, this_, uv8_c_shutdown_cb);
	//uv8_unfinished_args();
}

uv8_efunc(uv8_listen) {
	
	if (args.Length() != 1) {
		ThrowError(args.GetIsolate(), "Wrong number of arguments given");
		return;
	}

	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowError(args.GetIsolate(), "Wrong type of argument..");
		return;
	}
	uv_stream_t* bleh = get_stream(args);
	int ret = uv_listen(bleh, args[0]->ToInteger()->Int32Value(), uv8_c_connection_cb);
	if (ret < 0) {
		ThrowError(args.GetIsolate(), uv_strerror(ret));
	}
	//uv8_unfinished_args();
}
 
uv8_efunc(uv8_accept) {
	uv_stream_t* bleh = get_stream(args);
	uv_stream_t* client = nullptr;
	if (bleh->type == uv_handle_type::UV_TCP) {
		uv8_new_tcp(args);
		client = get_stream_from_ret(args);
	}

	int ret = uv_accept(bleh, client);
	if (ret < 0) {
		ThrowError(args.GetIsolate(), "Accepting connection failed.");
		return;
	}
	//uv_accept()
}

uv8_efunc(uv8_read_start) {
	uv_stream_t* bleh = get_stream(args); //code reuse
	int ret = uv_read_start(bleh, uv8_alloc_cb, uv8_c_read_cb);
	if (ret < 0) {
		ThrowError(args.GetIsolate(), uv_strerror(ret));
	}
	//uv8_unfinished_args();
}

uv8_efunc(uv8_read_stop) {
	uv_stream_t* bleh = get_stream(args);
	int ret = uv_read_stop(bleh);
	if (ret < 0) {
		ThrowError(args.GetIsolate(), uv_strerror(ret));
	}
	//uv8_unfinished_args();
}

uv8_efunc(uv8_write) {
	uv_stream_t* bleh = get_stream(args);
	uv_buf_t buf;
	uv_write_t* req;

	buf.base = (char*)ToCString(String::Utf8Value(args[0]->ToString()));
	req = (uv_write_t*)malloc(sizeof(*req));
	args.GetIsolate()->AdjustAmountOfExternalAllocatedMemory(sizeof(*req));
	int retn = uv_write(req, bleh, &buf, 1, uv8_write_cb);
	if (retn < 0) {
		ThrowError(args.GetIsolate(), uv_strerror(retn));
		return;
	}
	//uv8_unfinished_args();
}

uv8_efunc(uv8_is_readable) {
	uv_stream_t* this_ = get_stream(args);
	args.GetReturnValue().Set(Boolean::New(args.GetIsolate(), uv_is_readable(this_)));
	//uv8_unfinished_args();
}

uv8_efunc(uv8_is_writable) {
	uv_stream_t* this_ = get_stream(args);
	args.GetReturnValue().Set(Boolean::New(args.GetIsolate(), uv_is_writable(this_)));
	//uv8_unfinished_args();
}

uv8_efunc(uv8_stream_set_blocking) {
	if (args.Length() != 1) {
		ThrowError(args.GetIsolate(), "Wrong number of arguments");
		return;
	}

	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowError(args.GetIsolate(), "Wrong type of arguments.");
		return;
	}

	uv_stream_t* this_ = get_stream(args);
	int ret = uv_stream_set_blocking(this_, args[0]->ToInteger()->Value());
	if (ret < 0) {
		ThrowError(args.GetIsolate(), uv_strerror(ret));
	}
	//uv8_unfinished_args();
}

uv8_efunc(uv8_stream_onread) {

}

uv8_efunc(uv8_stream_onwrite) {

}

uv8_efunc(uv8_stream_onconnect) {

}

uv8_efunc(uv8_stream_onshutdown) {

}

uv8_efunc(uv8_stream_onconnection) {

}

Handle<ObjectTemplate> uv8_get_stream(Isolate* this_) {
	return StrongPersistentTL(uv8stream);
}

void uv8_init_stream(Isolate* this_) {
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
	stream->Set(this_, "onRead", FunctionTemplate::New(this_, uv8_stream_onread));
	stream->Set(this_, "onWrite", FunctionTemplate::New(this_, uv8_stream_onwrite));
	stream->Set(this_, "onConnect", FunctionTemplate::New(this_, uv8_stream_onconnect));
	stream->Set(this_, "onShutdown", FunctionTemplate::New(this_, uv8_stream_onshutdown));
	stream->Set(this_, "onConnection", FunctionTemplate::New(this_, uv8_stream_onconnection));
	stream->SetInternalFieldCount(1);
	uv8stream.Reset(this_, stream);

}