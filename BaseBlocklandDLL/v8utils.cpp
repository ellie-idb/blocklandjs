#pragma once
#include "uv8.h"

using namespace v8;


void ThrowError(Isolate* this_, const char* error) {
	this_->ThrowException(String::NewFromUtf8(this_, error));
}

uv_stream_t* get_stream(const FunctionCallbackInfo<Value> &args) {
	uv_stream_t* bleh = *(uv_stream_t**)Handle<External>::Cast(args.This()->GetPrototype()->ToObject()->GetInternalField(0))->Value();
	return bleh;
}

uv_stream_t* get_stream_from_ret(const FunctionCallbackInfo<Value> &args) {
	uv_stream_t* bleh = *(uv_stream_t**)Handle<External>::Cast(args.GetReturnValue().Get()->ToObject()->GetPrototype()->ToObject()->GetInternalField(0))->Value();
	return bleh;
}

void* uv8_malloc(Isolate* this_, size_t size) {
	this_->AdjustAmountOfExternalAllocatedMemory(size);
	return malloc(size);
}

void uv8_free(Isolate* this_, void* ptr) {
	this_->AdjustAmountOfExternalAllocatedMemory(-(int64_t)sizeof(ptr));
	free(ptr);
}

void uv8_gc_cb(const v8::WeakCallbackInfo<uv_stream_t**> &data) {
	uv_stream_t** bleh = *data.GetParameter();
	uv_stream_t* aaa = *bleh;
	Printf("The actual thing is %d", aaa->type);
	Printf("Bitch this shit being Garbage Collected..");
}

Maybe<uv_file> CheckFile(std::string search,
	CheckFileOptions opt = CLOSE_AFTER_CHECK) {
	uv_fs_t fs_req;
	std::string path = search;
	if (path.empty()) {
		return Nothing<uv_file>();
	}

	uv_file fd = uv_fs_open(nullptr, &fs_req, path.c_str(), O_RDONLY, 0, nullptr);
	uv_fs_req_cleanup(&fs_req);

	if (fd < 0) {
		return Nothing<uv_file>();
	}

	uv_fs_fstat(nullptr, &fs_req, fd, nullptr);
	uint64_t is_directory = fs_req.statbuf.st_mode & S_IFDIR;
	uv_fs_req_cleanup(&fs_req);

	if (is_directory) {
		uv_fs_close(nullptr, &fs_req, fd, nullptr);
		uv_fs_req_cleanup(&fs_req);
		return Nothing<uv_file>();
	}

	if (opt == CLOSE_AFTER_CHECK) {
		uv_fs_close(nullptr, &fs_req, fd, nullptr);
		uv_fs_req_cleanup(&fs_req);
	}

	return Just(fd);
}

void uv8_bind_all(Isolate* this_, Handle<ObjectTemplate> globalObject) {
	Local<ObjectTemplate> libuv = ObjectTemplate::New(this_);
	//libuv->Set(this_, "fs", uv8_bind_fs(this_));
	Handle<FunctionTemplate> proc = FunctionTemplate::New(this_, uv8_new_process);
	proc->SetClassName(String::NewFromUtf8(this_, "Process"));
	uv8_init_process(this_, proc);
	Handle<FunctionTemplate> stream = FunctionTemplate::New(this_, uv8_new_stream);
	stream->SetClassName(String::NewFromUtf8(this_, "Stream"));
	uv8_init_stream(this_, stream);
	Handle<ObjectTemplate> constants = ObjectTemplate::New(this_);
	constants->Set(this_, "O_RDONLY", Int32::New(this_, O_RDONLY));
	constants->Set(this_, "O_WRONLY", Int32::New(this_, O_WRONLY));
	constants->Set(this_, "O_RDWR", Int32::New(this_, O_RDWR));
	constants->Set(this_, "O_CREAT", Int32::New(this_, O_CREAT));
	constants->Set(this_, "O_EXCL", Int32::New(this_, O_EXCL));
	constants->Set(this_, "O_TRUNC", Int32::New(this_, O_TRUNC));
	constants->Set(this_, "O_APPEND", Int32::New(this_, O_APPEND));
	libuv->Set(this_, "constants", constants);
	libuv->Set(this_, "fs", uv8_bind_fs(this_));
	//libuv->Set(this_, "handle", uv8_bind_handle(this_));
	libuv->Set(this_, "loop", uv8_bind_loop(this_));
	libuv->Set(this_, "miniz", uv8_bind_miniz(this_));
	libuv->Set(this_, "misc", uv8_bind_misc(this_));
	//libuv->Set(this_, "req", uv8_bind_req(this_));
	libuv->Set(this_, "stream", stream);
	libuv->Set(this_, "process", proc);
	//uv8_init_stream_proto(this_, stream);
	Handle<FunctionTemplate> tcp = FunctionTemplate::New(this_, uv8_new_tcp);
	tcp->SetClassName(String::NewFromUtf8(this_, "Tcp"));
	//tcp->Inherit(stream);
	uv8_init_tcp(this_, tcp, stream);
	libuv->Set(this_, "tcp", tcp);
	Handle<FunctionTemplate> timer = FunctionTemplate::New(this_, uv8_new_timer);
	timer->SetClassName(String::NewFromUtf8(this_, "Timer"));
	uv8_init_timer(this_, timer);
	libuv->Set(this_, "timer", timer);
	Handle<FunctionTemplate> tty = FunctionTemplate::New(this_, uv8_new_tty);
	tty->SetClassName(String::NewFromUtf8(this_, "TTY"));
	uv8_init_tty(this_, tty, stream);
	libuv->Set(this_, "tty", tty);
	Handle<FunctionTemplate> pipe = FunctionTemplate::New(this_, uv8_new_pipe);
	pipe->SetClassName(String::NewFromUtf8(this_, "Pipe"));
	uv8_init_pipe(this_, pipe, stream);
	libuv->Set(this_, "pipe", pipe);
	//uv8_global_context.Reset(this_, this_->GetCurrentContext());

	globalObject->Set(this_, "uv", libuv);
}

Handle<Array> convert64To32(Isolate* iso, uint64_t in) {
	Handle<Array> conv = Array::New(iso, 2);
	uint32_t low = in;
	uint32_t high = (in >> 32);
	conv->Set(0, Integer::NewFromUnsigned(iso, high));
	conv->Set(1, Integer::NewFromUnsigned(iso, low));
	return conv;

}

const char* ToCString(const v8::String::Utf8Value& value)
{
	return *value ? *value : "<string conversion failed>";
}