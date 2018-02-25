#pragma once
#include "uv8.h"

using namespace v8;

Persistent<ObjectTemplate> uv8stream;

void uv8_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	buf->base = (char*)malloc(suggested_size);
	assert(buf->base);
	buf->len = suggested_size;
}

void uv8_stream_gc(const v8::WeakCallbackInfo<uv_stream_t*> &data) {
	//Printf("Freeing a stream with 0 references.");
	Isolate* this_ = data.GetIsolate();
	uv_stream_t** weak = data.GetParameter();
	uv_stream_t* notweak = *weak;
	if (notweak != nullptr) {
		uv8_handle* handle = (uv8_handle*)notweak->data;
		if (notweak->type != uv_handle_type::UV_STREAM) {
			uv_read_stop(notweak);
			uv_shutdown(nullptr, notweak, nullptr);
			uv_close((uv_handle_t*)notweak, nullptr);
		}
		uv_unref((uv_handle_t*)notweak);
		uv8_free(data.GetIsolate(), weak);
		delete notweak;
		delete handle;
	}
}

void uv8_c_connection_cb(uv_stream_t* handle, int status) {
	//Printf("Called!");
	uv8_prepare_cb();
	if (!fuck->onConnectionCB.IsEmpty()) {
		Handle<Function>lol = StrongPersistentTL(fuck->onConnectionCB);
		Handle<Value> args[1];
		args[0] = Int32::New(fuck->iso, status);
		lol->Call(StrongPersistentTL(fuck->ref), 1, args);
	}
	uv8_cleanup_cb();
}

void uv8_c_connect_cb(uv_connect_t* req, int status) {
	uv_stream_t* handle = (uv_stream_t*)req->handle;
	uv8_prepare_cb();
	if (!fuck->onConnectCB.IsEmpty()) {
		Handle<Function>lol = StrongPersistentTL(fuck->onConnectCB);
		Handle<Value> args[1];
		args[0] = Int32::New(fuck->iso, status);
		lol->Call(StrongPersistentTL(fuck->ref), 1, args);
	}
	uv8_cleanup_cb();
}

void uv8_c_close_cb(uv_handle_t* hand) {
	uv_stream_t* handle = (uv_stream_t*)hand;
	uv8_prepare_cb();
	if (!fuck->onCloseCB.IsEmpty()) {
		Handle<Function>lol = StrongPersistentTL(fuck->onCloseCB);
		Handle<Value> args[1];
		lol->Call(StrongPersistentTL(fuck->ref), 0, args);
	}
	uv8_cleanup_cb();
}

void uv8_write_cb(uv_write_t* req, int status) {
	uv_stream_t* handle = req->handle;
	uv8_prepare_cb();
	if (!fuck->onWriteCB.IsEmpty()) {
		Handle<Function> lol = StrongPersistentTL(fuck->onWriteCB);
		Handle<Value> args[1];
		args[0] = Int32::New(fuck->iso, status);
		lol->Call(StrongPersistentTL(fuck->ref), 1, args);
	}
	free(&req->write_buffer);
	uv8_cleanup_cb();
}


void uv8_c_read_cb(uv_stream_t* stream, ssize_t read, const uv_buf_t* buf) {
	uv_stream_t* handle = stream;
	void* tobefreed = nullptr;
	uv8_prepare_cb();
	if (!fuck->onReadCB.IsEmpty()) {
		Handle<Function> lol = StrongPersistentTL(fuck->onReadCB);

		
		Handle<Value> args[2];
		args[0] = Int32::New(fuck->iso, read);
		if (read < 0) {
			args[1] = String::NewFromUtf8(fuck->iso, "");
		}
		else if (read == UV_EOF) {
			//args[0] = Int32::New(fuck->iso, UV_EOF);
			args[1] = Undefined(fuck->iso);
		}
		else {
			//Persistent<ArrayBuffer> abuf;
			//uv_buf_t** wrappedBuf = (uv_buf_t**)uv8_malloc(fuck->iso, sizeof(*wrappedBuf));
			//*wrappedBuf = (uv_buf_t*)buf;
			Handle<ArrayBuffer> abuf = ArrayBuffer::New(fuck->iso, read);
			memcpy(abuf->GetContents().Data(), buf->base, read);
			args[1] = abuf;
		}

		lol->Call(StrongPersistentTL(fuck->ref), 2, args);

	}
	uv8_cleanup_cb();
	//free((void*)buf);
}

void uv8_c_shutdown_cb(uv_shutdown_t* req, int status) {
	uv_stream_t* handle = req->handle;
	uv8_prepare_cb();
	if (!fuck->onShutdownCB.IsEmpty()) {
		Handle<Function> lol = StrongPersistentTL(fuck->onShutdownCB);
		Handle<Value> args[1];
		args[0] = Int32::New(fuck->iso, status);
		lol->Call(StrongPersistentTL(fuck->ref), 1, args);
	}
	uv8_cleanup_cb();
	//free(req);
}

uv8_efunc(uv8_new_stream) {
	//Printf("Called");
	bool istcp = false;
	if (args.Length() == 1) {
		istcp = args[0]->BooleanValue();
	}
	Isolate* this_ = args.GetIsolate();
	uv8_handle* cb = new uv8_handle;
	uv_stream_t** weakPtr = (uv_stream_t**)uv8_malloc(this_, sizeof(*weakPtr));
	uv_stream_t* strm;
	if (istcp) {
		strm = (uv_stream_t*)new uv_tcp_t;
	}
	else {
		strm = new uv_stream_t;
	}
	cb->iso = this_;
	Handle<Object> nonpers = StrongPersistentTL(uv8stream)->NewInstance();
	strm->type = uv_handle_type::UV_STREAM;
	strm->data = (void*)cb;
	*weakPtr = strm;
	nonpers->SetInternalField(0, External::New(this_, (void*)weakPtr));
	cb->ref.Empty();
	cb->onConnectionCB.Empty();
	cb->onReadCB.Empty();
	cb->onWriteCB.Empty();
	cb->onConnectCB.Empty();
	cb->onCloseCB.Empty();
	cb->onShutdownCB.Empty();
	cb->ref.Reset(this_, nonpers);
	cb->ref.SetWeak<uv_stream_t*>(weakPtr, uv8_stream_gc, v8::WeakCallbackType::kParameter);
	args.GetReturnValue().Set(cb->ref);
	//uv8_unfinished_args();
}

uv8_efunc(uv8_shutdown) {
	uv_stream_t* this_ = get_stream(args);
	uv_shutdown_t req;
	uv_shutdown(&req, this_, uv8_c_shutdown_cb);

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
	else if (bleh->type == uv_handle_type::UV_NAMED_PIPE) {
		ThrowError(args.GetIsolate(), "Support not in yet.");
		return;
		//uv8_new_pipe(args);
	}
	else {
		ThrowError(args.GetIsolate(), "Unknown stream type.");
		return;
	}

	int ret = uv_accept(bleh, client);
	if (ret < 0) {
		ThrowError(args.GetIsolate(), "Accepting connection failed.");
		return;
	}
	//uv_accept()
}

uv8_efunc(uv8_read_start) {
	uv_stream_t* bleh = get_stream(args);
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
	ThrowArgsNotVal(1);
	if (!args[0]->IsUint8Array()) {
		ThrowBadArg();
	}
	Handle<Uint8Array> writeout = Handle<Uint8Array>::Cast(args[0]->ToObject());
	Handle<ArrayBuffer> actualstuff = writeout->Buffer();
	uv_stream_t* bleh = get_stream(args);
	uv_buf_t buf = uv_buf_init((char*)actualstuff->GetContents().Data(), actualstuff->ByteLength());
	uv_write_t req;

	int retn = uv_write(&req, bleh, &buf, 1, uv8_write_cb);
	//req.data = (void*)buf;
	if (retn < 0) {
		ThrowError(args.GetIsolate(), uv_strerror(retn));
		return;
	}
	//uv8_unfinished_args();
}

uv8_efunc(uv8_event_on) {
	ThrowArgsNotVal(2);

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}

	if (!args[1]->IsFunction()) {
		ThrowBadArg();
	}
	Isolate* this_ = args.GetIsolate();
	uv_stream_t* bleh = get_stream(args);
	uv8_handle* hand = (uv8_handle*)bleh->data;
	String::Utf8Value c_comp(args[0]->ToString());
	const char* comp = ToCString(c_comp);
	Handle<Function> theOther = Handle<Function>::Cast(args[1]->ToObject());
	hand->ref.Reset(this_, args.This());
	if (_stricmp(comp, "data") == 0) {
		hand->onReadCB.Reset(this_, theOther);
	}
	else if (_stricmp(comp, "connection") == 0) {
		hand->onConnectionCB.Reset(this_, theOther);
	}
	else if (_stricmp(comp, "write") == 0) {
		hand->onWriteCB.Reset(this_, theOther);
	}
	else if (_stricmp(comp, "connect") == 0) {
		hand->onConnectCB.Reset(this_, theOther);
	}
	else if (_stricmp(comp, "close") == 0) {
		hand->onCloseCB.Reset(this_, theOther);
	}
	else if (_stricmp(comp, "shutdown") == 0) {
		hand->onShutdownCB.Reset(this_, theOther);
	}
	else {
		ThrowError(this_, "No such event exists.");
		return;
	}
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

uv8_efunc(uv8_stream_close) {
	uv_stream_t* this_ = get_stream(args);

	uv_close((uv_handle_t*)this_, uv8_c_close_cb);
}

void uv8_init_stream(Isolate* this_, Handle<FunctionTemplate> constructor) {
	Handle<ObjectTemplate> stream = constructor->InstanceTemplate();
	stream->Set(this_, "shutdown", FunctionTemplate::New(this_, uv8_shutdown));
	stream->Set(this_, "listen", FunctionTemplate::New(this_, uv8_listen));
	stream->Set(this_, "accept", FunctionTemplate::New(this_, uv8_accept));
	stream->Set(this_, "read_start", FunctionTemplate::New(this_, uv8_read_start));
	stream->Set(this_, "read_stop", FunctionTemplate::New(this_, uv8_read_stop));
	stream->Set(this_, "write", FunctionTemplate::New(this_, uv8_write));
	stream->Set(this_, "is_readable", FunctionTemplate::New(this_, uv8_is_readable));
	stream->Set(this_, "is_writable", FunctionTemplate::New(this_, uv8_is_writable));
	stream->Set(this_, "set_blocking", FunctionTemplate::New(this_, uv8_stream_set_blocking));
	stream->Set(this_, "on", FunctionTemplate::New(this_, uv8_event_on));
	stream->Set(this_, "close", FunctionTemplate::New(this_, uv8_stream_close));
	stream->SetInternalFieldCount(2);
	uv8stream.Reset(this_, stream);

}