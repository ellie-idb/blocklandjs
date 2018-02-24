#pragma once
#include "uv8.h"

using namespace v8;

Persistent<ObjectTemplate> uvtimer;

void uv8_c_timer_cb(uv_timer_t* handle) {
	uv8_cb_handle* cb_handle = (uv8_cb_handle*)handle->data;
	Locker locker(cb_handle->iso);
	Isolate::Scope(cb_handle->iso);
	cb_handle->iso->Enter();
    HandleScope handle_scope(cb_handle->iso);
	Context::Scope contextl(StrongPersistentTL(_Context));
	StrongPersistentTL(_Context)->Enter();
	Handle<Value> args[1];
	StrongPersistentTL(cb_handle->ref)->Call(StrongPersistentTL(_Context)->Global(), 0, args);
	cb_handle->iso->Exit();
	Unlocker unlocker(cb_handle->iso);
}

void uv8_timer_gc(const v8::WeakCallbackInfo<uv_timer_t*> &data) {
	uv_timer_t** safe = data.GetParameter();
	uv_timer_t* unsafe = *safe;
	if (unsafe != nullptr) {
		if (uv_is_active((uv_handle_t*)unsafe)) {
			return;	
		}
		else {
			uv8_cb_handle* cb_handle = (uv8_cb_handle*)unsafe->data;
			uv_close((uv_handle_t*)unsafe, nullptr);
			delete unsafe;
			//uv8_free(data.GetIsolate(), (void*)unsafe);
			uv8_free(data.GetIsolate(), (void*)safe);
			uv8_free(data.GetIsolate(), (void*)cb_handle);
		}
	}
}

uv_timer_t* get_timer(const FunctionCallbackInfo<Value> &args) {
	uv_timer_t** aa = (uv_timer_t**)Handle<External>::Cast(args.This()->GetInternalField(0))->Value();
	return *aa;
}

uv8_efunc(uv8_new_timer) {
	//uv8_unfinished_args();
	Handle<Object> new_timer = StrongPersistentTL(uvtimer)->NewInstance();
	uv8_cb_handle* cb_handle = (uv8_cb_handle*)uv8_malloc(args.GetIsolate(), sizeof(*cb_handle));
	cb_handle->argc = 0;
	cb_handle->iso = args.GetIsolate();
	uv_timer_t* timer = new uv_timer_t;
	int ret = uv_timer_init(uv_default_loop(), timer);
	ThrowOnUV(ret);

	timer->data = (void*)cb_handle;
	//args.GetIsolate()->AdjustAmountOfExternalAllocatedMemory(sizeof(*timer));
	uv_timer_t** weakptr = (uv_timer_t**)uv8_malloc(args.GetIsolate(), sizeof(*weakptr));
	*weakptr = timer;
	new_timer->SetInternalField(0, External::New(args.GetIsolate(), (void*)weakptr));
	Persistent<Object> gc_timer;
	gc_timer.Reset(args.GetIsolate(), new_timer);
	gc_timer.SetWeak<uv_timer_t*>(weakptr, uv8_timer_gc, v8::WeakCallbackType::kParameter);
	args.GetReturnValue().Set(gc_timer);
}

uv8_efunc(uv8_timer_start) {
	ThrowArgsNotVal(3);

	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}

	if (!args[1]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}

	if (!args[2]->IsFunction()) {
		ThrowBadArg();
	}
	uv_timer_t* this_ = get_timer(args);
	int timeout = args[0]->ToInt32(args.GetIsolate())->Value();
	int repeat = args[1]->ToInt32(args.GetIsolate())->Value();
	Handle<Function> callback = Handle<Function>::Cast(args[2]->ToObject());
	uv8_cb_handle* cbhandle = (uv8_cb_handle*)this_->data;
	cbhandle->ref.Reset(args.GetIsolate(), callback);
	int ret = uv_timer_start(this_, uv8_c_timer_cb, timeout, repeat);
	ThrowOnUV(ret);
	//uv8_unfinished_args();
}

uv8_efunc(uv8_timer_stop) {
	uv_timer_t* this_ = get_timer(args);
	int ret = uv_timer_stop(this_);
	ThrowOnUV(ret);
	//uv8_unfinished_args();
}

uv8_efunc(uv8_timer_again) {
	uv_timer_t* this_ = get_timer(args);
	int ret = uv_timer_again(this_);
	ThrowOnUV(ret);
	//uv8_unfinished_args();
}

uv8_efunc(uv8_timer_set_repeat) {
	ThrowArgsNotVal(1);
	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}

	int repeat = args[0]->ToInt32(args.GetIsolate())->Value();
	uv_timer_t* this_ = get_timer(args);
	uv_timer_set_repeat(this_, repeat);

	//uv8_unfinished_args();
}

uv8_efunc(uv8_timer_get_repeat) {
	int ret = uv_timer_get_repeat(get_timer(args));
	args.GetReturnValue().Set(Int32::New(args.GetIsolate(), ret));
	//uv8_unfinished_args();
}

/*
Handle<ObjectTemplate> uv8_bind_timer(Isolate* this_) {
	Handle<ObjectTemplate> timer = ObjectTemplate::New(this_);
	return timer;
}
*/
void uv8_init_timer(Isolate* this_, Handle<FunctionTemplate> constructor) {
	Handle<ObjectTemplate> timer = constructor->InstanceTemplate();
	timer->Set(this_, "start", FunctionTemplate::New(this_, uv8_timer_start));
	timer->Set(this_, "stop", FunctionTemplate::New(this_, uv8_timer_stop));
	timer->Set(this_, "again", FunctionTemplate::New(this_, uv8_timer_again));
	timer->Set(this_, "set_repeat", FunctionTemplate::New(this_, uv8_timer_set_repeat));
	timer->Set(this_, "get_repeat", FunctionTemplate::New(this_, uv8_timer_get_repeat));
	timer->SetInternalFieldCount(1);

	uvtimer.Reset(this_, timer);
}