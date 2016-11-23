#pragma once
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
Handle<ObjectTemplate> global;
Handle<FunctionTemplate> jsHandleFunc;
Handle<FunctionTemplate> jsHandleSimFunc;
//Random junk we have up here for support functions.
void jsHelper(Local<ObjectTemplate> lol);
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
bool ExecuteString(v8::Isolate* isolate, v8::Local<v8::String> source,
	v8::Local<v8::Value> name, bool print_result,
	bool report_exceptions) {
	v8::HandleScope handle_scope(isolate);
	v8::TryCatch try_catch(isolate);
	v8::ScriptOrigin origin(name);
	v8::Local<v8::Context> context(isolate->GetCurrentContext());
	v8::Local<v8::Script> script;
	if (!v8::Script::Compile(context, source, &origin).ToLocal(&script)) {
		// Print errors that happened during compilation.
		if (report_exceptions)
			ReportException(isolate, &try_catch);
		return false;
	}
	else {
		v8::Local<v8::Value> result;
		if (!script->Run(context).ToLocal(&result)) {
			assert(try_catch.HasCaught());
			// Print errors that happened during execution.
			if (report_exceptions)
				ReportException(isolate, &try_catch);
			return false;
		}
		else {
			assert(!try_catch.HasCaught());
			if (print_result && !result->IsUndefined()) {
				// If all went well and the result wasn't undefined then print
				// the returned value.
				v8::String::Utf8Value str(result);
				const char* cstr = ToCString(str);
				printf("%s\n", cstr);
			}
			return true;
		}
	}
}
v8::MaybeLocal<v8::String> ReadFile(v8::Isolate* isolate, const char* name) {
	FILE* file = fopen(name, "rb");
	if (file == NULL) return v8::MaybeLocal<v8::String>();

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (size_t i = 0; i < size;) {
		i += fread(&chars[i], 1, size - i, file);
		if (ferror(file)) {
			fclose(file);
			return v8::MaybeLocal<v8::String>();
		}
	}
	fclose(file);
	v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(
		isolate, chars, v8::NewStringType::kNormal, static_cast<int>(size));
	delete[] chars;
	return result;
}
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
void Load(const v8::FunctionCallbackInfo<v8::Value>& args) {
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope(args.GetIsolate());
		v8::String::Utf8Value file(args[i]);
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
		if (!ExecuteString(args.GetIsolate(), source, args[i], false, false)) {
			args.GetIsolate()->ThrowException(
				v8::String::NewFromUtf8(args.GetIsolate(), "Error executing file",
					v8::NewStringType::kNormal).ToLocalChecked());
			return;
		}
	}
}
void js_isObject(const FunctionCallbackInfo<Value> &args)
{
	SimObject* Obj = Sim__findObject_id(args[0]->Uint32Value());
	if (Obj == NULL)
	{
		return args.GetReturnValue().Set(false);
	}
	else
	{
		return args.GetReturnValue().Set(true);
	}
}
/*
So, what are we doing?
We need to 
A. Pass a struct around in javascript
B. Create a function that calls CodeBlocks__execute
*/
//THANK YOU PORT! I would not be able to do this w/o you!
//This was written by port. I did some medium modifications.
void js_handleSimFunc(const FunctionCallbackInfo<Value> &args)
{
	//We have to do the resolving again.
	Namespace *ns;
	Namespace::Entry *nsE;
	char* callee = *String::Utf8Value(args.Callee()->GetName()->ToString());
	callee = strtok(callee, "__");
	const char* nameSpace = callee;
	callee = strtok(NULL, "__");
	const char* function = callee;
	ns = LookupNamespace(nameSpace);
	nsE = Namespace__lookup(ns, StringTableEntry(function));
	if (nsE == NULL)
	{
		return args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, "didn't work."));
	}
	int argc = 0;
	const char *argv[21];
	argv[argc++] = nsE->mFunctionName;
	SimObject *obj;
	Printf("Check if sim is null or whatev");
	if (!args[0]->IsNull() || args[0]->IsInt32() || args[0]->IsUint32())
	{
		Printf("FindObj");
		SimObject *ref = Sim__findObject_id(args[0]->Uint32Value());
		obj = ref;
	}
	else
		obj = NULL;

	Printf("aaa");
	if (obj != NULL)
	{
		char idbuf[sizeof(int) * 3 + 2];
		snprintf(idbuf, sizeof idbuf, "%d", obj->id);
		argv[argc++] = StringTableEntry(idbuf);
	}
	for (int i = 0; i < args.Length(); i++)
	{
		if (_stricmp(*String::Utf8Value(args[i]), ""))
		{
			argv[argc++] = StringTableEntry(StringTableEntry(*String::Utf8Value(args[i])));
		}
	}
	Printf("probably worked by now lol");
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
		args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, "invalid amount of args"));
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
	return args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, "invalid func"));
}


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
		return args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, "didn't work."));
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
		args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, "invalid amount of args"));
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
	return args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, "invalid func"));
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
void js_fcbn(const FunctionCallbackInfo<Value> &args)
{
	SimObjectId lol = Sim__findObject_name(StringTableEntry(*String::Utf8Value(args[0])))->id;
	if (lol != NULL)
	{
		Printf("exists");
	}
}
void js_newObj(const FunctionCallbackInfo<Value> &args)
{
	//apparantly we have to alloc memory, it's not doing this properly and it's leading to unexpected results
	SimObject* NewSimO = (SimObject *)AbstractClassRep_create_className(*String::Utf8Value(args[0]));
	if (NewSimO == NULL)
	{
		Printf(*String::Utf8Value(args[0]));
		return args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, "didn't work lol"));
	}

	NewSimO->mFlags |= SimObject::ModStaticFields;
	NewSimO->mFlags |= SimObject::ModDynamicFields;
	if (!_stricmp(*String::Utf8Value(args[0]), "fxDTSBrick"))
	{
		//Patch memory temporarily.
		WriteProcessMemory(GetCurrentProcess(), (void*)0x00568CD0, "\x89\xC8\x90", 3, NULL);
		//filler for brick shit here
		fxDTSBrick__setDataBlock(NewSimO, StringTableEntry(*String::Utf8Value(args[1])));
		WriteProcessMemory(GetCurrentProcess(), (void*)0x00568CD0, "\x8B\x46\x08", 3, NULL);
		SimObject__setDataField(NewSimO, StringTableEntry("isPlanted"), StringTableEntry(""), "true");
		SimObject__setDataField(NewSimO, StringTableEntry("position"), StringTableEntry(""), *String::Utf8Value(args[2]));
		SimObject__registerObject(NewSimO);
		fxDTSBrick__plant(NewSimO);
		std::stringstream ss;
		if (_stricmp(*String::Utf8Value(args[3]), ""))
		{
			ss << "brickGroup_" << *String::Utf8Value(args[3]) << ".add(" << NewSimO->id << ");";
		}
		else
		{
			ss << "brickGroup_999999.add(" << NewSimO->id << ");";
		}
		std::string s = ss.str();
		Eval(ss.str().c_str());
		//For later- get the id of setTrusted. I haven't fucking figured out how to do this entirely and it freaks me the fuck out...
		//Untapped potential!!!!
		ss << NewSimO->id << ".setTrusted(true);";
		Eval(ss.str().c_str());
		args.GetReturnValue().Set(Uint32::NewFromUnsigned(_Isolate, NewSimO->id));
		return;
	}
	else
	{
		SimObject__registerObject(NewSimO);
		args.GetReturnValue().Set(Uint32::NewFromUnsigned(_Isolate, NewSimO->id));
	}
}
/*
I'll fix this later.
void js_setDatablock(const FunctionCallbackInfo<Value> &args)
{
	Local<Integer> lol = Uint32::NewFromUnsigned(_Isolate, args[0]->Int32Value());
	SimObject* Obj = Sim__findObject_id(lol->Uint32Value());
	
	if (Obj->mTypeMask = atoi(GetGlobalVariable("$TypeMasks::FxBrickObjectType")))
	{
		Printf("lol brick detected");
		fxDTSBrick__setDataBlock(Obj, StringTableEntry(*String::Utf8Value(args[1])));
		return;
	}
	Printf("lol no brick detected");
	SimObject__setDataBlock(Obj, *String::Utf8Value(args[1]));
}
*/
void js_getDatablock(const FunctionCallbackInfo<Value> &args)
{
	Local<Integer> lol = Uint32::NewFromUnsigned(_Isolate, args[0]->Int32Value());
	SimObject* Obj = Sim__findObject_id(lol->Uint32Value());
}
void js_call(const FunctionCallbackInfo<Value> &args)
{
	int argc = args.Length();
	if (argc > 22) //i think 22 is max for torque, check that
		return;
	const char* argv[21];
	for (int i = 0; i < args.Length(); i++)
	{
		String::Utf8Value s(args[i]);
		argv[i] = ToCString(s);
	}
	RawCall(argc, *argv);
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
	SetGlobalVariable(*String::Utf8Value(args[0]), *String::Utf8Value(args[1]));
}

void js_setObjectField(const FunctionCallbackInfo<Value> &args)
{
	if (args[0].IsEmpty() | args[1].IsEmpty() | args[2].IsEmpty())
		return;

	Local<Integer> lol = Uint32::NewFromUnsigned(_Isolate, args[0]->Int32Value());
	SimObject* Obj = Sim__findObject_id(lol->Uint32Value());
	if (Obj->id == NULL)
	{
		return;
	}
	const char *dataField = *String::Utf8Value(args[1]);
	const char *value = *String::Utf8Value(args[2]);
	SimObject__setDataField(Obj, StringTableEntry(dataField), StringTableEntry(""), value);
}
void js_getObjectField(const FunctionCallbackInfo<Value> &args)
{
	if (args[0].IsEmpty() | args[1].IsEmpty() | args[2].IsEmpty())
		return;

	Local<Integer> lol = Uint32::NewFromUnsigned(_Isolate, args[0]->Int32Value());
	SimObject* Obj = Sim__findObject_id(lol->Uint32Value());
	if (Obj->id == NULL)
	{
		return;
	}
	const char* dataField = *String::Utf8Value(args[1]);
	args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, SimObject__getDataField(Obj, dataField, StringTableEntry(""))));
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
	args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, GetGlobalVariable(str.str().c_str())));
}
void js_quit(const FunctionCallbackInfo<Value> &args)
{
	v8::Unlocker unlocker(_Isolate);
	args.GetIsolate()->Exit();
	_Isolate->Dispose();
	V8::Dispose();
	V8::ShutdownPlatform();
	delete _Platform;
}
//Exposed torquescript functions
v8::Local<v8::Context> ContextL() { return StrongPersistentTL(_Context); }
static const char *ts__js_eval(SimObject *obj, int argc, const char *argv[])
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
					char *kappa = (char *)malloc(strlen(*str) + 1);
					strcpy(kappa, *str);
					return kappa;
				}
				else
				{
					String::Utf8Value str(result);
					Printf("%s", *str);
					return "y";
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
void ts__js_quit(SimObject *obj, int argc, const char *argv[])
{
	//Call the destruction procedure..
	v8::Unlocker unlocker(_Isolate);
	_Isolate->Dispose();
	V8::Dispose();
	V8::ShutdownPlatform();
	delete _Platform;
	//delete _Params.array_buffer_allocator;
	Printf("BL V8 | Detached");
}
void js_func(SimObject *obj, int argc, const char *argv[])
{
	Namespace *ns;
	Namespace::Entry *nsEn;
	if (!_stricmp(argv[1], "ts"))
	{
		ns = LookupNamespace(NULL);
	}
	else
	{
		ns = LookupNamespace(argv[1]);
	}
	const char* name = argv[2];
	Printf(name);
	nsEn = Namespace__lookup(ns, StringTableEntry(name));
	if (nsEn == NULL)
	{
		return;
	}
	Printf("making function temp");
	v8::Locker locker(_Isolate);
	_Isolate->Enter();
	HandleScope scope(_Isolate);
		const char* bla = argv[2];
		const char* nsw = argv[1];
		char buffer[256];
		strncpy(buffer, nsw, sizeof(buffer));
		strncat(buffer, "__", sizeof(buffer));
		strncat(buffer, bla, sizeof(buffer));
		Printf(buffer);
		Handle<ObjectTemplate> globall = ObjectTemplate::New(_Isolate);

		if (!_stricmp(argv[1], "ts"))
		{
			Printf("Non-SimObject function detected");
			if (jsHandleFunc.IsEmpty())
			{
				Printf("Found no instance of jsHandleFunc...creating");
				jsHandleFunc = FunctionTemplate::New(_Isolate, js_handleFunc);
			}
			Printf("Creating new entry in ObjectTemplate...");
				global->Set(v8::String::NewFromUtf8(_Isolate, buffer, NewStringType::kNormal).ToLocalChecked(),
					jsHandleFunc);
		}
		else
		{
			if (jsHandleSimFunc.IsEmpty())
			{
				Printf("Found no instance of jsHandleSimFunc...creating");
				jsHandleSimFunc = FunctionTemplate::New(_Isolate, js_handleSimFunc);
			}
			global->Set(v8::String::NewFromUtf8(_Isolate, buffer, NewStringType::kNormal).ToLocalChecked(),
				jsHandleSimFunc);
		}
		Printf("done everything else");
		Printf("creating context");
		Local<Context> context = Context::New(_Isolate, NULL, globall);
		Printf("resetting current context");
		globall = global;
		_Context.Reset(_Isolate, context);
	Printf("made context");
	Printf("reset");
	_Isolate->Exit();
	v8::Unlocker unlocker(_Isolate);
}
void ts__js_exec(SimObject *obj, int argc, const char *argv[])
{
	v8::Locker locker(_Isolate);
	Isolate::Scope isolate_scope(_Isolate);
	HandleScope handle_scope(_Isolate);
	v8::Context::Scope contextScope(ContextL());
	TryCatch try_catch;
	v8::ScriptOrigin origin(String::NewFromUtf8(_Isolate, "file"));
	Local<String> source =
		ReadFile(_Isolate, argv[1]).ToLocalChecked();

	Local<Script> script;
	if (!Script::Compile(ContextL(), source, &origin).ToLocal(&script))
	{
		ReportException(_Isolate, &try_catch);
		return;
	}
	else
	{
		v8::Local<v8::Value> result;
		if (!script->Run(ContextL()).ToLocal(&result))
		{
			assert(try_catch.HasCaught());
			ReportException(_Isolate, &try_catch);
			return;
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
					char *kappa = (char *)malloc(strlen(*str) + 1);
					strcpy(kappa, *str);
					return;
				}
				else
				{
					String::Utf8Value str(result);
					Printf("%s", *str);
					return;
				}
				//thanks buddy now i have to do this
			}
			else return;
		}
	}
	_Context.Reset(_Isolate, ContextL());
	v8::Unlocker unlocker(_Isolate);
	_Isolate->Exit();
	return;
}
/*
void ts__js_test(SimObject *obj, int argc, const char *argv[])
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
void jsHelper(Local<ObjectTemplate> lol)
{
	lol->Set(String::NewFromUtf8(_Isolate, "print", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_print));
	lol->Set(String::NewFromUtf8(_Isolate, "ts_eval", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_eval));
	lol->Set(String::NewFromUtf8(_Isolate, "ts_setvariable", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_addVariable));
	lol->Set(String::NewFromUtf8(_Isolate, "ts_getvariable", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_getVariable));
	lol->Set(String::NewFromUtf8(_Isolate, "ts_call", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_call));
	lol->Set(String::NewFromUtf8(_Isolate, "ts_newObj", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_newObj));
	lol->Set(String::NewFromUtf8(_Isolate, "quit", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_quit));
	lol->Set(String::NewFromUtf8(_Isolate, "ts_getDataField", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_getObjectField));
	lol->Set(String::NewFromUtf8(_Isolate, "ts_setDataField", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_setObjectField));
	lol->Set(v8::String::NewFromUtf8(_Isolate, "read", v8::NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, Read));
	lol->Set(v8::String::NewFromUtf8(_Isolate, "load", v8::NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, Load));
	lol->Set(v8::String::NewFromUtf8(_Isolate, "ts_isObject", v8::NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_isObject));
	lol->Set(v8::String::NewFromUtf8(_Isolate, "ts_fcbn", v8::NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, js_fcbn));
}
DWORD WINAPI Init(LPVOID args)
{
	if (!torque_init())
	{
		return 0;
	}

	//--Initialize V8
	Printf("Initializing V8");
	V8::InitializeICU();
	//man this is one hacky hack. thank the fucking lord that the cwd is ./Blockland or else it would fuck up
	//for any code reviewers or modifiers, please change this! it's ugly
	//recompile the v8 engine and include these .bin files!!!
	V8::InitializeExternalStartupData("./Blockland");
	_Platform = platform::CreateDefaultPlatform();
	V8::InitializePlatform(_Platform);
	V8::Initialize();

	Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
	
	
	_Isolate = Isolate::New(create_params);
	//wtf? it seems to be exiting this thread and joining into another one.
	_Isolate->Enter();

	//enter the isolate, create a shit		for it
	HandleScope scope(_Isolate);

	global = ObjectTemplate::New(_Isolate);
	jsHelper(global);
	//global->Set(String::NewFromUtf8(_Isolate, "ts_setDatablock", NewStringType::kNormal).ToLocalChecked(),
		//FunctionTemplate::New(_Isolate, js_setDatablock));

	// Create a new context.
	Local<v8::Context> context = Context::New(_Isolate, NULL, global);
	_Context.Reset(_Isolate, context);
	Context::Scope contextScope(context);
	//-ttt
	_Isolate->Exit();

	//--Torque Stuff
	ConsoleFunction(NULL, "js_eval", ts__js_eval,
		"js_eval(string) - evaluate a javascript string", 2, 3);
	ConsoleFunction(NULL, "js_exec", ts__js_exec,
		"js_exec(dir to string) - evaluate a javascript file", 2, 2);
	ConsoleFunction(NULL, "js_quit", ts__js_quit,
		"js_quit, safely quit or some shit idk", 2, 2);
	ConsoleFunction(NULL, "js_func", js_func,
		"define a function to use in javascript", 2, 3);
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
