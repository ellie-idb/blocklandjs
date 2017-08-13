#pragma once

#include <stdlib.h>
#include "torque.h"
#include "duktape.h"
#include "duk_module_node.h"
#include <map>
#include <Windows.h>
#include <thread>
#include <string>

//global context for everything duktape related
static duk_context* _Context;
//static uv_loop_t *loop;
//std::map for storing pointers to function entries
static std::map<const char*, Namespace::Entry*> cache;
//std::map for storing pointers to namespace objects which I have cached
static std::map<const char*, Namespace*> nscache;
//std::map for storing pointers which I have to free upon detach
static std::map<int, SimObject**> garbagec_ids;
//std::map for objects we created that i want to delete >:(
static std::map<int, SimObject**> garbagec_objs;

//GlobalNS is a stored pointer for the namespace which houses all "global" funcs
static Namespace* GlobalNS = NULL;

static std::map<int, std::thread> threads;

//stolen from the duktape example thing, ill rewrite it eventually tm
static void push_file_as_string(duk_context *ctx, const char *filename) {
	FILE* f;
	size_t len;
	char buf[16384];
#pragma warning(disable: 4996)
	//we already check if f exists and i don't want to use fopen_s
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

static duk_ret_t duk__loadfile(duk_context *ctx)
{
	//read the file, return it as a lstring
	push_file_as_string(ctx, duk_require_string(ctx, 0));
	return 1;
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

static duk_ret_t duk__version(duk_context *ctx) {
	duk_push_string(ctx, DUK_GIT_DESCRIBE);
	return 1;
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

static duk_ret_t duk__ts_newObj(duk_context *ctx)
{
	const char* className = duk_require_string(ctx, 0);
	SimObject* obj = (SimObject*)AbstractClassRep_create_className(className);
	//register it here

	if (obj == NULL)
	{
		return 0;
	}
	SimObject** ptr = (SimObject**)duk_alloc(ctx, sizeof(SimObject*));
	*ptr = obj;
	SimObject__registerReference(obj, ptr);
	//we can't insert in the garbage collection until it has been registered..
	obj->mFlags |= SimObject::ModStaticFields;
	obj->mFlags |= SimObject::ModDynamicFields;
	duk_push_pointer(ctx, ptr);
	return 1;
}

static duk_ret_t duk__ts_registerObject(duk_context *ctx)
{
	SimObject** simRef = (SimObject**)duk_require_pointer(ctx, 0);
	SimObject* simObj = *simRef;
	SimObject__registerObject(simObj);
	//do it after ts has registered it and given us an id
	//make sure that we're not inserting over something else
	std::map<int, SimObject**>::iterator it;
	it = garbagec_objs.find(simObj->id);
	if (it == garbagec_objs.end())
	{
		garbagec_objs.insert(garbagec_objs.end(), std::make_pair(simObj->id, simRef));
	}
	return 0;
}

static duk_ret_t duk__ts_setObjectField(duk_context *ctx)
{
	SimObject** b = (SimObject**)duk_require_pointer(ctx, 0);
	SimObject* a = *b;
	if (!a)
	{
		Printf("Null pointer to object");
		return 0;
	}
	const char* dataf = duk_get_string(ctx, 1);
	const char* val = 0;
	std::string str;

	switch (duk_get_type(ctx, 2))
	{
	case DUK_TYPE_POINTER:
	{
		Printf("Second parameter cannot be a pointer");
		return 0;
	}
	case DUK_TYPE_NUMBER:
	{
		duk_double_t arg = duk_get_number(ctx, 2);
		str = std::to_string(arg);
		val = str.c_str();
		break;
	}
	case DUK_TYPE_STRING:
	{
		val = duk_get_string(ctx, 2);
		break;
	}
	case DUK_TYPE_BOOLEAN:
	{
		duk_bool_t arg = duk_get_boolean(ctx, 2);
		str = arg ? "1" : "0";
		val = str.c_str();
		break;
	}
	default:
		Printf("tried to pass %s to ts", duk_get_string(ctx, 2));
		return 0;
	}

	SimObject__setDataField(a, dataf, StringTableEntry(""), StringTableEntry(val));
	return 0;
}

static duk_ret_t duk__ts_getObjectField(duk_context *ctx)
{
	SimObject** b = (SimObject**)duk_require_pointer(ctx, 0);
	SimObject* a = *b;
	if (!a)
	{
		Printf("Null pointer to object");
		return 0;
	}
	const char* dataf = duk_require_string(ctx, 1);

	const char* result = SimObject__getDataField(a, dataf, StringTableEntry(""));
	Printf("%s", result);
	duk_push_string(ctx, result);
	return 1;
}

static duk_ret_t duk__ts_setVariable(duk_context *ctx)
{
	const char* dataf = duk_require_string(ctx, 0);
	const char* val = duk_get_string(ctx, 1);
	SetGlobalVariable(dataf, val);
	return 0;
}

static duk_ret_t duk__ts_getVariable(duk_context *ctx)
{
	const char* name = duk_require_string(ctx, 0);
	const char* res = GetGlobalVariable(name);
	duk_push_string(ctx, res);
	return 1;
}
//a large part of this was taken from blocklandjs, thank you port for all the wonderful code <3
static duk_ret_t duk__ts_handlefunc(duk_context *ctx)
{
	duk_idx_t nargs;
	nargs = duk_get_top(ctx) - 1;
	if (nargs > 19)
		duk_error(ctx, DUK_ERR_ERROR, "too many args for ts");

	const char* tsns = duk_require_string(ctx, 0);
	const char* fnName = duk_require_string(ctx, 1);
	Namespace *ns;
	Namespace::Entry *nsE;
	SimObject* obj = NULL;
	int argc = 0;
	const char* argv[21];
	//Printf("args: %d", nargs);

	if (tsns == NULL || fnName == NULL)
	{
		duk_error(ctx, DUK_ERR_ERROR, "namespace/function name were null upon checking!!");
		return 0;
	}

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
		std::map<const char*, Namespace*>::iterator its;
		auto dumb = tsns;
		its = nscache.find(dumb);
		if(its != nscache.end())
		{
			ns = nscache.find(dumb)->second;
			if (ns == NULL)
			{
				//look it up the old way
				ns = LookupNamespace(tsns);
			}
		}
		else
		{
			ns = LookupNamespace(tsns);
			if (ns != NULL)
			{
				nscache.insert(nscache.end(), std::make_pair(dumb, ns));
			}
			else
			{
				duk_error(ctx, DUK_ERR_ERROR, "namespace lookup fail");
				return 0;
			}
		}
	}

	std::map<const char*, Namespace::Entry*>::iterator it;
	it = cache.find(fnName);
	if (it == cache.end())
	{
		nsE = Namespace__lookup(ns, StringTableEntry(fnName));
	}
	else
	{
		nsE = cache.find(fnName)->second;
		if (nsE == NULL)
		{
			//try the normal way, cache missed :(
			nsE = Namespace__lookup(ns, StringTableEntry(fnName));
		}
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
		cache.insert(cache.end(), std::make_pair(fnName, nsE));
	}
	//set up arrays for passing to tork
	argv[argc++] = nsE->mFunctionName;
	if (duk_is_pointer(ctx, 2))
	{
		//it's a thiscall
		SimObject** ref = (SimObject**)duk_get_pointer(ctx, 2);
		//duk_pop(ctx);
		obj = *ref;
		if (obj == NULL)
		{
			duk_error(ctx, DUK_ERR_ERROR, "Expected valid reference to object, got null.");
			//duk_pop(ctx);
			return 0;
		}
		char idbuf[sizeof(int) * 3 + 2];
		snprintf(idbuf, sizeof idbuf, "%d", obj->id);
		argv[argc++] = StringTableEntry(idbuf);
	}
	std::string a;
	for (int i = 2; i <= nargs; i++)
	{
		switch (duk_get_type(ctx, i))
		{
		case DUK_TYPE_POINTER:
		{
			//don't look back at the same ptr we just skipped over
			if (i != 2)
			{
				SimObject** a = (SimObject**)duk_get_pointer(ctx, i);
				SimObject* ref = *a;
				SimObjectId id;
				if (ref != NULL)
				{
					id = ref->id;
					//Printf("ID: %d", id);
				}
				else
				{
					id = 0;
					//Printf("found invalid reference :(");
				}

				char idbuf[sizeof(int) * 3 + 2];
				snprintf(idbuf, sizeof idbuf, "%d", id);
				argv[argc++] = StringTableEntry(idbuf);
			}
			break;
		}
		case DUK_TYPE_NUMBER:
		{
			duk_double_t arg = duk_get_number(ctx, i);
			//Printf("argv %d for argc %d", arg, argc + 1);
			a = std::to_string(arg);
			argv[argc++] = a.c_str();
			break;
		}
		case DUK_TYPE_STRING:
		{
			const char* arg = duk_get_string(ctx, i);
			//Printf("argv %s for argc %d", arg, argc + 1);
			argv[argc++] = arg;
			break;
		}
		case DUK_TYPE_BOOLEAN:
		{
			const char* arg = duk_get_string(ctx, i);
			argv[argc++] = arg ? "1" : "0";
			break;
		}
		default:
			Printf("tried to pass %s to ts", duk_get_string(ctx, i));
			break;
		}
		//duk_pop(ctx);
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
	//Printf("argc: %d", argc);
	switch (nsE->mType)
	{
	case Namespace::Entry::StringCallbackType:
		duk_push_string(ctx, nsE->cb.mStringCallbackFunc(obj, argc, argv));
		return 1;
	case Namespace::Entry::IntCallbackType:
		duk_push_int(ctx, nsE->cb.mIntCallbackFunc(obj, argc, argv));
		return 1;
	case Namespace::Entry::FloatCallbackType:
		duk_push_number(ctx, nsE->cb.mFloatCallbackFunc(obj, argc, argv));
		return 1;
	case Namespace::Entry::BoolCallbackType:
		duk_push_boolean(ctx, nsE->cb.mBoolCallbackFunc(obj, argc, argv));
		return 1;
	case Namespace::Entry::VoidCallbackType:
		nsE->cb.mVoidCallbackFunc(obj, argc, argv);
		return 0;
	}
	//duk_pop(ctx);
	return 0;
}

static duk_ret_t duk__ts_obj(duk_context *ctx)
{
	duk_idx_t nargs;

	nargs = duk_get_top(ctx);
	SimObject *obj;
	switch (duk_get_type(ctx, 0))
	{
		case DUK_TYPE_NUMBER:
		{
			int id = duk_get_int(ctx, 0);
			obj = Sim__findObject_id(id);
			break;
		}

		case DUK_TYPE_STRING:
		{
			const char* name = duk_get_string(ctx, 0);
			obj = Sim__findObject_name(name);
			break;
		}

		default:
		{
			Printf("expected string or int");
			//duk_pop(ctx);
			return 0;
		}
	}
	if (obj == NULL)
	{
		duk_error(ctx, DUK_ERR_ERROR, "couldn't find object");
		//duk_pop(ctx);
		return 0;
	}
	std::map<int, SimObject**>::iterator it;
	it = garbagec_ids.find(obj->id);
	if (it != garbagec_ids.end())
	{
		SimObject** a = garbagec_ids.find(obj->id)->second;
		Printf("using cached pointer for %d", obj->id);
		if (a == NULL)
		{
			Printf("CACHE MISS!!!");
			goto CacheMiss;
		}
		duk_push_pointer(ctx, a);
		return 1;
	}
	CacheMiss:
	SimObject** ptr = (SimObject**)duk_alloc(ctx, sizeof(SimObject*));
	*ptr = obj;
	SimObject__registerReference(obj, ptr);
	garbagec_ids.insert(garbagec_ids.end(), std::make_pair(obj->id, ptr));
	duk_push_pointer(ctx, ptr);
	return 1;
}
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
	//duk_pop(_Context);
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
		Printf("load error: %s\n", duk_safe_to_string(_Context, -1));
	}
	else
	{
		return duk_get_string(_Context, -1);
	}
	//duk_pop(_Context);
	return "";
}

static const char *ts__js_version(SimObject* obj, int argc, const char* argv[])
{
	return DUK_GIT_DESCRIBE;
}
//same for here whatever LOL
//but i'm weak, what's wrong with that?
static duk_ret_t duk__ts_getMethods(duk_context *ctx) {
	std::map<const char*, Namespace*>::iterator it;
	auto ns = duk_require_string(ctx, 0);
	it = nscache.find(ns);
	Namespace* a;
	if (it != nscache.end())
	{
		a = nscache.find(ns)->second;
	}
	else
	{
		a = LookupNamespace(ns);
		if (a == NULL)
		{
			return 0;
		}
	}
	auto ret = duk_push_array(ctx);
	auto i = 0;
	for (; a; a = a->mParent)
	{
		for (auto walk = a->mEntryList; walk; walk = walk->mNext)
		{
			//does this even work
			auto funcName = walk->mFunctionName;
			auto etype = walk->mType;
			if (etype >= Namespace::Entry::ScriptFunctionType || etype == Namespace::Entry::OverloadMarker)
			{
				if (etype == Namespace::Entry::OverloadMarker)
				{
					etype = 8;
					for (Namespace::Entry* eseek = a->mEntryList; eseek; eseek = eseek->mNext)
					{
						auto fnnamest = walk->cb.mGroupName;
						if (!_stricmp(eseek->mFunctionName, fnnamest))
						{
							etype = eseek->mType;
							break;
						}
					}
					funcName = walk->cb.mGroupName;
				}
				duk_push_string(ctx, funcName);
				duk_put_prop_index(ctx, ret, i);
				++i;
			}
		}
	}
	return 1;
}
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
	actually update this is probably going to come later tm
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

	//Initialize Duktape lol
	Printf("Initializing Duktape");
//	threads.insert(threads.end(), std::pair<int, std::thread>(1, std::thread(loop)));
	_Context = duk_create_heap_default();
	if (_Context)
	{
		duk_push_object(_Context);
		duk_push_c_function(_Context, cb_resolve_module, DUK_VARARGS);
		duk_put_prop_string(_Context, -2, "resolve");
		duk_push_c_function(_Context, cb_load_module, DUK_VARARGS);
		duk_put_prop_string(_Context, -2, "load");
		duk_module_node_init(_Context);
		Printf("top after init: %ld\n", (long)duk_get_top(_Context));
		duk_push_c_function(_Context, duk__print, DUK_VARARGS);
		duk_put_global_string(_Context, "print");
		duk_push_c_function(_Context, duk__ts_eval, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_eval");
		duk_push_c_function(_Context, duk__ts_handlefunc, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_call");
		duk_push_c_function(_Context, duk__ts_obj, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_obj");
		duk_push_c_function(_Context, duk__loadfile, DUK_VARARGS);
		duk_put_global_string(_Context, "load");
		duk_push_c_function(_Context, duk__ts_newObj, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_newObj");
		duk_push_c_function(_Context, duk__version, DUK_VARARGS);
		duk_put_global_string(_Context, "version");
		duk_push_c_function(_Context, duk__ts_registerObject, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_registerObject");
		duk_push_c_function(_Context, duk__ts_getObjectField, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_getObjectField");
		duk_push_c_function(_Context, duk__ts_setObjectField, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_setObjectField");
		duk_push_c_function(_Context, duk__ts_getVariable, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_getVariable");
		duk_push_c_function(_Context, duk__ts_setVariable, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_setVariable");
		duk_push_c_function(_Context, duk__ts_getMethods, DUK_VARARGS);
		duk_put_global_string(_Context, "ts_getMethods");
		duk_push_c_function(_Context, handle_assert, 2);
		duk_put_global_string(_Context, "assert");
	}
	else
	{
		return 0;
	}

	//TorqueScript functions
	ConsoleFunction(NULL, "js_eval", ts__js_eval,
		"js_eval(string) - evaluates a javascript string and returns the result", 2, 2);

	ConsoleFunction(NULL, "js_load", ts__js_load,
		"js_load(path to file) - loads a javascript file, evals it, returns the result", 2, 2);

	ConsoleFunction(NULL, "js_version", ts__js_version,
		"js_version() - returns the current duktape version used", 1, 1);

	push_file_as_string(_Context, "./init.js");
	if (duk_peval(_Context) != 0) {
		Printf("init script error: %s\n", duk_safe_to_string(_Context, -1));
	}

	//loop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
	//uv_loop_init(loop);
	//uv_run(loop, UV_RUN_DEFAULT);
	//threads.insert(std::pair<int, std::thread>(1, std::thread(uv_run(loop, UV_RUN_DEFAULT))));

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

	for (const auto& ab : garbagec_objs) {
		Printf("Freeing and deleting ID %d", ab.first);
		//eh there are cleaner ways to do it
		//fuk that tho
		//a
		//SimObject__delete(*ab.second);
		SimObject__unregisterReference(*ab.second, ab.second);
	}

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
