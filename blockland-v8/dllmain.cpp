#pragma once
#include "torque.h"
#include "duktape.h"
#include <Windows.h>

duk_context* _Context;

static duk_ret_t duk__print_helper(duk_context *ctx) {
	duk_idx_t nargs;

	nargs = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_concat(ctx, nargs);
	Printf("%s", duk_to_string(ctx, -1));
	return 0;
}

static duk_ret_t duk__print(duk_context *ctx) {
	return duk__print_helper(ctx);
}

static duk_ret_t duk__ts_eval(duk_context *ctx) {
	const char* result = Eval(duk_require_string(ctx, -1));
	if (result == NULL)
	{
		return 0;
	}
	else
	{
		duk_push_string(_Context, result);
		return 1;
	}
}

static duk_ret_t duk__ts_call(duk_context *ctx) {
	duk_idx_t nargs;

	nargs = duk_get_top(ctx);
}

static const char *ts__js_eval(SimObject *obj, int argc, const char *argv[])
{
	if (argv[1] == NULL)
		return "";

	if (duk_peval_string(_Context, argv[1]) != 0)
	{
		Printf("eval fail: %s", duk_safe_to_string(_Context, -1));
	}
	else
	{
		return duk_get_string(_Context, -1);
	}
	duk_pop(_Context);
	//Unlocker unlocker(_Isolate);
	return "";
}

bool init()
{
	if (!torque_init())
		return false;

	//Initialize V8
	Printf("Initializing Duktape");
	_Context = duk_create_heap_default();
	if (_Context)
	{
		duk_push_global_object(_Context);
		duk_push_string(_Context, "print");
		duk_push_c_function(_Context, duk__print, DUK_VARARGS);
		duk_def_prop(_Context, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WRITABLE | DUK_DEFPROP_SET_CONFIGURABLE);
		duk_push_string(_Context, "ts_eval");
		duk_push_c_function(_Context, duk__ts_eval, DUK_VARARGS);
		duk_def_prop(_Context, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WRITABLE | DUK_DEFPROP_SET_CONFIGURABLE);
		duk_pop(_Context);
	}
	else
	{
		return 0;
	}

/*
	global->Set(String::NewFromUtf8(_Isolate, "print", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_print));

	global->Set(String::NewFromUtf8(_Isolate, "ts_eval", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_eval));

	global->Set(String::NewFromUtf8(_Isolate, "ts_call", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_call));

	global->Set(String::NewFromUtf8(_Isolate, "ts_newObj", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_newObj));

	global->Set(String::NewFromUtf8(_Isolate, "ts_setVariable", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_setVariable));

	global->Set(String::NewFromUtf8(_Isolate, "ts_getVariable", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_getVariable));

	global->Set(String::NewFromUtf8(_Isolate, "ts_setObjectField", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_setObjectField));

	global->Set(String::NewFromUtf8(_Isolate, "ts_getObjectField", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_getObjectField));

	global->Set(String::NewFromUtf8(_Isolate, "ts_func", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_func));

	global->Set(String::NewFromUtf8(_Isolate, "load", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, Load));

	global->Set(String::NewFromUtf8(_Isolate, "read", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, Read));

	global->Set(String::NewFromUtf8(_Isolate, "ts_newBrick", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_newBrick));

	//Create context w/ global
	Local<Context> context = Context::New(_Isolate, NULL, global);
	_Context.Reset(_Isolate, context);
*/
	//TorqueScript functions
	ConsoleFunction(NULL, "js_eval", ts__js_eval,
		"js_eval(string [, silent]) - evaluates a javascript string and returns the result", 2, 3);

	Eval("package blocklandjs{function quit(){return parent::quit();}};activatepackage(blocklandjs);");

	Printf("BlocklandJS | Attached");
	return true;
}

bool deinit()
{
	//delete _Platform; <- this seemed to break it
	duk_destroy_heap(_Context);
	Printf("BlocklandJS | Detached");
	return true;
}

int __stdcall DllMain(HINSTANCE hInstance, unsigned long reason, void *reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
		return init();
	else if (reason == DLL_PROCESS_DETACH)
		return deinit();

	return true;
}

extern "C" void __declspec(dllexport) __cdecl placeholder(void) { }
