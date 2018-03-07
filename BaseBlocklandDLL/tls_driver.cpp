#define WIN32_LEAN_AND_MEAN
#pragma once
#include "tls.h"
#include "uv8.h"
#include "Torque.h"

Persistent<FunctionTemplate> tls_template;

void new_tls(const FunctionCallbackInfo<Value> &args) {
	//uv8_unfinished_args();
	//ThrowArgsNotVal(2);
	Isolate* iso = args.GetIsolate();
	if (args.Length() < 3) {
		ThrowError(args.GetIsolate(), "Not enough arguments");
		return;
	}

	if (!args[0]->IsObject()) {
		ThrowBadArg();
	}

	Handle<Object> undertcp = args[0]->ToObject();
	Local<String> constructorName = undertcp->GetConstructorName();
	if (!constructorName->Equals(String::NewFromUtf8(args.GetIsolate(), "tcp"))) {
		String::Utf8Value c_cname(constructorName);
		Printf("%s", *c_cname);
		ThrowError(args.GetIsolate(), "Not a LibUV tcp object underlying.");
		return;
	}
	

	Handle<External> wrapped = Handle<External>::Cast(undertcp->GetPrototype()->ToObject()->GetInternalField(0));
	uv_stream_t** bb = (uv_stream_t**)wrapped->Value();
	uv_tcp_t* aaa = (uv_tcp_t*)*bb;

	if (!args[1]->IsBoolean()) {
		ThrowBadArg();
	}

	bool is_client = args[1]->BooleanValue();

	if (aaa->socket == -1 && is_client) {
		ThrowError(args.GetIsolate(), "TCP object is not connected.");
		return;
	}

	tls* t;



	if (is_client) {
		t = tls_client();
	}
	else {
		t = tls_server();
	}


	String::Utf8Value c_serverName(args[2]->ToString());

	const char* serverName = *c_serverName;

	tls_config* config = tls_config_new();
	uint32_t protocols = TLS_PROTOCOLS_ALL;
	const char* ciphers = "secure";
	const char* dheparams = "auto";
	const char* echdecurves = "default";
	const char* ca_cert_file = "cacert.pem";
	const char* certificate = "cert.pem";
	const char* private_key = "key.pem";
	bool prefer_ciphers_client = false;
	if (args.Length() == 4) {
		if (!args[3]->IsObject()) {
			ThrowBadArg();
		}

	}
	tls_config_set_dheparams(config, dheparams);
	tls_config_set_ecdhecurve(config, echdecurves);
	tls_config_set_ciphers(config, ciphers);
	if (prefer_ciphers_client) {
		tls_config_prefer_ciphers_client(config);
	}
	else {
		tls_config_prefer_ciphers_server(config);
	}
	tls_config_verify(config);
	
	undertcp->SetInternalField(0, True(args.GetIsolate()));
	undertcp->SetInternalField(1, External::New(args.GetIsolate(), (void*)t));

	uint8_t *mem;
	size_t mem_len;
	if ((mem = tls_load_file(ca_cert_file, &mem_len, NULL)) == NULL) {
		ThrowError(args.GetIsolate(), "unable to load ca cert file");
		return;
	}

	if (tls_config_set_ca_mem(config, mem, mem_len) != 0) {
		ThrowError(args.GetIsolate(), "unable to set ca cert file");
		return;
	}

	if (!is_client) {

		if ((mem = tls_load_file(certificate, &mem_len, NULL)) == NULL) {
			ThrowError(args.GetIsolate(), "unable to load certificate file");
			return;
		}

		if (tls_config_set_cert_mem(config, mem, mem_len) != 0) {
			ThrowError(args.GetIsolate(), "unable to set certificate file");
			return;
		}

		if ((mem = tls_load_file(private_key, &mem_len, NULL)) == NULL) {
			ThrowError(args.GetIsolate(), "unable to load private key file");
			return;
		}

		if (tls_config_set_key_mem(config, mem, mem_len) != 0) {
			ThrowError(args.GetIsolate(), "unable to set private key file");
			return;
		}
	}

	tls_configure(t, config);

	if (is_client) {
		int con = tls_connect_socket(t, aaa->socket, serverName);
		if (con == 0) {
			Handle<Object> proto = undertcp->GetPrototype()->ToObject();
			proto->Set(String::NewFromUtf8(args.GetIsolate(), "read"), FunctionTemplate::New(args.GetIsolate(), js_tls_read)->GetFunction());
			proto->Set(String::NewFromUtf8(args.GetIsolate(), "write"), FunctionTemplate::New(args.GetIsolate(), js_tls_read)->GetFunction());
			Local<Function> oldReset = Local<Function>::Cast(proto->Get(String::NewFromUtf8(args.GetIsolate(), "close"))->ToObject());
			proto->Set(String::NewFromUtf8(args.GetIsolate(), "__close__"), oldReset);
			proto->Set(String::NewFromUtf8(args.GetIsolate(), "close"), FunctionTemplate::New(args.GetIsolate(), js_tls_free)->GetFunction());
			Printf("connected");
		}
		else {
			Printf("unsuccessful");
		}
	}
	else {
		Handle<Object> proto = undertcp->GetPrototype()->ToObject();
		Local<Function> oldReset = Local<Function>::Cast(proto->Get(String::NewFromUtf8(args.GetIsolate(), "close"))->ToObject());
		proto->Set(String::NewFromUtf8(args.GetIsolate(), "__close__"), oldReset);
		proto->Set(String::NewFromUtf8(args.GetIsolate(), "close"), FunctionTemplate::New(args.GetIsolate(), js_tls_free)->GetFunction());
	}
}

void js_tls_free(const FunctionCallbackInfo<Value> &args) {
	Handle<Object> this_ = args.This();
	if (this_->GetInternalField(0)->ToBoolean()->BooleanValue()) {
		Handle<External> tlsa = Handle<External>::Cast(this_->GetInternalField(1));
		tls* t = (tls*)tlsa->Value();
		tls_close(t);
		if (tls_error(t) == NULL) {
			tls_free(t);
			this_->SetInternalField(0, False(args.GetIsolate()));
		}
		else {
			Printf("tls error: %s", tls_error(t));
		}

		Handle<Function> closeCb = Handle<Function>::Cast(this_->GetPrototype()->ToObject()->Get(String::NewFromUtf8(args.GetIsolate(), "__close__"))->ToObject());
		Handle<Value> args[1];
		Handle<Value> aa = closeCb->Call(this_, 0, args);
	}
	else {
		ThrowError(args.GetIsolate(), "tls has already been freed");
		return;
	}

}

void js_tls_reset(const FunctionCallbackInfo<Value> &args) {

}

void js_tls_configure(const FunctionCallbackInfo<Value> &args) {

}

void js_tls_read(const FunctionCallbackInfo<Value> &args) {
	uv8_unfinished_args();
}

void js_tls_write(const FunctionCallbackInfo<Value> &args) {
	uv8_unfinished_args();
}

void tls_wrapper_init(Isolate* this_, Handle<ObjectTemplate> global) {
	tls_init();

	Handle<FunctionTemplate> tls_1 = FunctionTemplate::New(this_, new_tls);
	tls_1->SetClassName(String::NewFromUtf8(this_, "tls"));
	global->Set(this_, "tls", tls_1);
}