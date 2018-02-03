#pragma once
#include "uv8.h"

using namespace v8;

Persistent<ObjectTemplate> uvtcp;

uv8_efunc(uv8_new_tcp) {
	Handle<ObjectTemplate> ay = uv8_get_stream(args.GetIsolate());
	args.GetIsolate()->AdjustAmountOfExternalAllocatedMemory(sizeof(uv_tcp_t*));
	uv_tcp_t* tc = (uv_tcp_t*)malloc(sizeof(*tc));
	Handle<Object> nwtcp = StrongPersistentTL(uvtcp)->NewInstance();
	Handle<Object> nwstream = ay->NewInstance();
	int ret = uv_tcp_init(uv_default_loop(), tc);
	if (ret < 0) {
		free(tc);
		ThrowError(args.GetIsolate(), uv_strerror(ret));
		return;
	}
	nwtcp->SetInternalField(0, External::New(args.GetIsolate(), tc));
	nwtcp->SetPrototype(args.GetIsolate()->GetCallingContext(), nwstream);
	nwstream->SetInternalField(0, External::New(args.GetIsolate(), tc));
	Persistent<Object> perstcp;
	//perstcp.Reset(args.GetIsolate(), nwtcp);
	uv8_handle* hand = new uv8_handle;
	args.GetIsolate()->AdjustAmountOfExternalAllocatedMemory(sizeof(uv8_handle*));
	hand->ref.Reset(args.GetIsolate(), nwtcp);
	hand->iso = args.GetIsolate();
	tc->data = (void*)hand;
	uv_stream_t** stream = (uv_stream_t**)malloc(sizeof(uv_stream_t**));
	args.GetIsolate()->AdjustAmountOfExternalAllocatedMemory(sizeof(uv_stream_t**));
	*stream = (uv_stream_t*)tc;
	//nwtcp->SetPrototype(args.GetIsolate()->GetCurrentContext(), StrongPersistentTL(uvstream)->NewInstance());
	//
	//Handle<Object> bleh = uvObj->ToObject();
	//Handle<Value> streamConstr = bleh->Get(String::NewFromUtf8(args.GetIsolate(), "stream"));
	//Handle<Function> actualConstr = Handle<Function>::Cast(streamConstr);

	//idk = Handle<Object>::Cast(idk->GetPrototype());

	//idk = StrongPersistentTL(uvtcp)->NewInstance();
	perstcp.Reset(args.GetIsolate(), nwtcp);
	perstcp.SetWeak<uv_stream_t**>(&stream, uv8_gc_cb, v8::WeakCallbackType::kParameter);
	args.GetReturnValue().Set(perstcp);
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
	
	if (args.Length() != 2) {
		ThrowError(args.GetIsolate(), "Wrong number of arguments");
		return;
	}

	if (!args[0]->IsString() || (!args[1]->IsInt32() || !args[1]->IsNumber())) {
		ThrowError(args.GetIsolate(), "Incorrect type of arguments.");
		return;
	}

	struct sockaddr_storage addr;
	const char* host = ToCString(String::Utf8Value(args[0]->ToString()));
	int port = args[1]->ToInteger()->Int32Value();
	if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) && uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
		ThrowError(args.GetIsolate(), "Invalid IP/Port.");
		return;
	}
	Handle<External> bleh = Handle<External>::Cast(args.This()->GetInternalField(0));
	uv_tcp_t* aaa = (uv_tcp_t*)bleh->Value();
	int error = uv_tcp_bind(aaa, (struct sockaddr*)&addr, 0);
	if (error < 0) {
		args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), uv_strerror(error)));
	}
	else {
		return;
	}
	//uv8_unfinished_args();
}


void wrap_sockaddr(const FunctionCallbackInfo<Value> &args, struct sockaddr_storage* addr, int len) {
	char ipaddr[INET6_ADDRSTRLEN];
	int port = 0;

	Isolate* this_ = args.GetIsolate();
	Handle<Object> retval = Object::New(this_);
	if (addr->ss_family == AF_INET) {
		struct sockaddr_in* ad = (struct sockaddr_in*) addr;
		uv_inet_ntop(AF_INET, &(ad->sin_addr), ipaddr, len);
		port = ntohs(ad->sin_port);
		retval->Set(String::NewFromUtf8(this_, "family"), String::NewFromUtf8(this_, "INET"));
	}
	else if (addr->ss_family == AF_INET6) {
		struct sockaddr_in6* ad = (struct sockaddr_in6*) addr;
		uv_inet_ntop(AF_INET6, &(ad->sin6_addr), ipaddr, len);
		port = ntohs(ad->sin6_port);
		retval->Set(String::NewFromUtf8(this_, "family"), String::NewFromUtf8(this_, "INET6"));
	}
	retval->Set(String::NewFromUtf8(this_, "port"), Int32::New(this_, port));
	retval->Set(String::NewFromUtf8(this_, "ip"), String::NewFromUtf8(this_, ipaddr));
	args.GetReturnValue().Set(retval);

}

uv8_efunc(uv8_tcp_getpeername) {
	uv_tcp_t* this_ = (uv_tcp_t*)get_stream(args);
	struct sockaddr_storage addr;
	int len = sizeof(addr);
	int ret = uv_tcp_getpeername(this_, (struct sockaddr*)&addr, &len);
	if (ret < 0) {
		ThrowError(args.GetIsolate(), uv_strerror(ret));
		return;
	}
	wrap_sockaddr(args, &addr, len);
	
	//uv8_unfinished_args();
}

uv8_efunc(uv8_tcp_getsockname) {
	uv_tcp_t* this_ = (uv_tcp_t*)get_stream(args);
	struct sockaddr_storage addr;
	int len = sizeof(addr);
	int ret = uv_tcp_getsockname(this_, (struct sockaddr*)&addr, &len);
	if (ret < 0) {
		ThrowError(args.GetIsolate(), uv_strerror(ret));
		return;
	}
	wrap_sockaddr(args, &addr, len);
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
	tcp->SetInternalFieldCount(1);


	//uvstream.Reset(this_, streamin);
	uvtcp.Reset(this_, tcp);
}
