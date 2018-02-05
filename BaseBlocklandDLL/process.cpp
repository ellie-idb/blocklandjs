#pragma once
#include "uv8.h"

using namespace v8;

Persistent<ObjectTemplate> uvprocess;

uv8_efunc(uv8_new_process) {
	//numb, numb, numb..
	//so give me just enough to
	//make me feel something~
	//make me feel something~
	ThrowArgsNotVal(1);
	if (!args[0]->IsObject()) {
		ThrowBadArg();
	}

	Handle<Object> arguments = args[0]->ToObject();
	uv_process_t proc;
	uv_process_options_t opts = { 0 };
	if (!arguments->Has(String::NewFromUtf8(args.GetIsolate(), "file")) || !arguments->Has(String::NewFromUtf8(args.GetIsolate(), "args"))) {
		ThrowError(args.GetIsolate(), "Could not find required arguments..");
		return;
	}

	if (!arguments->Get(String::NewFromUtf8(args.GetIsolate(), "args"))->IsArray()) {
		ThrowError(args.GetIsolate(), "Arguments was not an array!");
		return; 
	}

	opts.file = ToCString(String::Utf8Value(arguments->Get(String::NewFromUtf8(args.GetIsolate(), "file")->ToString())));
	Handle<Array> procArgs = Handle<Array>::Cast(arguments->Get(String::NewFromUtf8(args.GetIsolate(), "args")));
	char** arr = (char**)malloc(sizeof(*arr) * procArgs->Length());
	for (int i = 0; i < procArgs->Length(); i++) {
		 arr[i] = (char*)ToCString(String::Utf8Value(procArgs->ToString()));
	}
	opts.args = arr;

	Handle<Array> stdio = Array::New(args.GetIsolate());
	if (arguments->Has(String::NewFromUtf8(args.GetIsolate(), "stdio"))) {
		if (!arguments->Get(String::NewFromUtf8(args.GetIsolate(), "stdio"))->IsArray()) {
			ThrowError(args.GetIsolate(), "Stdio was not an array!");
			return;
		}
		else {
			//stdio = Handle<Array>::Cast(arguments->Get(String::NewFromUtf8(args.GetIsolate(), "stdio")));
			uv_stdio_container_t* arrr = (uv_stdio_container_t*)malloc(sizeof(arrr) * stdio->Length());
			for (int i = 0; i < 3; i++) {
				arrr[i].flags = (uv_stdio_flags)(UV_CREATE_PIPE & UV_READABLE_PIPE & UV_WRITABLE_PIPE);
			}
			opts.stdio = arrr;
		}
	}


	Handle<String> env = String::NewFromUtf8(args.GetIsolate(), "");
	if (arguments->Has(String::NewFromUtf8(args.GetIsolate(), "env"))) {
		if (!arguments->Get(String::NewFromUtf8(args.GetIsolate(), "env"))->IsString()) {
			ThrowError(args.GetIsolate(), "Env was not an string!");
			return;
		}
		else {
			env = arguments->Get(String::NewFromUtf8(args.GetIsolate(), "env"))->ToString();
			opts.env = NULL;
		}
	}
	else {
		opts.env = NULL;
	}

	Handle<String> cwd = String::NewFromUtf8(args.GetIsolate(), "");
	if (arguments->Has(String::NewFromUtf8(args.GetIsolate(), "cwd"))) {
		if (!arguments->Get(String::NewFromUtf8(args.GetIsolate(), "cwd"))->IsString()) {
			ThrowError(args.GetIsolate(), "cwd was not an string!");
			return;
		}
		else {
			cwd = arguments->Get(String::NewFromUtf8(args.GetIsolate(), "cwd"))->ToString();
			opts.cwd = ToCString(String::Utf8Value(cwd));
		}
	}
	Handle<Int32> flags;
	if (arguments->Has(String::NewFromUtf8(args.GetIsolate(), "flags"))) {
		if (!arguments->Get(String::NewFromUtf8(args.GetIsolate(), "flags"))->IsInt32() || !arguments->Get(String::NewFromUtf8(args.GetIsolate(), "flags"))->IsNumber()) {
			ThrowError(args.GetIsolate(), "flags was not an string!");
			return;
		}
		else {
			flags = arguments->Get(String::NewFromUtf8(args.GetIsolate(), "flags"))->ToInt32(args.GetIsolate());
			opts.flags = flags->Value();
		}
	}
	Handle<Function> exit_cb;

//	opts.stdio_count = 2;
	opts.stdio_count = 0;
	opts.gid = 0;
	opts.uid = 0;
	opts.flags = UV_PROCESS_WINDOWS_VERBATIM_ARGUMENTS;
	int ret = uv_spawn(uv_default_loop(), &proc, &opts);
	//free(arr);
	ThrowOnUV(ret);

}

uv8_efunc(uv8_process_kill) {

}

uv8_efunc(uv8_process_get_pid) {

}

uv8_efunc(uv8_pid_kill) {

}



void uv8_init_process(Isolate* this_) {
	Handle<ObjectTemplate> proc = ObjectTemplate::New(this_);
	proc->Set(this_, "kill", FunctionTemplate::New(this_, uv8_process_kill));

}

//Living in a cruel world.
