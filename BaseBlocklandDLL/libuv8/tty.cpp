#pragma once
#include "uv8.h"

using namespace v8;

Persistent<FunctionTemplate> uv8tty;
Persistent<FunctionTemplate> uv8stream;

uv8_efunc(uv8_new_tty) {
	//uv8_unfinished_args();
	ThrowArgsNotVal(2);
	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}

	if (!args[1]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}

	Handle<Context> ctx = args.GetIsolate()->GetCurrentContext();
	Handle<Object> cons = StrongPersistentTL(uv8tty)->InstanceTemplate()->NewInstance();
	Handle<Object> str = StrongPersistentTL(uv8stream)->GetFunction()->NewInstance(ctx).ToLocalChecked();
	Handle<External> wrapped = Handle<External>::Cast(str->GetInternalField(0));
	uv_stream_t** bb = (uv_stream_t**)wrapped->Value();
	uv_stream_t* aaa = *bb;
	cons->SetPrototype(ctx, str);
	int ret = uv_tty_init(uv_default_loop(), (uv_tty_t*)aaa, (uv_file)args[0]->Int32Value(), args[1]->Int32Value());
	if (ret < 0) {
		//free(aaa);
		ThrowError(args.GetIsolate(), uv_strerror(ret));
		return;
	}
	args.GetReturnValue().Set(cons);
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

void uv8_init_tty(Isolate* this_, Handle<FunctionTemplate> constructor, Handle<FunctionTemplate> parent) {
	Handle<ObjectTemplate> tty = constructor->InstanceTemplate();
	Handle<ObjectTemplate> ttymode = ObjectTemplate::New(this_);
	ttymode->Set(this_, "reset", FunctionTemplate::New(this_, uv8_tty_reset_mode));
	ttymode->Set(this_, "set", FunctionTemplate::New(this_, uv8_tty_set_mode));
	tty->Set(this_, "mode", ttymode);
	tty->Set(this_, "get_winsize", FunctionTemplate::New(this_, uv8_tty_get_winsize));
	//tty->SetInternalFieldCount(1);

	uv8tty.Reset(this_, constructor);
	uv8stream.Reset(this_, parent);
}
