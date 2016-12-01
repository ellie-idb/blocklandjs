#pragma comment(lib, "v8_nosnapshot.lib")
#pragma comment(lib, "icui18n.dll.lib")
#pragma comment(lib, "icuuc.dll.lib")
#pragma comment(lib, "v8_libsampler.lib")
#pragma comment(lib, "v8_libbase.dll.lib")
#pragma comment(lib, "v8.dll.lib")
#pragma comment(lib, "v8_libplatform.lib")
#pragma comment(lib, "v8_base.lib")

#include <cstdint>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <vector>

#include "Torque.h"
#include "MologieDetours/detours.h"
#include "include/v8-inspector.h"
#include "include/v8.h"
#include "libplatform/libplatform.h"

using namespace v8;

Platform *_Platform;
Isolate *_Isolate;
Persistent<Context> _Context;

Persistent<Context> tempObjContext;
Handle<ObjectTemplate> globall;
Persistent<ObjectTemplate> glo;

bool istrue(const char *arg)
{
	if (arg == NULL)
		return false;
	return !_stricmp(arg, "true") || !_stricmp(arg, "1") || (0 != atoi(arg));
}

template <class TypeName>
inline v8::Local<TypeName> StrongPersistentTL(
	const v8::Persistent<TypeName>& persistent)
{
	return *reinterpret_cast<v8::Local<TypeName>*>(
		const_cast<v8::Persistent<TypeName>*>(&persistent));
}

template <class TypeName>
inline v8::Local<TypeName> StrongPersistentTL(const v8::Persistent<TypeName>& persistent);

v8::Local<v8::Context> ContextL()
{
	return StrongPersistentTL(_Context);
}

const char *ToCString(const String::Utf8Value &value)
{
	char *str = (char *)malloc(strlen(*value) + 1);
	strcpy(str, *value);
	return str == NULL ? "" : str;
}

bool ReportException(Isolate *isolate, TryCatch *try_catch)
{
	HandleScope handle_scope(isolate);
	String::Utf8Value exception(try_catch->Exception());
	Local<Message> message = try_catch->Message();

	if (message.IsEmpty())
		Printf("%s", ToCString(exception));
	else
	{
		String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
		Local<Context> context(isolate->GetCurrentContext());
		int linenum = message->GetLineNumber(context).FromJust();

		Printf("%s:%i: %s", ToCString(filename), linenum, ToCString(exception));

		String::Utf8Value sourceline(message->GetSourceLine(context).ToLocalChecked());
		Printf("%s", ToCString(sourceline));

		std::stringstream ss;
		int start = message->GetStartColumn(context).FromJust();
		for (int i = 0; i < start; i++)
			ss << " ";

		int end = message->GetEndColumn(context).FromJust();
		for (int i = start; i < end; i++)
			ss << "^";

		Printf("%s", ss.str().c_str());

		Local<Value> stack_trace_string;
		if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
			stack_trace_string->IsString() &&
			Local<String>::Cast(stack_trace_string)->Length() > 0)
		{
			String::Utf8Value stack_trace(stack_trace_string);
			Printf("%s", ToCString(stack_trace));
		}
	}
	return true;
}

bool ExecuteString(Isolate* isolate, Local<String> source,
	Local<Value> name, bool print_result,
	bool report_exceptions)
{
	HandleScope handle_scope(isolate);
	TryCatch try_catch(isolate);
	ScriptOrigin origin(name);
	Local<Context> context(isolate->GetCurrentContext());
	Local<Script> script;
	if (!Script::Compile(context, source, &origin).ToLocal(&script))
	{
		// Print errors that happened during compilation.
		if (report_exceptions)
			ReportException(isolate, &try_catch);
		return false;
	}
	else
	{
		Local<Value> result;
		if (!script->Run(context).ToLocal(&result))
		{
			assert(try_catch.HasCaught());
			// Print errors that happened during execution.
			if (report_exceptions)
				ReportException(isolate, &try_catch);
			return false;
		}
		else {
			assert(!try_catch.HasCaught());
			if (print_result && !result->IsUndefined())
			{
				// If all went well and the result wasn't undefined then print
				// the returned value.
				Printf("%s", ToCString(String::Utf8Value(result)));
			}
			return true;
		}
	}
}

MaybeLocal<String> ReadFile(Isolate* isolate, const char* name)
{
	FILE* file = fopen(name, "rb");
	if (file == NULL)
		return MaybeLocal<v8::String>();

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (size_t i = 0; i < size;)
	{
		i += fread(&chars[i], 1, size - i, file);
		if (ferror(file))
		{
			fclose(file);
			return MaybeLocal<String>();
		}
	}

	fclose(file);
	MaybeLocal<String> result = String::NewFromUtf8(
		isolate, chars, NewStringType::kNormal, static_cast<int>(size));
	delete[] chars;
	return result;
}

void Read(const FunctionCallbackInfo<Value>& args)
{
	if (args.Length() != 1)
	{
		args.GetIsolate()->ThrowException(
			String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
				NewStringType::kNormal).ToLocalChecked());
		return;
	}

	String::Utf8Value file(args[0]);
	if (*file == NULL)
	{
		args.GetIsolate()->ThrowException(
			String::NewFromUtf8(args.GetIsolate(), "Error loading file",
				NewStringType::kNormal).ToLocalChecked());
		return;
	}

	Local<String> source;
	if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source))
	{
		args.GetIsolate()->ThrowException(
			String::NewFromUtf8(args.GetIsolate(), "Error loading file",
				NewStringType::kNormal).ToLocalChecked());
		return;
	}
	args.GetReturnValue().Set(source);
}

void Load(const FunctionCallbackInfo<Value>& args)
{
	for (int i = 0; i < args.Length(); i++)
	{
		HandleScope handle_scope(args.GetIsolate());
		String::Utf8Value file(args[i]);
		if (*file == NULL)
		{
			args.GetIsolate()->ThrowException(
				String::NewFromUtf8(args.GetIsolate(), "Error loading file",
					NewStringType::kNormal).ToLocalChecked());
			return;
		}

		Local<String> source;
		if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source))
		{
			args.GetIsolate()->ThrowException(
				String::NewFromUtf8(args.GetIsolate(), "Error loading file",
					NewStringType::kNormal).ToLocalChecked());
			return;
		}

		if (!ExecuteString(args.GetIsolate(), source, args[i], false, false))
		{
			args.GetIsolate()->ThrowException(
				String::NewFromUtf8(args.GetIsolate(), "Error executing file",
					NewStringType::kNormal).ToLocalChecked());
			return;
		}
	}
}
//Support functions for js_func

void js_handleFunc(const FunctionCallbackInfo<Value> &args)
{
	//We have to do the resolving again.
	Namespace *ns;
	Namespace::Entry *nsE;
	char* callee = *String::Utf8Value(args.Callee()->GetName()->ToString());
	callee = strtok(callee, "__");
	const char* nameSpace = callee;
	if (!_stricmp(callee, "ts"))
	{
		ns = LookupNamespace(NULL);
	}
	else
	{
		ns = LookupNamespace(callee);
	}
	callee = strtok(NULL, "__");
	const char* function = callee;
	nsE = Namespace__lookup(ns, StringTableEntry(function));
	if (nsE == NULL)
	{
		return args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, "ERROR: Namespace lookup failed"));
	}
	int argc = 0;
	const char *argv[21];
	argv[argc++] = nsE->mFunctionName;
	SimObject *obj = NULL;
	for (int i = 0; i < args.Length(); i++)
	{
		if (_stricmp(*String::Utf8Value(args[i]), ""))
		{
			argv[argc++] = StringTableEntry(*String::Utf8Value(args[i]));
		}
	}
	if (nsE->mType == Namespace::Entry::ScriptFunctionType)
	{
		if (nsE->mFunctionOffset)
		{
			args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, CodeBlock__exec(
				nsE->mCode, nsE->mFunctionOffset,
				nsE->mNamespace, nsE->mFunctionName, argc, argv,
				false, nsE->mPackage, 0)));
		}
		else
			args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, ""));

		return;
	}
	S32 mMinArgs = nsE->mMinArgs;
	S32 mMaxArgs = nsE->mMaxArgs;
	if ((mMinArgs && argc < mMinArgs) || (mMaxArgs && argc > mMaxArgs))
	{
		args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, "ERROR: Invalid amount of arguments to pass to torquescript."));
		return;
	}
	void *cb = nsE->cb;
	switch (nsE->mType)
	{
	case Namespace::Entry::StringCallbackType:
		args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, ((StringCallback)cb)(obj, argc, argv)));
		return;
	case Namespace::Entry::IntCallbackType:
		args.GetReturnValue().Set(Integer::New(_Isolate, ((IntCallback)cb)(obj, argc, argv)));
		return;
	case Namespace::Entry::FloatCallbackType:
		args.GetReturnValue().Set(Integer::New(_Isolate, ((FloatCallback)cb)(obj, argc, argv)));
		return;
	case Namespace::Entry::BoolCallbackType:
		args.GetReturnValue().Set(Boolean::New(_Isolate, ((BoolCallback)cb)(obj, argc, argv)));
		return;
	case Namespace::Entry::VoidCallbackType:
		((VoidCallback)cb)(obj, argc, argv);
		return;
	}
	return args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, "ERROR: Invalid function."));
	/*
	Namespace *ns;
	Namespace::Entry *nsEn;
	if (stuff == "null")
	{
	//doesn't have a namespace
	ns = LookupNamespace(NULL);
	Namespace__lookup(ns, stuff);
	}
	*/
}


//JavaScript Functions
void js_print(const FunctionCallbackInfo<Value> &args)
{
	std::stringstream str;

	for (int i = 0; i < args.Length(); i++)
		str << ToCString(String::Utf8Value(args[i]));

	Printf("%s", str.str().c_str());
}

void js_eval(const FunctionCallbackInfo<Value> &args)
{
	std::stringstream str;

	for (int i = 0; i < args.Length(); i++)
		str << ToCString(String::Utf8Value(args[i]));

	args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, Eval(str.str().c_str())));
}

void js_call(const FunctionCallbackInfo<Value> &args)
{
	int argc = args.Length();
	if (argc > 21)
		argc = 21;

	const char *strobj = ToCString(String::Utf8Value(args[0]));

	if (_stricmp(strobj, "") != 0)
	{
		SimObject *obj;
		obj = Sim__findObject_id(atoi(strobj));
		if (obj == NULL)
			obj = Sim__findObject_name(strobj);

		if (obj == NULL)
			return;

		const char *argv[21];

		char idbuf[16];
		sprintf(idbuf, "%d", obj->id);
		argv[1] = idbuf;

		argv[0] = ToCString(String::Utf8Value(args[1]));

		for (int i = 2; i < argc; i++)
			argv[i] = ToCString(String::Utf8Value(args[i]));

		const char *res = ObjCall(obj, argc, argv);
		if (res != NULL)
			args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, res));
	}
	else
	{
		const char *argv[21];

		for (int i = 1; i < argc; i++)
			argv[i - 1] = ToCString(String::Utf8Value(args[i]));

		const char *res = Call(argc - 1, argv);
		if (res != NULL)
			args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, res));
	}
}

void AddFunction(const char *name, FunctionCallback func)
{
	// Get global template
	Local<ObjectTemplate> global = Local<ObjectTemplate>::New(_Isolate, StrongPersistentTL(glo));

	// Add new function to it
	global->Set(String::NewFromUtf8(_Isolate, name), FunctionTemplate::New(_Isolate, func));
}

void js_func(const FunctionCallbackInfo<Value> &args)
{
	Namespace *ns;
	Namespace::Entry *nsEn;

	const char *nameSpace = ToCString(String::Utf8Value(args[0]));
	if (_stricmp(nameSpace, "") == 0)
		nameSpace = "ts";

	const char *fnName = ToCString(String::Utf8Value(args[1]));
	if (_stricmp(fnName, "") == 0)
		return;

	char buffer[256];
	strncpy(buffer, nameSpace, sizeof(buffer));
	strncat(buffer, "__", sizeof(buffer));
	strncat(buffer, fnName, sizeof(buffer));
	Printf("%s", (const char *)buffer);

	if (_stricmp(nameSpace, "ts") == 0)
	{
		ns = LookupNamespace(NULL);

		nsEn = Namespace__lookup(ns, StringTableEntry(fnName));
		if (nsEn == NULL)
			return;

		Local<FunctionTemplate> jsHandleFunc = FunctionTemplate::New(_Isolate, js_handleFunc);

		//this worked just fucking fine except the callee name is null so fuck
		//ContextL()->Global()->Set(String::NewFromUtf8(_Isolate, (const char *)buffer, NewStringType::kNormal).ToLocalChecked(),
		//  jsHandleFunc->GetFunction());

		AddFunction((const char *)buffer, js_handleFunc);
		Local<ObjectTemplate> global = Local<ObjectTemplate>::New(_Isolate, glo);
		Local<Context> context = Context::New(_Isolate, NULL, global);
		tempObjContext.Reset(_Isolate, context);
		Context::Scope scope(context);
		//glo.Reset(_Isolate, global);
	}
	else
	{
		return;
	}
}

void js_setVariable(const FunctionCallbackInfo<Value> &args)
{
	SetGlobalVariable(ToCString(String::Utf8Value(args[0])), ToCString(String::Utf8Value(args[1])));
}

void js_getVariable(const FunctionCallbackInfo<Value> &args)
{
	args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, GetGlobalVariable(ToCString(String::Utf8Value(args[0])))));
}

//TorqueScript Functions
static const char *ts__js_eval(SimObject *obj, int argc, const char *argv[])
{
	if (argv[1] == NULL)
		return "";

	Locker locker(_Isolate);
	Isolate::Scope isolate_scope(_Isolate);
	HandleScope handle_scope(_Isolate);
	_Context.Reset(_Isolate, StrongPersistentTL(tempObjContext));
	Context::Scope context_scope(ContextL());
	TryCatch try_catch;

	v8::ScriptOrigin origin(String::NewFromUtf8(_Isolate, "(shell)"));
	Local<String> source = String::NewFromUtf8(_Isolate, argv[1],
		NewStringType::kNormal).ToLocalChecked();

	Local<Script> script;
	if (!Script::Compile(ContextL(), source, &origin).ToLocal(&script))
	{
		ReportException(_Isolate, &try_catch);
		return "";
	}
	else
	{
		Local<Value> result;
		if (!script->Run(ContextL()).ToLocal(&result))
		{
			ReportException(_Isolate, &try_catch);
			return "";
		}
		else
		{
			if (try_catch.HasCaught())
			{
				ReportException(_Isolate, &try_catch);
				return "";
			}

			if (!result.IsEmpty())
			{
				const char *res = ToCString(String::Utf8Value(result));

				if (!istrue(argv[2]))
					Printf("%s", res);

				return res;
			}
			else
			{
				Unlocker unlocker(_Isolate);
				_Isolate->Exit();
				if (!tempObjContext.IsEmpty())
				{
					Printf("deb");
					_Context.Reset(_Isolate, tempObjContext);
				}

				return "true";
			}
		}
	}
	Unlocker unlocker(_Isolate);
	_Isolate->Exit();
	return "";
}

static const char *ts__js_exec(SimObject *obj, int argc, const char *argv[])
{
	Locker locker(_Isolate);
	HandleScope handle_scope(_Isolate);
	Context::Scope context_scope(ContextL());
	TryCatch try_catch;

	ScriptOrigin origin(String::NewFromUtf8(_Isolate, argv[1]));
	Local<String> source = ReadFile(_Isolate, argv[1]).ToLocalChecked();

	Local<Script> script;
	if (!Script::Compile(ContextL(), source, &origin).ToLocal(&script))
	{
		ReportException(_Isolate, &try_catch);
		return "false";
	}
	else
	{
		Local<Value> result;
		if (!script->Run(ContextL()).ToLocal(&result))
		{
			ReportException(_Isolate, &try_catch);
			return "false";
		}
		else
		{
			if (try_catch.HasCaught())
			{
				ReportException(_Isolate, &try_catch);
				return "false";
			}

			if (!result.IsEmpty())
			{
				const char *res = ToCString(String::Utf8Value(result));
				if (!istrue(argv[2]))
					Printf("%s", res);
				return res;
			}
		}
	}

	Unlocker unlocker(_Isolate);
	return "";
}

bool deinit();

static bool ts__js_deinit(SimObject *obj, int argc, const char *argv[])
{
	return deinit();
}

bool init()
{
	if (!torque_init())
		return false;

	//Initialize V8
	Printf("Initializing V8");
	V8::InitializeExternalStartupData("./Blockland");
	V8::InitializeICU();

	//Create platform
	_Platform = platform::CreateDefaultPlatform();
	V8::InitializePlatform(_Platform);
	V8::Initialize();

	//Create isolate
	Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
	_Isolate = Isolate::New(create_params);

	_Isolate->Enter();

	//Create a handle scope
	HandleScope scope(_Isolate);

	//Set up the functions
	globall = ObjectTemplate::New(_Isolate);

	globall->Set(String::NewFromUtf8(_Isolate, "print", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_print));

	globall->Set(String::NewFromUtf8(_Isolate, "ts_eval", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_eval));

	globall->Set(String::NewFromUtf8(_Isolate, "ts_call", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_call));

	globall->Set(String::NewFromUtf8(_Isolate, "ts_setVariable", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_setVariable));

	globall->Set(String::NewFromUtf8(_Isolate, "ts_getVariable", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_getVariable));

	globall->Set(String::NewFromUtf8(_Isolate, "ts_func", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_func));

	//Create context w/ global
	Local<Context> context = Context::New(_Isolate, NULL, globall);
	_Context.Reset(_Isolate, context);
	glo.Reset(_Isolate, globall);
	tempObjContext.Reset(_Isolate, context);
	//TorqueScript functions
	ConsoleFunction(NULL, "js_eval", ts__js_eval,
		"js_eval(string [, silent]) - evaluates a javascript string and returns the result", 2, 3);

	ConsoleFunction(NULL, "js_exec", ts__js_exec,
		"js_exec(file) - evaluates a javascript file", 2, 2);

	ConsoleFunction(NULL, "js_deInit", ts__js_deinit,
		"js_deInit - deinitialize v8. this is not undoable.", 0, 0);

	Printf("BlocklandJS | Attached");

	Eval("package blocklandjs{function quit(){js_deInit(); return parent::quit();}};activatepackage(blocklandjs);");
	return true;
}

bool deinit()
{
	_Isolate->Exit();
	_Isolate->Dispose();

	V8::Dispose();
	V8::ShutdownPlatform();
	//delete _Platform; <- this seemed to break it
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
