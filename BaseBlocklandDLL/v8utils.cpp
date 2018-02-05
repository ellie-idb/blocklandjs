#pragma once
#include "uv8.h"

using namespace v8;


void ThrowError(Isolate* this_, const char* error) {
	this_->ThrowException(String::NewFromUtf8(this_, error));
}

uv_stream_t* get_stream(const FunctionCallbackInfo<Value> &args) {
	uv_stream_t* bleh = (uv_stream_t*)Handle<External>::Cast(args.This()->GetInternalField(0))->Value();
	return bleh;
}

uv_stream_t* get_stream_from_ret(const FunctionCallbackInfo<Value> &args) {
	uv_stream_t* bleh = (uv_stream_t*)Handle<External>::Cast(args.GetReturnValue().Get()->ToObject()->GetInternalField(0))->Value();
	return bleh;
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
	uv8_init_fs(this_);
	uv8_init_timer(this_);
	uv8_init_stream(this_);
	uv8_init_process(this_);
	Handle<FunctionTemplate> proc = FunctionTemplate::New(this_, uv8_new_process);
	Handle<FunctionTemplate> stream = FunctionTemplate::New(this_, uv8_new_stream);
	stream->SetClassName(String::NewFromUtf8(this_, "Stream"));
	Handle<FunctionTemplate> fs = FunctionTemplate::New(this_, uv8_fs_new);
	fs->SetClassName(String::NewFromUtf8(this_, "Fs"));
	libuv->Set(this_, "fs", fs);
	libuv->Set(this_, "handle", uv8_bind_handle(this_));
	libuv->Set(this_, "loop", uv8_bind_loop(this_));
	libuv->Set(this_, "miniz", uv8_bind_miniz(this_));
	libuv->Set(this_, "misc", uv8_bind_misc(this_));
	libuv->Set(this_, "req", uv8_bind_req(this_));
	libuv->Set(this_, "stream", stream);
	libuv->Set(this_, "process", proc);
	//uv8_init_stream_proto(this_, stream);
	uv8_init_tcp(this_);
	Handle<FunctionTemplate> tcp = FunctionTemplate::New(this_, uv8_new_tcp);
	tcp->SetClassName(String::NewFromUtf8(this_, "Tcp"));
	libuv->Set(this_, "tcp", tcp);
	libuv->Set(this_, "timer", FunctionTemplate::New(this_, uv8_new_timer));
	//libuv->Set(this_, "tty", uv8_bind_tty(this_));
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