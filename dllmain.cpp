#pragma once
#pragma comment(lib, "v8_nosnapshot.lib")
#pragma comment(lib, "v8_libsampler.lib")
#pragma comment(lib, "v8_libbase.lib")
#pragma comment(lib, "v8_base_0.lib")
#pragma comment(lib, "v8_base_1.lib")
#pragma comment(lib, "v8_libplatform.lib")
#pragma comment(lib, "v8_init.lib")
#pragma comment(lib, "v8_initializers.lib")


#include <cstdint>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <chrono>
#include "Torque.h"
#include "include/v8-inspector.h"
#include "include/v8.h"
#include "libplatform/libplatform.h"
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
		Printf("\x03%s", *exception);
	else
	{
		String::Utf8Value filename(message->GetScriptOrigin().ResourceName());
		Local<Context> context(isolate->GetCurrentContext());
		int linenum = message->GetLineNumber(context).FromJust();

		Printf("\x03%s:%i: %s", *filename, linenum, *exception);

		String::Utf8Value sourceline(message->GetSourceLine(context).ToLocalChecked());
		Printf("\x03%s", *sourceline);

		std::stringstream ss;
		int start = message->GetStartColumn(context).FromJust();
		for (int i = 0; i < start; i++)
			ss << " ";

		int end = message->GetEndColumn(context).FromJust();
		for (int i = start; i < end; i++)
			ss << "^";

		Printf("\x03%s", ss.str().c_str());

		Local<Value> stack_trace_string;
		if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
			stack_trace_string->IsString() &&
			Local<String>::Cast(stack_trace_string)->Length() > 0)
		{
			String::Utf8Value stack_trace(stack_trace_string);
			Printf("\x03%s", *stack_trace);
		}
	}

	return true;
}

void js_getGlobalVar(const FunctionCallbackInfo<Value> &args)
{
	if (args.Length() != 1 || !args[0]->IsString()) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "invalid arguments passed to ts_getVariable"));
		return;
	}

	const char* varName = ToCString(String::Utf8Value(args[0]));
	Local<String> ret = String::NewFromUtf8(_Isolate, GetGlobalVariable(varName));
	args.GetReturnValue().Set(ret);
	return;
}

void js_setGlobalVar(const FunctionCallbackInfo<Value> &args)
{
	if (args.Length() != 2 || !args[0]->IsString() || !args[1]->IsString()) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "invalid arguments passed to ts_setVariable"));
		return;
	}

	const char* varName = ToCString(String::Utf8Value(args[0]));
	const char* value = ToCString(String::Utf8Value(args[1]));

	SetGlobalVariable(varName, value);
	//Local<String> ret = String::NewFromUtf8(_Isolate, varName);
	args.GetReturnValue().Set(true);
	return;
}

void js_print(const FunctionCallbackInfo<Value> &args)
{
	std::stringstream str; //Good for avoiding the trap that is fixed buffers..

	for (int i = 0; i < args.Length(); i++)
	{
		if (i > 0)
			str << " ";
		String::Utf8Value s(args[i]);
		str << *s;
	}

	Printf("%s", str.str().c_str());
}

v8::Local<v8::Context> ContextL() { return StrongPersistentTL(_Context); }

static const char* ts_js_eval(SimObject* this_, int argc, const char* argv[]) {
	Isolate::Scope iso_scope(_Isolate);
	HandleScope handle_scope(_Isolate);
	v8::Context::Scope cScope(ContextL());
	v8::ScriptOrigin origin(String::NewFromUtf8(_Isolate, "<shell>"));
	v8::TryCatch exceptionHandler(_Isolate);
	Local<String> scriptCode = String::NewFromUtf8(_Isolate, argv[1], NewStringType::kNormal).ToLocalChecked();
	Local<Script> script;
	Local<Value> result;

	if (!Script::Compile(ContextL(), scriptCode, &origin).ToLocal(&script))
	{
		ReportException(_Isolate, &exceptionHandler);
		return "false";
	}
	else {
		if (!script->Run(ContextL()).ToLocal(&result)) {
			ReportException(_Isolate, &exceptionHandler);
			return "false";
		}
		else {
			if (result->IsString()) {
				String::Utf8Value a(result);
				return ToCString(a);
			}
		}
	}
	return "true";
}

bool init()
{
	if (!torque_init())
	{
		return false;
	}

	Printf("BlocklandJS || Init");
	Printf("BlocklandJS: version v8.0.0.0");
	_Platform = platform::CreateDefaultPlatform();
	V8::InitializePlatform(_Platform);
	V8::Initialize();

	Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
	
	_Isolate = Isolate::New(create_params);
	_Isolate->Enter();

	HandleScope scope(_Isolate);

	Local<ObjectTemplate> global = ObjectTemplate::New(_Isolate);
	global->Set(String::NewFromUtf8(_Isolate, "print", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_print));
	global->Set(String::NewFromUtf8(_Isolate, "ts_getVariable", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_getGlobalVar));
	global->Set(String::NewFromUtf8(_Isolate, "ts_setVariable", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_setGlobalVar));

    Local<v8::Context> context = Context::New(_Isolate, NULL, global);
	_Context.Reset(_Isolate, context);
	_Isolate->Exit();

	ConsoleFunction(NULL, "js_eval", ts_js_eval, "(string script) - Evaluate a script in the JavaScript engine,", 2, 2);

	Printf("BlocklandJS || Attached");
	return true;
}

bool deinit() {
	_Isolate->Dispose();
	V8::Dispose();
	V8::ShutdownPlatform();
	delete _Platform;
	Printf("BlocklandJS || Detached");
	return true;
}

int WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
		return init();
	else if (reason == DLL_PROCESS_DETACH)
		return deinit();

	return true;
}
