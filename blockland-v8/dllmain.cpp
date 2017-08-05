#pragma once
#include "torque.h"
#include "duktape.h"
#include "duk_module_node.h"
#include <map>
#include <Windows.h>

static duk_context* _Context;
static std::map<char*, Namespace::Entry*> cache;
static std::map<char*, Namespace*> nscache;
static std::map<int, SimObject**> garbagec_ids;
static Namespace* GlobalNS = NULL;

/* stolen from the duktape example as well, too lazy to rewrite an existing solution which i know works */
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

//part stolen from the duktape example code but it's modified to be shorter- no raw data however :(
static duk_ret_t duk__print_helper(duk_context *ctx) {
	duk_idx_t nargs;

	nargs = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_concat(ctx, nargs);
	Printf("%s", duk_to_string(ctx, -1));
	return 0;
}

//stolen from duktape example as well
static duk_ret_t duk__print(duk_context *ctx) {
	return duk__print_helper(ctx);
}

//yeah this was easy to write in- self implemented
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

//a large part of this was taken from blocklandjs, thank you port for all the wonderful code <3
static duk_ret_t duk__ts_handlefunc(duk_context *ctx)
{
	duk_idx_t nargs;

	nargs = duk_get_top(ctx) - 1;
	if (nargs > 19)
		duk_error(ctx, DUK_ERR_ERROR, "too many args for ts");

	const char* function_name = duk_get_string(ctx, 0);
	Namespace *ns;
	SimObject* blah = NULL;
	Namespace::Entry *nsE;
	int argc = 0;
	const char* argv[21];

	char* callee = const_cast<char*>(function_name);
	callee = strtok(callee, "__");
	const char* tsns = callee;
	callee = strtok(NULL, "__");
	const char* fnName = callee;
	if (tsns == NULL || fnName == NULL)
		return 0;

	if (_stricmp(tsns, "ts") == 0)
	{
		if (GlobalNS == NULL)
		{
			ns = LookupNamespace(NULL);
			GlobalNS = ns;
		}
		else
		{
			ns = GlobalNS;
		}
	}
	else
	{
		std::map<char*, Namespace*>::iterator its;
		char* dumb = const_cast<char*>(tsns);
		its = nscache.find(dumb);
		if(its != nscache.end())
		{
			ns = nscache.find(dumb)->second;
		}
		else
		{
			ns = LookupNamespace(tsns);
			if (ns != NULL)
			{
				nscache.insert(nscache.end(), std::pair<char*, Namespace*>(dumb, ns));
			}
			else
			{
				return 0;
			}
		}
	}
	std::map<char*, Namespace::Entry*>::iterator it;
	char* nonconst = const_cast<char*>(fnName);
	it = cache.find(nonconst);
	if (it == cache.end())
	{
		nsE = Namespace__lookup(ns, StringTableEntry(fnName));
	}
	else
	{
		nsE = cache.find(const_cast<char*>(fnName))->second;
	}
	//I have no clue?
	if (nsE == NULL)
	{
		Printf("ts namespace lookup fail");
		duk_push_boolean(ctx, false);
		//duk_pop(ctx);
		return 1;
	}
	else if (it == cache.end() && nsE != NULL)
	{
		cache.insert(cache.end(), std::pair<char*, Namespace::Entry*>(nonconst, nsE));
	}
	//set up arrays for passing to tork
	argv[argc++] = nsE->mFunctionName;
	if (duk_is_pointer(ctx, 1))
	{
		//it's a thiscall
		SimObject** ref = (SimObject**)duk_get_pointer(ctx, 1);
		blah = *ref;
		if (blah == NULL)
		{
			Printf("expected valid pointer to object");
			duk_pop(ctx);
			return 0;
		}
		char idbuf[sizeof(int) * 3 + 2];
		snprintf(idbuf, sizeof idbuf, "%d", blah->id);
		argv[argc++] = StringTableEntry(idbuf);
	}
	else
	{
		blah = NULL;
	}
	for (int i = 1; i < nargs; i++)
	{
		if (duk_is_pointer(ctx, i))
		{
			SimObject** a = (SimObject**)duk_get_pointer(ctx, i);
			SimObject* ref = *a;
			SimObjectId id;
			if (ref != NULL)
			{
				id = ref->id;
				Printf("ID: %d", id);
			}
			else
			{
				id = 0;
				Printf("found invalid reference :(");
			}

			char idbuf[sizeof(int) * 3 + 2];
			snprintf(idbuf, sizeof idbuf, "%d", id);
			argv[argc++] = StringTableEntry(idbuf);
		}
		else if(duk_is_string(ctx, i))
		{
			const char* arg = duk_get_string(ctx, i);
			argv[argc++] = arg;
		}
		else if (duk_is_boolean(ctx, i))
		{
			bool arg = duk_get_boolean(ctx, i);
			argv[argc++] = arg ? "1" : "0";
		}
		else if (duk_is_number(ctx, i))
		{
			const char* arg = duk_get_string(ctx, i);
			argv[argc++] = arg;
		}
		else
		{
			Printf("tried to pass %s to ts", duk_get_string(ctx, i));
		}
	}
	if (nsE->mType == Namespace::Entry::ScriptFunctionType)
	{
		if (nsE->mFunctionOffset)
		{
			duk_push_string(ctx, CodeBlock__exec(
				nsE->mCode, nsE->mFunctionOffset,
				nsE->mNamespace, nsE->mFunctionName, argc, argv,
				false, nsE->mPackage, 0));
			//duk_pop(ctx);
			return 1;
		}
		else
			return 0;
	}
	//Printf("%d, %s, %s, %s", argc, argv[0], argv[1], argv[2]);
	S32 mMinArgs = nsE->mMinArgs;
	S32 mMaxArgs = nsE->mMaxArgs;
	if ((mMinArgs && argc < mMinArgs) || (mMaxArgs && argc > mMaxArgs))
	{
		Printf("Expected args between %d and %d, got %d", mMinArgs, mMaxArgs, argc);
		duk_push_boolean(ctx, false);
		//duk_pop(ctx);
		return 1;
	}
	void *cb = nsE->cb;
	switch (nsE->mType)
	{
	case Namespace::Entry::StringCallbackType:
		duk_push_string(ctx, ((StringCallback)cb)(blah, argc, argv));
		return 1;
	case Namespace::Entry::IntCallbackType:
		duk_push_int(ctx, ((IntCallback)cb)(blah, argc, argv));
		return 1;
	case Namespace::Entry::FloatCallbackType:
		duk_push_int(ctx, ((FloatCallback)cb)(blah, argc, argv));
		return 1;
	case Namespace::Entry::BoolCallbackType:
		duk_push_boolean(ctx, ((BoolCallback)cb)(blah, argc, argv));
		return 1;
	case Namespace::Entry::VoidCallbackType:
		((VoidCallback)cb)(blah, argc, argv);
		return 0;
	}
	duk_pop(ctx);
	return 0;
}

static duk_ret_t duk__ts_obj(duk_context *ctx)
{
	duk_idx_t nargs;

	nargs = duk_get_top(ctx);
	SimObject *obj;
	if (duk_is_number(ctx, 0))
	{
		int id = duk_get_int(ctx, 0);
		obj = Sim__findObject_id(id);
		/*
		Printf("detected int arg, looking up %d", id);
		//check if gc is done
		std::map<int, SimObject**>::iterator it;
		it = garbagec_ids.find(id);
		if (it != garbagec_ids.end())
		{
			SimObject__unregisterReference(obj, garbagec_ids.find(id)->second);
		}*/

	}
	else if(duk_is_string(ctx, 0))
	{
		const char* name = duk_get_string(ctx, 0);
		obj = Sim__findObject_name(name);
		//Printf("detected string arg, looking up %s", name);
		/*
		std::map<int, SimObject**>::iterator it;
		it = garbagec_ids.find(obj->id);
		if (it != garbagec_ids.end())
		{
			SimObject__unregisterReference(obj, garbagec_ids.find(obj->id)->second);
			garbagec_ids.erase(it);
		}*/
	}
	else
	{
		Printf("expected string or int");
		duk_pop(ctx);
		return 0;
	}
	if (obj == NULL)
	{
		duk_error(ctx, DUK_ERR_ERROR, "couldn't find object");
		duk_pop(ctx);
		return 0;
	}
	std::map<int, SimObject**>::iterator it;
	it = garbagec_ids.find(obj->id);
	//if it already exists then just refer back to it
	//idk why i thought i should realloc/unalloc on the fly when we can just refer to it lol
	if (it != garbagec_ids.end())
	{
		SimObject** a = garbagec_ids.find(obj->id)->second;
		//all these damn freaks it's a fucking circus
		Printf("using cached pointer for %d", obj->id);
		duk_push_pointer(ctx, a);
		return 1;
		/*
		Printf("FREEING %d !!!", obj->id);
		SimObject__unregisterReference(obj, a);
		duk_free(ctx, (void*)a);
		garbagec_ids.erase(it);
		*/
	}
	SimObject** ptr = (SimObject**)duk_alloc(ctx, sizeof(SimObject*));
	*ptr = obj;
	SimObject__registerReference(obj, ptr);
	//on detach free all of this shit, seriously
	garbagec_ids.insert(garbagec_ids.end(), std::pair<int, SimObject**>(obj->id, ptr));
	duk_push_pointer(ctx, ptr);
	return 1;
}
/* Removed function. Left here for reference. When code cleanup happens I'll remove it.
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
*/
//stolen from blocklandjs
static const char *ts__js_eval(SimObject *obj, int argc, const char *argv[])
{
	if (argv[1] == NULL)
		return "0";

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
	return "0";
}
//i stole this and rewrote it and made it better ^_^
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
//same for here whatever LOL
static duk_ret_t cb_resolve_module(duk_context *ctx) {
	const char *module_id;
	const char *parent_id;

	module_id = duk_require_string(ctx, 0);
	parent_id = duk_require_string(ctx, 1);

	duk_push_sprintf(ctx, "%s.js", module_id);
	printf("resolve_cb: id:'%s', parent-id:'%s', resolve-to:'%s'\n",
		module_id, parent_id, duk_get_string(ctx, -1));

	return 1;
}
/*
	yeah i got this from test.c from the node module example- i'm going to rewrite it like, tommorow?
	shrug
*/
static duk_ret_t cb_load_module(duk_context *ctx) {
	const char *filename;
	const char *module_id;

	module_id = duk_require_string(ctx, 0);
	duk_get_prop_string(ctx, 2, "filename");
	filename = duk_require_string(ctx, -1);

	Printf("load_cb: id:'%s', filename:'%s'", module_id, filename);

	if (strcmp(module_id, "pig.js") == 0) {
		duk_push_sprintf(ctx, "module.exports = 'you\\'re about to get eaten by %s';",
			module_id);
	}
	else if (strcmp(module_id, "cow.js") == 0) {
		duk_push_string(ctx, "module.exports = require('pig');");
	}
	else if (strcmp(module_id, "ape.js") == 0) {
		duk_push_string(ctx, "module.exports = { module: module, __filename: __filename, wasLoaded: module.loaded };");
	}
	else if (strcmp(module_id, "badger.js") == 0) {
		duk_push_string(ctx, "exports.foo = 123; exports.bar = 234;");
	}
	else if (strcmp(module_id, "comment.js") == 0) {
		duk_push_string(ctx, "exports.foo = 123; exports.bar = 234; // comment");
	}
	else if (strcmp(module_id, "shebang.js") == 0) {
		duk_push_string(ctx, "#!ignored\nexports.foo = 123; exports.bar = 234;");
	}
	else {
		(void)duk_error(ctx, DUK_ERR_ERROR, "cannot find module: %s", module_id);
	}

	return 1;
}
//i think this is from a duktape example, with the MIT license
static duk_ret_t handle_assert(duk_context *ctx) {
	if (duk_to_boolean(ctx, 0)) {
		return 0;
	}
	(void)duk_error(ctx, DUK_ERR_ERROR, "assertion failed: %s", duk_safe_to_string(ctx, 1));
	return 0;
}

//stolen from the initial code, rewritten and fixed up for duktape
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
		duk_push_object(_Context);
		duk_push_c_function(_Context, cb_resolve_module, DUK_VARARGS);
		duk_put_prop_string(_Context, -2, "resolve");
		duk_push_c_function(_Context, cb_load_module, DUK_VARARGS);
		duk_put_prop_string(_Context, -2, "load");
		duk_module_node_init(_Context);
		Printf("top after init: %ld\n", (long)duk_get_top(_Context));
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
		duk_push_c_function(_Context, duk__ts_obj, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_obj");
		duk_push_c_function(_Context, handle_assert, 2);
		duk_put_global_string(_Context, "assert");

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

	push_file_as_string(_Context, "./init.js");
	if (duk_peval(_Context) != 0) {
		Printf("init script error: %s\n", duk_safe_to_string(_Context, -1));
	}

	
	Printf("BlocklandJS | Attached");
	return true;
}

bool deinit()
{
	//delete _Platform; <- this seemed to break it
	for (const auto& kv : garbagec_ids) {
		//free em all up
		Printf("Freeing ID %d", kv.first);
		SimObject__unregisterReference(*kv.second, kv.second);
		duk_free(_Context, kv.second);
	}

	duk_destroy_heap(_Context);
	//free memory by deregistering all references
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
