#pragma once
#include "torque.h"
#include "duktape.h"
#include <Windows.h>

duk_context* _Context;

/* For brevity assumes a maximum file length of 16kB. */
static void push_file_as_string(duk_context *ctx, const char *filename) {
	FILE *f;
	size_t len;
	char buf[16384];

	f = fopen(filename, "rb");
	if (f) {
		len = fread((void *)buf, 1, sizeof(buf), f);
		fclose(f);
		duk_push_lstring(ctx, (const char *)buf, (duk_size_t)len);
	}
	else {
		duk_push_undefined(ctx);
	}
}

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
		duk_push_string(ctx, result);
		return 1;
	}
}

static duk_ret_t duk__ts_handlefunc(duk_context *ctx)
{
	duk_idx_t nargs;

	nargs = duk_get_top(ctx);
	const char* function_name = duk_get_string(ctx, 0);
	Namespace *ns;
	Namespace::Entry *nsE;

	char* callee = const_cast<char*>(function_name);
	callee = strtok(callee, "__");
	const char* tsns = callee;
	callee = strtok(NULL, "__");
	const char* fnName = callee;
	if (tsns == NULL || fnName == NULL)
		return 0;

	ns = LookupNamespace(NULL);
	nsE = Namespace__lookup(ns, StringTableEntry(fnName));
	//I have no clue?
	if (nsE == NULL)
	{
		Printf("ts namespace lookup fail");
		duk_push_boolean(ctx, false);
		duk_pop(ctx);
		return 1;
	}
	//set up arrays for passing to tork
	int argc = 0;
	const char* argv[21];
	argv[argc++] = nsE->mFunctionName;
	for (int i = 1; i < nargs; i++)
	{
		const char* arg = duk_require_string(ctx, i);
		argv[argc++] = arg;
	}
	if (nsE->mType == Namespace::Entry::ScriptFunctionType)
	{
		if (nsE->mFunctionOffset)
		{
			duk_push_string(ctx, CodeBlock__exec(
				nsE->mCode, nsE->mFunctionOffset,
				nsE->mNamespace, nsE->mFunctionName, argc, argv,
				false, nsE->mPackage, 0));
			duk_pop(ctx);
			return 1;
		}
		else
			return 0;
	}

	S32 mMinArgs = nsE->mMinArgs;
	S32 mMaxArgs = nsE->mMaxArgs;
	if ((mMinArgs && argc < mMinArgs) || (mMaxArgs && argc > mMaxArgs))
	{
		Printf("Too many args to pass to TS.");
		duk_push_boolean(ctx, false);
		duk_pop(ctx);
		return 1;
	}
	void *cb = nsE->cb;
	switch (nsE->mType)
	{
	case Namespace::Entry::StringCallbackType:
		duk_push_string(ctx, ((StringCallback)cb)(NULL, argc, argv));
	case Namespace::Entry::IntCallbackType:
		duk_push_int(ctx, ((IntCallback)cb)(NULL, argc, argv));
	case Namespace::Entry::FloatCallbackType:
		duk_push_int(ctx, ((FloatCallback)cb)(NULL, argc, argv));
	case Namespace::Entry::BoolCallbackType:
		duk_push_boolean(ctx, ((BoolCallback)cb)(NULL, argc, argv));
	case Namespace::Entry::VoidCallbackType:
		((VoidCallback)cb)(NULL, argc, argv);
	}
	//?? im so fucking covfefe'd why this wipes itself after being called once...
	duk_pop(ctx);
	return 1;
}

static duk_ret_t duk__ts_func(duk_context *ctx)
{
	//Holy shit lmao. This is so much simpler here. I love you Duktape.
	Namespace *ns;
	Namespace::Entry *nsEn;
	duk_idx_t nargs;

	nargs = duk_get_top(ctx);
	//this is fine but we need the func name or we'll error :(
	const char* cNamespace = duk_to_string(ctx, 0);
	if (_stricmp(cNamespace, "") == 0)
		cNamespace = "ts";

	const char* fnName = duk_to_string(ctx, 1);
	if (_stricmp(fnName, "") == 0)
		return 0;
		
	char buffer[256];
	strncpy_s(buffer, cNamespace, sizeof(buffer));
	strncat_s(buffer, "__", sizeof(buffer));
	strncat_s(buffer, fnName, sizeof(buffer));
	if (_stricmp(cNamespace, "ts") == 0)
	{
		ns = LookupNamespace(NULL);
		nsEn = Namespace__lookup(ns, StringTableEntry(fnName));
		if (nsEn == NULL)
			return 0;

		duk_push_global_object(ctx);
		duk_push_string(ctx, buffer);
		duk_push_c_function(ctx, duk__ts_handlefunc, DUK_VARARGS);
		duk_push_string(ctx, "targetfunc"); 
		duk_push_string(ctx, buffer);
		duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
		duk_put_global_string(ctx, buffer);
		duk_pop(ctx);
	}
	return 0;
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

static const char *ts__js_load(SimObject *obj, int argc, const char* argv[])
{
	if (argv[1] == NULL)
		return "";

	push_file_as_string(_Context, argv[1]);
	if (duk_peval(_Context) != 0) {
		printf("load error: %s\n", duk_safe_to_string(_Context, -1));
	}
	else
	{
		return duk_get_string(_Context, -1);
	}
	duk_pop(_Context);
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
		//duk_push_global_object(_Context);
		//duk_push_string(_Context, "print");
		duk_push_c_function(_Context, duk__print, DUK_VARARGS);
		duk_put_global_string(_Context, "print");
		//duk_def_prop(_Context, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WRITABLE | DUK_DEFPROP_SET_CONFIGURABLE);
		//duk_push_string(_Context, "ts_eval");
		duk_push_c_function(_Context, duk__ts_eval, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_eval");
		//duk_def_prop(_Context, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WRITABLE | DUK_DEFPROP_SET_CONFIGURABLE);
		//duk_push_string(_Context, "ts_call");
		duk_push_c_function(_Context, duk__ts_handlefunc, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_call");
		//duk_def_prop(_Context, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WRITABLE | DUK_DEFPROP_SET_CONFIGURABLE);
		//duk_pop(_Context);
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

	ConsoleFunction(NULL, "js_load", ts__js_load,
		"js_load(path to file) - loads a javascript files returns the result", 2, 2);

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

extern "C" void __declspec(dllexport) __cdecl placeholder(void) { }#pragma once
#include "torque.h"
#include "duktape.h"
#include <Windows.h>

duk_context* _Context;

/* For brevity assumes a maximum file length of 16kB. */
static void push_file_as_string(duk_context *ctx, const char *filename) {
	FILE *f;
	size_t len;
	char buf[16384];

	f = fopen(filename, "rb");
	if (f) {
		len = fread((void *)buf, 1, sizeof(buf), f);
		fclose(f);
		duk_push_lstring(ctx, (const char *)buf, (duk_size_t)len);
	}
	else {
		duk_push_undefined(ctx);
	}
}

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
		duk_push_string(ctx, result);
		return 1;
	}
}

static duk_ret_t duk__ts_handlefunc(duk_context *ctx)
{
	duk_idx_t nargs;

	nargs = duk_get_top(ctx);
	const char* function_name = duk_get_string(ctx, 0);
	Namespace *ns;
	Namespace::Entry *nsE;

	char* callee = const_cast<char*>(function_name);
	callee = strtok(callee, "__");
	const char* tsns = callee;
	callee = strtok(NULL, "__");
	const char* fnName = callee;
	if (tsns == NULL || fnName == NULL)
		return 0;

	ns = LookupNamespace(NULL);
	nsE = Namespace__lookup(ns, StringTableEntry(fnName));
	//I have no clue?
	if (nsE == NULL)
	{
		Printf("ts namespace lookup fail");
		duk_push_boolean(ctx, false);
		duk_pop(ctx);
		return 1;
	}
	//set up arrays for passing to tork
	int argc = 0;
	const char* argv[21];
	argv[argc++] = nsE->mFunctionName;
	for (int i = 1; i < nargs; i++)
	{
		const char* arg = duk_require_string(ctx, i);
		argv[argc++] = arg;
	}
	if (nsE->mType == Namespace::Entry::ScriptFunctionType)
	{
		if (nsE->mFunctionOffset)
		{
			duk_push_string(ctx, CodeBlock__exec(
				nsE->mCode, nsE->mFunctionOffset,
				nsE->mNamespace, nsE->mFunctionName, argc, argv,
				false, nsE->mPackage, 0));
			duk_pop(ctx);
			return 1;
		}
		else
			return 0;
	}

	S32 mMinArgs = nsE->mMinArgs;
	S32 mMaxArgs = nsE->mMaxArgs;
	if ((mMinArgs && argc < mMinArgs) || (mMaxArgs && argc > mMaxArgs))
	{
		Printf("Too many args to pass to TS.");
		duk_push_boolean(ctx, false);
		duk_pop(ctx);
		return 1;
	}
	void *cb = nsE->cb;
	switch (nsE->mType)
	{
	case Namespace::Entry::StringCallbackType:
		duk_push_string(ctx, ((StringCallback)cb)(NULL, argc, argv));
	case Namespace::Entry::IntCallbackType:
		duk_push_int(ctx, ((IntCallback)cb)(NULL, argc, argv));
	case Namespace::Entry::FloatCallbackType:
		duk_push_int(ctx, ((FloatCallback)cb)(NULL, argc, argv));
	case Namespace::Entry::BoolCallbackType:
		duk_push_boolean(ctx, ((BoolCallback)cb)(NULL, argc, argv));
	case Namespace::Entry::VoidCallbackType:
		((VoidCallback)cb)(NULL, argc, argv);
	}
	//?? im so fucking covfefe'd why this wipes itself after being called once...
	duk_pop(ctx);
	return 1;
}

static duk_ret_t duk__ts_func(duk_context *ctx)
{
	//Holy shit lmao. This is so much simpler here. I love you Duktape.
	Namespace *ns;
	Namespace::Entry *nsEn;
	duk_idx_t nargs;

	nargs = duk_get_top(ctx);
	//this is fine but we need the func name or we'll error :(
	const char* cNamespace = duk_to_string(ctx, 0);
	if (_stricmp(cNamespace, "") == 0)
		cNamespace = "ts";

	const char* fnName = duk_to_string(ctx, 1);
	if (_stricmp(fnName, "") == 0)
		return 0;
		
	char buffer[256];
	strncpy_s(buffer, cNamespace, sizeof(buffer));
	strncat_s(buffer, "__", sizeof(buffer));
	strncat_s(buffer, fnName, sizeof(buffer));
	if (_stricmp(cNamespace, "ts") == 0)
	{
		ns = LookupNamespace(NULL);
		nsEn = Namespace__lookup(ns, StringTableEntry(fnName));
		if (nsEn == NULL)
			return 0;

		duk_push_global_object(ctx);
		duk_push_string(ctx, buffer);
		duk_push_c_function(ctx, duk__ts_handlefunc, DUK_VARARGS);
		duk_push_string(ctx, "targetfunc"); 
		duk_push_string(ctx, buffer);
		duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
		duk_put_global_string(ctx, buffer);
		duk_pop(ctx);
	}
	return 0;
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

static const char *ts__js_load(SimObject *obj, int argc, const char* argv[])
{
	if (argv[1] == NULL)
		return "";

	push_file_as_string(_Context, argv[1]);
	if (duk_peval(_Context) != 0) {
		printf("load error: %s\n", duk_safe_to_string(_Context, -1));
	}
	else
	{
		return duk_get_string(_Context, -1);
	}
	duk_pop(_Context);
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
		//duk_push_global_object(_Context);
		//duk_push_string(_Context, "print");
		duk_push_c_function(_Context, duk__print, DUK_VARARGS);
		duk_put_global_string(_Context, "print");
		//duk_def_prop(_Context, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WRITABLE | DUK_DEFPROP_SET_CONFIGURABLE);
		//duk_push_string(_Context, "ts_eval");
		duk_push_c_function(_Context, duk__ts_eval, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_eval");
		//duk_def_prop(_Context, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WRITABLE | DUK_DEFPROP_SET_CONFIGURABLE);
		//duk_push_string(_Context, "ts_call");
		duk_push_c_function(_Context, duk__ts_handlefunc, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_call");
		//duk_def_prop(_Context, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_WRITABLE | DUK_DEFPROP_SET_CONFIGURABLE);
		//duk_pop(_Context);
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

	ConsoleFunction(NULL, "js_load", ts__js_load,
		"js_load(path to file) - loads a javascript files returns the result", 2, 2);

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
