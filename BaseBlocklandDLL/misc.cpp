#include "uv8.h"
#include "include/uv.h"

using namespace v8;

void uv8_guess_handle(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_version(const FunctionCallbackInfo<Value> &args) {
	args.GetReturnValue().Set(Integer::New(args.GetIsolate(), uv_version()));
	return;
}

void uv8_version_string(const FunctionCallbackInfo<Value> &args) {
	args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), uv_version_string()));
	return;
}

void uv8_get_process_title(const FunctionCallbackInfo<Value> &args) {
	char procTitle[1024];
	uv_get_process_title(procTitle, 1024);
	args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), procTitle));
	return;
}

void uv8_set_process_title(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_resident_set_memory(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_uptime(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_getrusage(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_cpu_info(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_interface_addresses(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_loadavg(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_exepath(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_cwd(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_os_homedir(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_get_total_memory(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_hrtime(const FunctionCallbackInfo<Value> &args) {
	args.GetReturnValue().Set(Number::New(args.GetIsolate(), uv_hrtime()));
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_update_time(const FunctionCallbackInfo<Value> &args) {
	args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

void uv8_now(const FunctionCallbackInfo<Value> &args) {
	uv_update_time(uv_default_loop());
	uint64_t highprec = uv_now(uv_default_loop());
	args.GetReturnValue().Set(Uint32::NewFromUnsigned(args.GetIsolate(), highprec));
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}