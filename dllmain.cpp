#define _WIN32 1
//If I could keep you here
//One more weekend~!
#include <stdlib.h>
#include "torque.h"
#include "lib/duktape/src/duktape.h"
#include <map>
#include <windows.h>
#include <thread>
#include <string>
extern "C" {
#include "duk_module_node.h"
#include "lib/dukluv/src/duv.h"
#include "lib/dukluv/src/refs.h"
#include "lib/dukluv/src/utils.h"
#include "lib/dukluv/src/loop.h"
#include "lib/dukluv/src/req.h"
#include "lib/dukluv/src/handle.h"
#include "lib/dukluv/src/timer.h"
#include "lib/dukluv/src/stream.h"
#include "lib/dukluv/src/tcp.h"
#include "lib/dukluv/src/pipe.h"
#include "lib/dukluv/src/tty.h"
#include "lib/dukluv/src/fs.h"
#include "lib/dukluv/src/misc.h"
#include "lib/dukluv/src/miniz.h"
}
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
//std::map for stuffz
static std::map<int, duk_idx_t> mapping;

//GlobalNS is a stored pointer for the namespace which houses all "global" funcs
static Namespace* GlobalNS = NULL;

static uv_loop_t loop;

static std::map<int, std::thread> threads;

static void duv_dump_error(duk_context *ctx, duk_idx_t idx) {
  fprintf(stderr, "\nUncaught Exception:\n");
  if (duk_is_object(ctx, idx)) {
    duk_get_prop_string(ctx, -1, "stack");
    fprintf(stderr, "\n%s\n\n", duk_get_string(ctx, -1));
    duk_pop(ctx);
  }
  else {
    fprintf(stderr, "\nThrown Value: %s\n\n", duk_json_encode(ctx, idx));
  }
}


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

static duk_ret_t duk__ts_registerCallback(duk_context *ctx) 
{
	const char* var1 = duk_require_string(ctx, 0);
	const char* var2 = duk_require_string(ctx, 1);
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
//i think this is from a duktape example, with the MIT license
static duk_ret_t handle_assert(duk_context *ctx) {
	if (duk_to_boolean(ctx, 0)) {
		return 0;
	}
	(void)duk_error(ctx, DUK_ERR_ERROR, "assertion failed: %s", duk_safe_to_string(ctx, 1));
	return 0;
}

static duk_ret_t duv_loadfile(duk_context *ctx) {
  const char* path = duk_require_string(ctx, 0);
  uv_fs_t req;
  int fd = 0;
  uint64_t size;
  char* chunk;
  uv_buf_t buf;

  if (uv_fs_open(&loop, &req, path, O_RDONLY, 0644, NULL) < 0) goto fail;
  uv_fs_req_cleanup(&req);
  fd = req.result;
  if (uv_fs_fstat(&loop, &req, fd, NULL) < 0) goto fail;
  uv_fs_req_cleanup(&req);
  size = req.statbuf.st_size;
  chunk = (char*)duk_alloc(ctx, size);
  buf = uv_buf_init(chunk, size);
  if (uv_fs_read(&loop, &req, fd, &buf, 1, 0, NULL) < 0) {
    duk_free(ctx, chunk);
    goto fail;
  }
  uv_fs_req_cleanup(&req);
  duk_push_lstring(ctx, chunk, size);
  duk_free(ctx, chunk);
  uv_fs_close(&loop, &req, fd, NULL);
  uv_fs_req_cleanup(&req);

  return 1;

  fail:
  uv_fs_req_cleanup(&req);
  if (fd) uv_fs_close(&loop, &req, fd, NULL);
  uv_fs_req_cleanup(&req);
  duk_error(ctx, DUK_ERR_ERROR, "%s: %s: %s", uv_err_name(req.result), uv_strerror(req.result), path);
}

struct duv_list {
  const char* part;
  int offset;
  int length;
  struct duv_list* next;
};
typedef struct duv_list duv_list_t;

static duv_list_t* duv_list_node(const char* part, int start, int end, duv_list_t* next) {
  duv_list_t *node = (duv_list_t*)malloc(sizeof(*node));
  node->part = part;
  node->offset = start;
  node->length = end - start;
  node->next = next;
  return node;
}

static duk_ret_t duv_path_join(duk_context *ctx) {
  duv_list_t *list = NULL;
  int absolute = 0;

  // Walk through all the args and split into a linked list
  // of segments
  {
    // Scan backwards looking for the the last absolute positioned path.
    int top = duk_get_top(ctx);
    int i = top - 1;
    while (i > 0) {
      const char* part = duk_require_string(ctx, i);
      if (part[0] == '\\') break;
      i--;
    }
    for (; i < top; ++i) {
      const char* part = duk_require_string(ctx, i);
      int j;
      int start = 0;
      int length = strlen(part);
      if (part[0] == '\\') {
        absolute = 1;
      }
      while (start < length && part[start] == 0x2f) { ++start; }
      for (j = start; j < length; ++j) {
        if (part[j] == 0x2f) {
          if (start < j) {
            list = duv_list_node(part, start, j, list);
            start = j;
            while (start < length && part[start] == 0x2f) { ++start; }
          }
        }
      }
      if (start < j) {
        list = duv_list_node(part, start, j, list);
      }
    }
  }

  // Run through the list in reverse evaluating "." and ".." segments.
  {
    int skip = 0;
    duv_list_t *prev = NULL;
    while (list) {
      duv_list_t *node = list;

      // Ignore segments with "."
      if (node->length == 1 &&
          node->part[node->offset] == 0x2e) {
        goto skip;
      }

      // Ignore segments with ".." and grow the skip count
      if (node->length == 2 &&
          node->part[node->offset] == 0x2e &&
          node->part[node->offset + 1] == 0x2e) {
        ++skip;
        goto skip;
      }

      // Consume the skip count
      if (skip > 0) {
        --skip;
        goto skip;
      }

      list = node->next;
      node->next = prev;
      prev = node;
      continue;

      skip:
        list = node->next;
        free(node);
    }
    list = prev;
  }

  // Merge the list into a single `/` delimited string.
  // Free the remaining list nodes.
  {
    int count = 0;
    if (absolute) {
      duk_push_string(ctx, "\\");
      ++count;
    }
    while (list) {
      duv_list_t *node = list;
      duk_push_lstring(ctx, node->part + node->offset, node->length);
      ++count;
      if (node->next) {
        duk_push_string(ctx, "\\");
        ++count;
      }
      list = node->next;
      free(node);
    }
    duk_concat(ctx, count);
  }
  return 1;
}

void duk_dump_context_stdout(duk_context *ctx) {
	duk_push_context_dump(ctx);
	Printf("%s\n", duk_to_string(ctx, -1));
	duk_pop(ctx);
}

static duk_ret_t cb_resolve_module(duk_context *ctx) {
  const char* modID = duk_require_string(ctx, 0);
  const char* parentID = duk_require_string(ctx, 1);

  if(!_stricmp(parentID, ""))
  {
  	duk_push_c_function(ctx, duv_cwd, 0);
	duk_call(ctx, 0);
	duk_size_t len;
	const char* cwd = duk_get_lstring(ctx, -1, &len);
	duk_pop(ctx);
	duk_push_c_function(ctx, duv_path_join, DUK_VARARGS);
	duk_push_lstring(ctx, cwd, len);
	duk_push_string(ctx, modID);
	duk_call(ctx, 2);
	const char* resolvedID = duk_get_string(ctx, -1);
	return 1;
  }
  else {
 	 duk_push_c_function(ctx, duv_path_join, DUK_VARARGS); //3
     duk_push_string(ctx, parentID); //2
     duk_push_string(ctx, ".."); //1
     duk_push_string(ctx, modID); //0
     duk_call(ctx, 3);
     return 1;
  }

  return 0;
}

static duk_ret_t cb_load_module(duk_context *ctx) {
  const char* id;
  const char* fileName;

  id = duk_require_string(ctx, 0);

  duk_get_prop_string(ctx, 2, "filename");
  fileName = duk_require_string(ctx, -1);
  // duk_push_string(ctx, "module.exports = 'hello';");
  push_file_as_string(ctx, fileName);
  return 1;
}

static duk_ret_t duv_main(duk_context *ctx) {
  duk_push_global_object(ctx);
  duk_dup(ctx, -1);
  duk_put_prop_string(ctx, -2, "global");

  duk_push_boolean(ctx, 1);
  duk_put_prop_string(ctx, -2, "dukluv");

  // Load duv module into global uv
  duk_push_c_function(ctx, dukopen_uv, 0);
  duk_call(ctx, 0);
  duk_put_prop_string(ctx, -2, "uv");

  duk_push_c_function(ctx, duv_path_join, DUK_VARARGS);
  duk_put_prop_string(ctx, -2, "pathJoin");

  duk_push_c_function(ctx, duv_loadfile, 1);
  duk_put_prop_string(ctx, -2, "loadFile");
  //duk_call_method(ctx, 1);
  duk_pop(ctx); //Gotta remove the spacer..
  uv_run(&loop, UV_RUN_DEFAULT);

return 0;
}

bool init()
{
	if (!torque_init())
		return false;

	uv_loop_init(&loop);
	//Initialize Duktape lol
	Printf("Initializing Duktape");
	_Context = duk_create_heap(NULL, NULL, NULL, &loop, NULL);
	if (_Context)
	{
		Printf("Binding all C++ functions to JS..");
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
		loop.data = _Context;
		Printf("Initializing dukluv subsystem..");
		duk_push_c_function(_Context, duv_main, 1);
		duk_push_string(_Context, "spacer");
		if(duk_pcall(_Context, 1))
		{
			Printf("Error in duv_main!");
			duv_dump_error(_Context, -1);
			uv_loop_close(&loop);
			duk_destroy_heap(_Context);
			return 0;
		}
		Printf("Setting up node.js-like module loading..");
		duk_push_object(_Context);
		duk_push_c_function(_Context, cb_resolve_module, DUK_VARARGS);
		duk_put_prop_string(_Context, -2, "resolve");
		duk_push_c_function(_Context, cb_load_module, DUK_VARARGS);
		duk_put_prop_string(_Context, -2, "load");
		duk_module_node_init(_Context);
	//Now setup our node module loading system.
	Printf("top after init %ld", (long)duk_get_top(_Context));
		//TorqueScript functions
	ConsoleFunction(NULL, "js_eval", ts__js_eval,
		"js_eval(string) - evaluates a javascript string and returns the result", 2, 2);

	ConsoleFunction(NULL, "js_load", ts__js_load,
		"js_load(path to file) - loads a javascript file, evals it, returns the result", 2, 2);

	ConsoleFunction(NULL, "js_version", ts__js_version,
		"js_version() - returns the current duktape version used", 1, 1);

	Printf("Loading JS init script..");

	duk_push_c_function(_Context, duv_cwd, 0);
	duk_call(_Context, 0);
	duk_size_t len;
	const char* cwd = duk_get_lstring(_Context, -1, &len); //Get our current working directory (where the EXE is.)
	const char* initScript;
	duk_pop(_Context); //Pop the CWD off the stack..
	duk_push_c_function(_Context, duv_path_join, DUK_VARARGS);
	duk_push_lstring(_Context, cwd, len);
	duk_push_string(_Context, "init.js");
	duk_call(_Context, 2); //Now append our CWD with init.js
	initScript = duk_get_string(_Context, -1); //The directory of the init.js..
	duk_pop_2(_Context);

	push_file_as_string(_Context, initScript);
	if (duk_module_node_peval_main(_Context, initScript) != 0) {
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

	for (const auto& ab : garbagec_objs) {
		Printf("Freeing and deleting ID %d", ab.first);
		//eh there are cleaner ways to do it
		//fuk that tho
		//a
		//SimObject__delete(*ab.second);
		SimObject__unregisterReference(*ab.second, ab.second);
	}
	uv_loop_close(&loop);
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
