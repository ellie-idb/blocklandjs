#pragma once
#include "uv8.h"
#include "../libs/miniz.c"

uv8_efunc(uv8_tinfl) {
	ThrowArgsNotVal(2);
	if (!args[0]->IsUint8Array()) {
		ThrowBadArg();
	}
	if (!args[1]->IsNumber()) {
		ThrowBadArg();
	}

	int flags = args[1]->Int32Value();
	Handle<Uint8Array> ar = Handle<Uint8Array>::Cast(args[0]->ToObject());
	ArrayBuffer::Contents ct = ar->Buffer()->GetContents();
	size_t outl;
	void* out = tinfl_decompress_mem_to_heap(ct.Data(), ct.ByteLength(), &outl, flags);
	//memcpy(out, ct.Data(), outl);
	Handle<ArrayBuffer> ret_ab = ArrayBuffer::New(args.GetIsolate(), outl);
	memcpy(ret_ab->GetContents().Data(), out, outl);
	free(out);
	Handle<Uint8Array> ret = Uint8Array::New(ret_ab, 0, outl);
	args.GetReturnValue().Set(ret);
	//free(out);
}

uv8_efunc(uv8_tdefl) {
	ThrowArgsNotVal(2);
	if (!args[0]->IsUint8Array()) {
		ThrowBadArg();
	}
	if (!args[1]->IsNumber()) {
		ThrowBadArg();
	}
	int flags = args[1]->Int32Value();
	Handle<Uint8Array> ar = Handle<Uint8Array>::Cast(args[0]->ToObject());
	ArrayBuffer::Contents ct = ar->Buffer()->GetContents();
	size_t outl;
	void* out = tdefl_compress_mem_to_heap(ct.Data(), ct.ByteLength(), &outl, flags);
	Handle<ArrayBuffer> ret_ab = ArrayBuffer::New(args.GetIsolate(), outl);
	memcpy(ret_ab->GetContents().Data(), out, outl);
	free(out);
	Handle<Uint8Array> ret = Uint8Array::New(ret_ab, 0, outl);
	args.GetReturnValue().Set(ret);
	//uv8_unfinished_args();
}

Handle<ObjectTemplate> uv8_bind_miniz(Isolate* this_) {
	Handle<ObjectTemplate> miniz = ObjectTemplate::New(this_);
	miniz->Set(this_, "tinfl", FunctionTemplate::New(this_, uv8_tinfl));
	miniz->Set(this_, "tdefl", FunctionTemplate::New(this_, uv8_tdefl));
	return miniz;
}