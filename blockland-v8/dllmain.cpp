#pragma once
#pragma comment(lib, "v8_base.lib")
#pragma comment(lib, "v8_nosnapshot.lib")
#pragma comment(lib, "icui18n.dll.lib")
#pragma comment(lib, "icuuc.dll.lib")
#pragma comment(lib, "v8_libsampler.lib")
#pragma comment(lib, "v8_libbase.dll.lib")
#pragma comment(lib, "v8_libplatform.lib")
#pragma comment(lib, "v8.dll.lib")

#include <cstdint>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <chrono>
#include "Torque.h"
#include "include/v8.h"
#include "libplatform/libplatform.h"
#include "types.h"
#include <vector>

#pragma warning( push )
#pragma warning( disable : 4946 )

using namespace v8;

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
public:
	virtual void* Allocate(size_t length) {
		void* data = AllocateUninitialized(length);
		return data == NULL ? data : memset(data, 0, length);
	}
	virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
	virtual void Free(void* data, size_t) { free(data); }
};
template <class TypeName>
inline v8::Local<TypeName> StrongPersistentTL(
	const v8::Persistent<TypeName>& persistent)
{
	return *reinterpret_cast<v8::Local<TypeName>*>(
		const_cast<v8::Persistent<TypeName>*>(&persistent));
}
typedef void (WINAPI *vCall)(S32 argc, const char* argv[]);
vCall tCall = (vCall)(0x004A7110);

template <class TypeName>
inline v8::Local<TypeName> StrongPersistentTL(
	const v8::Persistent<TypeName>& persistent);
Platform *_Platform;
Isolate *_Isolate;
//Persistent<Context, CopyablePersistentTraits<Context>> _Context;
Persistent<Context> _Context;

//Random junk we have up here for support functions.
const char* ToCString(const v8::String::Utf8Value& value)
{
	return *value ? *value : "<string conversion failed>";
}
bool ReportException(Isolate *isolate, TryCatch *try_catch)
{
	HandleScope handle_scope(isolate);
	String::Utf8Value exception(try_catch->Exception());
	Local<Message> message = try_catch->Message();

	if (message.IsEmpty())
		Printf("%s", *exception);
	else
	{
		String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
		Local<Context> context(isolate->GetCurrentContext());
		int linenum = message->GetLineNumber(context).FromJust();

		Printf("%s:%i: %s", *filename, linenum, *exception);

		String::Utf8Value sourceline(message->GetSourceLine(context).ToLocalChecked());
		Printf("%s", *sourceline);

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
			Printf("%s", *stack_trace);
		}
	}

	return true;
}
/*
void Read(const v8::FunctionCallbackInfo<v8::Value>& args) {
	if (args.Length() != 1) {
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
				v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}
	v8::String::Utf8Value file(args[0]);
	if (*file == NULL) {
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
				v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}
	v8::Local<v8::String> source;
	if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
		args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
				v8::NewStringType::kNormal).ToLocalChecked());
		return;
	}
	args.GetReturnValue().Set(source);
}
*/
void js_call(const FunctionCallbackInfo<Value> &args)
{
	std::stringstream str;
	for (int i = 0; i < args.Length(); i++)
	{
		if (i > 0)
			str << " ";
		String::Utf8Value s(args[i]);
		str << *s;
	}
	Printf(str.str().c_str());
	const char* argslol[] = { "echo", 0, "Message!" };
	tCall(3, argslol);
	void *garbage;
	__asm {
		pop garbage
		pop garbage
		pop garbage
	}

}

void js_print(const FunctionCallbackInfo<Value> &args)
{
	std::stringstream str;

	for (int i = 0; i < args.Length(); i++)
	{
		if (i > 0)
			str << " ";
		String::Utf8Value s(args[i]);
		str << *s;
	}

	Printf("%s", str.str().c_str());
}
void js_eval(const FunctionCallbackInfo<Value> &args)
{
	std::stringstream str;

	for (int i = 0; i < args.Length(); i++)
	{
		if (i > 0)
			str << " ";
		String::Utf8Value s(args[i]);
		str << *s;
	}
	args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, Eval(str.str().c_str())));
}
void js_addVariable(const FunctionCallbackInfo<Value> &args)
{
	for (int i = 0; i < args.Length(); i++)
	{
		String::Utf8Value s(args[i]);
	}
	Printf("%s", *String::Utf8Value(args[0]));
	Printf("%s", *String::Utf8Value(args[1]));
	setVariable(*String::Utf8Value(args[0]), *String::Utf8Value(args[1]));
}
void js_getVariable(const FunctionCallbackInfo<Value> &args)
{
	std::stringstream str;

	for (int i = 0; i < args.Length(); i++)
	{
		if (i > 0)
			str << " ";
		String::Utf8Value s(args[i]);
		str << *s;
	}
	args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, getVariable(str.str().c_str())));
}
//Exposed torquescript functions
v8::Local<v8::Context> ContextL() { return StrongPersistentTL(_Context); }
const char* ts__js_eval(DWORD *obj, int argc, const char *argv[])
{
	if (argv[1] == NULL)
		return false;

	//Why	
	v8::Locker locker(_Isolate);
	Isolate::Scope isolate_scope(_Isolate);
	HandleScope handle_scope(_Isolate);
	v8::Context::Scope contextScope(ContextL());
	TryCatch try_catch;
	v8::ScriptOrigin origin(String::NewFromUtf8(_Isolate, "(shell)"));
	Local<String> source =
		String::NewFromUtf8(_Isolate, argv[1],
			NewStringType::kNormal).ToLocalChecked();

	Local<Script> script;
	if (!Script::Compile(ContextL(), source, &origin).ToLocal(&script))
	{
		ReportException(_Isolate, &try_catch);
		return false;
	}
	else
	{
		v8::Local<v8::Value> result;
		if (!script->Run(ContextL()).ToLocal(&result))
		{
			assert(try_catch.HasCaught());
			ReportException(_Isolate, &try_catch);
			return false;
		}
		else
		{
			//i
			assert(!try_catch.HasCaught());
			if (!result.IsEmpty())
			{
				if (argv[2])
				{
					String::Utf8Value str(result);
					return *str;
				}
				else
				{
					String::Utf8Value str(result);
					Printf("%s", *str);
					return "1";
				}
				//thanks buddy now i have to do this
			}
			else return "done successfully.";
		}
	}
	_Context.Reset(_Isolate, ContextL());
	v8::Unlocker unlocker(_Isolate);
	_Isolate->Exit();
	return false;
}

/* Unneeded.
void ts__js_test(DWORD *obj, int argc, const char *argv[])
{
	Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();

	Isolate* isolate = Isolate::New(create_params);
	{
		Isolate::Scope isolate_scope(isolate);

		HandleScope handle_scope(isolate);

		Local<Context> context = Context::New(isolate);
		Context::Scope context_scope(context);

		Local<String> source =
			String::NewFromUtf8(isolate, argv[1],
				NewStringType::kNormal).ToLocalChecked();

		Local<Script> script = Script::Compile(context, source).ToLocalChecked();
		Local<Value> result = script->Run(context).ToLocalChecked();

		String::Utf8Value utf8(result);
		Printf("%s\n", *utf8);
	}

	isolate->Dispose();
	return;
}
*/
DWORD WINAPI Init(LPVOID args)
{
	if (!InitTorque())
		return 0;

	//--Initialize V8
	Printf("Initializing V8");
	V8::InitializeICU();
	_Platform = platform::CreateDefaultPlatform();
	V8::InitializePlatform(_Platform);
	V8::Initialize();

	Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();

	_Isolate = Isolate::New(create_params);
	//wtf? it seems to be exiting this thread and joining into another one.
	_Isolate->Enter();

	//enter the isolate, create a shit for it
	HandleScope scope(_Isolate);

	Local<ObjectTemplate> global = ObjectTemplate::New(_Isolate);
	global->Set(String::NewFromUtf8(_Isolate, "print", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_print));
	global->Set(String::NewFromUtf8(_Isolate, "ts_eval", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_eval));
	global->Set(String::NewFromUtf8(_Isolate, "ts_addvariable", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_addVariable));
	global->Set(String::NewFromUtf8(_Isolate, "ts_getvariable", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_getVariable));
	global->Set(String::NewFromUtf8(_Isolate, "ts_call", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_call));


	// Create a new context.
	Local<v8::Context> context = Context::New(_Isolate, NULL, global);
	_Context.Reset(_Isolate, context);
	Context::Scope contextScope(context);
	//--
	_Isolate->Exit();


	//--Torque Stuff
	ConsoleFunction(NULL, "js_eval", ts__js_eval,
		"js_eval(string) - evaluate a javascript string", 2, 3);
	//--

	Printf("BL V8 | Attached");
	return 0;
}

int WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Init, NULL, 0, NULL);
	else if (reason == DLL_PROCESS_DETACH)
	{
		//this worked at one point but now it just makes bl hang for whatever reason
		_Isolate->Dispose();
		V8::Dispose();
		V8::ShutdownPlatform();
		delete _Platform;
		//delete _Params.array_buffer_allocator;
		Printf("BL V8 | Detached");
	}
	return true;
}
