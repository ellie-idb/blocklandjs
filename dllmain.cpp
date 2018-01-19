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

static char* ReadChars(const char* name, int* size_out) {

	FILE* file = fopen(name, "rb");
	if (file == nullptr) return nullptr;

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	char* chars = new char[size + 1];
	chars[size] = '\0';
	for (size_t i = 0; i < size;) {
		i += fread(&chars[i], 1, size - i, file);
		if (ferror(file)) {
			fclose(file);
			delete[] chars;
			return nullptr;
		}
	}
	fclose(file);
	*size_out = static_cast<int>(size);
	return chars;
}
class ExternalOwningOneByteStringResource
	: public String::ExternalOneByteStringResource {
public:
	ExternalOwningOneByteStringResource() : length_(0) {}
	ExternalOwningOneByteStringResource(std::unique_ptr<const char[]> data,
		size_t length)
		: data_(std::move(data)), length_(length) {}
	const char* data() const override { return data_.get(); }
	size_t length() const override { return length_; }

private:
	std::unique_ptr<const char[]> data_;
	size_t length_;
};

Local<String> ReadFile(Isolate* isolate, const char* name) {
	int size = 0;
	char* chars = ReadChars(name, &size);
	if (chars == nullptr) return Local<String>();
	Local<String> result;
		result = String::NewFromUtf8(isolate, chars, NewStringType::kNormal, size)
			.ToLocalChecked();
		delete[] chars;
	return result;
}


template <class TypeName>
inline v8::Local<TypeName> StrongPersistentTL(
	const v8::Persistent<TypeName>& persistent);
Platform *_Platform;
Isolate *_Isolate;
//Persistent<Context, CopyablePersistentTraits<Context>> _Context;
Persistent<Context> _Context;
Persistent<ObjectTemplate> objectHandlerTemp;
v8::Local<v8::Context> ContextL() { return StrongPersistentTL(_Context); }
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

void Load(const v8::FunctionCallbackInfo<v8::Value>& args) {
	for (int i = 0; i < args.Length(); i++) {
		HandleScope handle_scope(args.GetIsolate());
		String::Utf8Value file(args.GetIsolate(), args[i]);
		if (*file == nullptr) {
			_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Error loading file"));
			return;
		}
		Local<String> source = ReadFile(args.GetIsolate(), *file);
		if (source.IsEmpty()) {
			_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Error loading file"));
			return;
		}
		v8::ScriptOrigin origin(String::NewFromUtf8(_Isolate, "<shell>"));
		v8::TryCatch exceptionHandler(_Isolate);
		Local<Script> script;
		Local<Value> result;
		if (!Script::Compile(ContextL(), source, &origin).ToLocal(&script))
		{
			ReportException(_Isolate, &exceptionHandler);
			return;
		}
		else {
			if (!script->Run(ContextL()).ToLocal(&result)) {
				ReportException(_Isolate, &exceptionHandler);
				return;
			}
		}
		return;
	}
}

bool trySimObjectRef(SimObject** check) {
	if (check != nullptr && check != NULL) {
		if (*check != nullptr && *check != NULL) {
			return true;
		}
	}
	return false;
}

void js_interlacedCall(Namespace::Entry* ourCall, SimObject* obj, const FunctionCallbackInfo<Value> &args) {
	if (ourCall == NULL) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "ourCall == NULL"));
		return;
	}
	if (args.Length() > 19) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Too many arguments for TS to handle."));
		return;
	}
	const char* argv[21];
	int argc = 0;
	argv[argc++] = ourCall->mFunctionName;
	SimObject* actualObj;
	std::string quickref;
	if (obj == NULL) {
		actualObj = NULL;
	}
	else
	{
		quickref = std::to_string(obj->id);
		argv[argc++] = quickref.c_str();
		actualObj = obj;
	}
	for (int i = 0; i < args.Length(); i++) {
		if (args[i]->IsObject()) {
			Local<Object> bleh = args[i]->ToObject();
			if (bleh->InternalFieldCount() != 2) {
				_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Got bad object reference"));
				return;
			}
			else {
				if (bleh->GetInternalField(1)->BooleanValue()) {
					Handle<External> ugh = Handle<External>::Cast(bleh->GetInternalField(0));
					SimObject** SimO = static_cast<SimObject**>(ugh->Value());
					if (trySimObjectRef(SimO)) {
						SimObject* this_ = *SimO;
						quickref = std::to_string(this_->id);
						argv[argc++] = quickref.c_str();
					}
				}
				else {
					_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Object not registered!"));
					return;
				}
			}
		}
		else {
			argv[argc++] = ToCString(String::Utf8Value(args[i]->ToString()));
		}
	}
	if (ourCall->mType == Namespace::Entry::ScriptFunctionType) {
		if (ourCall->mFunctionOffset) {
			const char* retVal = CodeBlock__exec(
				ourCall->mCode, ourCall->mFunctionOffset,
				ourCall->mNamespace, ourCall->mFunctionName,
				argc, argv, false, ourCall->mNamespace->mPackage,
				0);
			args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, retVal));
			return;
		}
		else {
			return;
		}
	}
	U32 mMinArgs = ourCall->mMinArgs, mMaxArgs = ourCall->mMaxArgs;
	if ((mMinArgs && argc < mMinArgs) || (mMaxArgs && argc > mMaxArgs)) {
		Printf("Expected args between %d and %d, got %d", mMinArgs, mMaxArgs, argc);
		//_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Got wrong number of arguments"));
		return;
	}
	switch (ourCall->mType) {
	case Namespace::Entry::StringCallbackType:
		args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, ourCall->cb.mStringCallbackFunc(actualObj, argc, argv)));
		return;
	case Namespace::Entry::IntCallbackType:
		args.GetReturnValue().Set(Integer::New(_Isolate, ourCall->cb.mIntCallbackFunc(actualObj, argc, argv)));
		return;
	case Namespace::Entry::FloatCallbackType: {
		//Wtf?
		args.GetReturnValue().Set(Number::New(_Isolate, ourCall->cb.mFloatCallbackFunc(actualObj, argc, argv)));
		return;
	}
	case Namespace::Entry::BoolCallbackType:
		args.GetReturnValue().Set(Boolean::New(_Isolate, ourCall->cb.mBoolCallbackFunc(actualObj, argc, argv)));
		return;
	case Namespace::Entry::VoidCallbackType:
		ourCall->cb.mVoidCallbackFunc(actualObj, argc, argv);
		return;
	}

	_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Something went horribly wrong."));
	return;
}

void js_handleCall(const FunctionCallbackInfo<Value> &args) {
	Handle<External> ourEntry = Handle<External>::Cast(args.Data());
	Namespace::Entry* bleh = static_cast<Namespace::Entry*>(ourEntry->Value());
	Handle<Object> ptr = args.This();
	Handle<External> ugh = Handle<External>::Cast(ptr->GetInternalField(0));
	SimObject** SimO = static_cast<SimObject**>(ugh->Value());
	if (trySimObjectRef(SimO)) {
		SimObject* this_ = *SimO;
		js_interlacedCall(bleh, this_, args);
	}
	else
	{
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Was unable to get unsafe pointer.."));
	}
	//Printf("%s", bleh->mNamespace->mName);
	//args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, "todo"));
}

void js_plainCall(const FunctionCallbackInfo<Value> &args) {
	Handle<External> ourEntry = Handle<External>::Cast(args.Data());
	Namespace::Entry* bleh = static_cast<Namespace::Entry*>(ourEntry->Value());
	js_interlacedCall(bleh, NULL, args);
}

void js_getter(Local<String> prop, const PropertyCallbackInfo<Value> &args) {
	//args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, "todo"));
	Handle<Object> ptr = args.This();
	Handle<External> ugh = Handle<External>::Cast(ptr->GetInternalField(0));
	SimObject** SimO = static_cast<SimObject**>(ugh->Value());
	if (trySimObjectRef(SimO)) {
		SimObject* this_ = *SimO;
		if (ptr->GetInternalField(1)->ToBoolean()->BooleanValue()) {
			Namespace::Entry* entry = fastLookup(this_->mNameSpace->mName, ToCString(String::Utf8Value(prop)));
			if (entry != nullptr) {
				//It's a function call.
				Handle<External> theLookup = External::New(_Isolate, (void*)entry);
				Local<Function> blah = FunctionTemplate::New(_Isolate, js_handleCall, theLookup)->GetFunction();
				args.GetReturnValue().Set(blah);
				return;
			}
		}
		const char* field = ToCString(String::Utf8Value(prop));
		if (_stricmp(field, "mTypeMask") == 0) {
			args.GetReturnValue().Set(Integer::New(_Isolate, this_->mTypeMask));
		}
		else {
			args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, SimObject__getDataField(this_, field, StringTableEntry(""))));
		}
	}
	else
	{
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Was unable to get unsafe pointer.."));
	}
	//bool registered = ptr->GetInternalField(1)->ToBoolean()->BooleanValue();
	//if (!registered) {
	//	_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Object not registered.."));
	//}
	return;
}

void js_setter(Local<String> prop, Local<Value> value, const PropertyCallbackInfo<Value> &args) {
	Handle<Object> ptr = args.This();
	Handle<External> ugh = Handle<External>::Cast(ptr->GetInternalField(0));
	SimObject** SimO = static_cast<SimObject**>(ugh->Value());
	if (trySimObjectRef(SimO)) {
		SimObject* this_ = *SimO;
		SimObject__setDataField(this_, ToCString(String::Utf8Value(prop)), StringTableEntry(""), ToCString(String::Utf8Value(value->ToString())));
	}
	else
	{
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Was unable to get unsafe pointer.."));
	}
	return;
}

void js_registerObject(const FunctionCallbackInfo<Value>&args) {
	if (!args[0]->IsObject()) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Object not passed to ts.registerObject"));
		return;
	}

	Handle<Object> ptr = args[0]->ToObject();
	if (ptr->InternalFieldCount() != 2) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Nice try"));
		return;
	}
	Handle<External> ugh = Handle<External>::Cast(ptr->GetInternalField(0));
	SimObject** SimO = static_cast<SimObject**>(ugh->Value());
	if (SimO != NULL && SimO != nullptr) {
		SimObject* this_ = *SimO;
		if (this_ != NULL && this_ != nullptr) {
			SimObject__registerObject(*SimO);
		}
		else {
			args.GetReturnValue().Set(false);
			return;
		}
	}
	else {
		args.GetReturnValue().Set(false);
		return;
	}
	ptr->SetInternalField(1, v8::Boolean::New(_Isolate, true));
	args.GetReturnValue().Set(true);
}

void WeakPtrCallback(const v8::WeakCallbackInfo<SimObject**> &data){
	Printf("Bitch this shit being Garbage Collected..");

}

void js_constructObject(const FunctionCallbackInfo<Value> &args) {
	if (!args.IsConstructCall())
	{
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "js_constructObject not called as constructor.."));
		return;
	}

	const char* className = ToCString(String::Utf8Value(_Isolate, args.Data()->ToString()));
	Printf("%s", className);

	SimObject* simObj = (SimObject*)AbstractClassRep_create_className(className);
	if (simObj == NULL) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "unable to construct new object"));
		return;
	}
	SimObject** safePtr = (SimObject**)malloc(sizeof(SimObject*));
	*safePtr = simObj;
	simObj->mFlags |= SimObject::ModStaticFields;
	simObj->mFlags |= SimObject::ModDynamicFields;
	SimObject__registerReference(simObj, safePtr);
	Handle<External> ref = External::New(_Isolate, (void*)safePtr);
	Local<Object> fuck = StrongPersistentTL(objectHandlerTemp)->NewInstance();
	fuck->SetInternalField(0, ref);
	fuck->SetInternalField(1, v8::Boolean::New(_Isolate, false));
	UniquePersistent<Object> out(_Isolate, fuck);
	out.SetWeak<SimObject**>(&safePtr, WeakPtrCallback, v8::WeakCallbackType::kInternalFields);
	args.GetReturnValue().Set(out);
	return;
}

SimObject* SimSet__getObject(DWORD set, int index)
{
	DWORD ptr1 = *(DWORD*)(set + 0x3C);
	SimObject* ptr2 = (SimObject*)*(DWORD*)(ptr1 + 0x4 * index);
	return ptr2;
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

void js_linkClass(const FunctionCallbackInfo<Value> &args) {
	if (args.Length() != 1 || !args[0]->IsString()) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "invalid arguments attempting to link class.."));
		return;
	}

	Local<Function> blah = FunctionTemplate::New(_Isolate, js_constructObject, args[0])->GetFunction();
	args.GetReturnValue().Set(blah);
}

void js_obj(const FunctionCallbackInfo<Value> &args) {
	if (args.Length() != 1) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Wrong number of arguments"));
		return;
	}

	if (!args[0]->IsString()) {
		if (!args[0]->IsNumber()) {
			_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Wrong type of arguments"));
			return;
		}
	}
	
	SimObject* find = nullptr;
	if (args[0]->IsString()) {
		find = Sim__findObject_name(ToCString(String::Utf8Value(args[0]->ToString())));
	}
	else if (args[0]->IsNumber()) {
		find = Sim__findObject_id(args[0]->ToInteger()->Int32Value());
	}
	if (find == nullptr) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "could not find object"));
		return;
	}
	SimObject** safePtr = (SimObject**)malloc(sizeof(SimObject*));
	*safePtr = find;
	SimObject__registerReference(find, safePtr);
	Handle<External> ref = External::New(_Isolate, (void*)safePtr);
	Local<Object> fuck = StrongPersistentTL(objectHandlerTemp)->NewInstance();
	fuck->SetInternalField(0, ref);
	fuck->SetInternalField(1, v8::Boolean::New(_Isolate, true));
	UniquePersistent<Object> out(_Isolate, fuck);
	out.SetWeak<SimObject**>(&safePtr, WeakPtrCallback, v8::WeakCallbackType::kInternalFields);
	args.GetReturnValue().Set(out);
}

void js_func(const FunctionCallbackInfo<Value> &args) {
	if (args.Length() != 1) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Bad arguments passed to ts.func"));
		return;
	}
	Namespace::Entry* entry = fastLookup("", ToCString(String::Utf8Value(args[0])));
	if (entry == nullptr || entry == NULL) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Function not found"));
		return;
	}

	Handle<External> theLookup = External::New(_Isolate, (void*)entry);
	Local<Function> blah = FunctionTemplate::New(_Isolate, js_plainCall, theLookup)->GetFunction();
	args.GetReturnValue().Set(blah);
}

void js_print(const FunctionCallbackInfo<Value> &args)
{
	std::stringstream str; //Good for avoiding the trap that is fixed buffers..

	for (int i = 0; i < args.Length(); i++)
	{
		if (i > 0)
			str << " ";
		String::Utf8Value s(args[i]->ToString());
		if (*s != NULL && *s != nullptr) {
			str << *s;
		}
		else {
			str << "(string conversion failed)";
		}
	}

	Printf("%s", str.str().c_str());
}

void js_SimSet_getObject(const FunctionCallbackInfo<Value> &args) {
	if (args.Length() != 2) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Wrong number of arguments"));
		return;
	}

	if (!args[0]->IsObject()) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "First argument is the wrong type.."));
		return;
	}

	if (!args[1]->IsNumber()) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Second argument is the wrong type..."));
		return;
	}
	Handle<Object> ptr = args[0]->ToObject();
	if (ptr->InternalFieldCount() != 2) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Object not special.."));
		return;
	}
	Handle<External> ugh = Handle<External>::Cast(ptr->GetInternalField(0));
	SimObject** SimO = static_cast<SimObject**>(ugh->Value());
	if (trySimObjectRef(SimO)) {
		SimObject* this_ = *SimO;
		SimObject** safePtr = (SimObject**)malloc(sizeof(SimObject*));
		SimObject* unSafe = SimSet__getObject((DWORD)this_, args[1]->ToInteger()->Int32Value());
		*safePtr = unSafe;
		SimObject__registerReference(unSafe, safePtr);
		Handle<External> ref = External::New(_Isolate, (void*)safePtr);
		Local<Object> fuck = StrongPersistentTL(objectHandlerTemp)->NewInstance();
		fuck->SetInternalField(0, ref);
		fuck->SetInternalField(1, v8::Boolean::New(_Isolate, true));
		UniquePersistent<Object> out(_Isolate, fuck);
		out.SetWeak<SimObject**>(&safePtr, WeakPtrCallback, v8::WeakCallbackType::kInternalFields);
		args.GetReturnValue().Set(out);
		return;
	}
	else {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Was not able to get safe pointer"));
		return;
	}
}

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

static const char* ts_js_exec(SimObject* this_, int argc, const char* argv[]) {
	Isolate::Scope iso_scope(_Isolate);
	HandleScope handle_scope(_Isolate);
	v8::Context::Scope cScope(ContextL());
	v8::ScriptOrigin origin(String::NewFromUtf8(_Isolate, "<shell>"));
	v8::TryCatch exceptionHandler(_Isolate);
	Local<String> scriptCode = ReadFile(_Isolate, argv[1]);
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
	global->Set(String::NewFromUtf8(_Isolate, "load", NewStringType::kNormal).ToLocalChecked(),
		FunctionTemplate::New(_Isolate, Load));

	Local<ObjectTemplate> globalTS = ObjectTemplate::New(_Isolate);
	globalTS->Set(String::NewFromUtf8(_Isolate, "setVariable", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_setGlobalVar)); //Basically, store the function template as a function object here..
	globalTS->Set(String::NewFromUtf8(_Isolate, "getVariable", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_getGlobalVar));
	globalTS->Set(String::NewFromUtf8(_Isolate, "linkClass", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_linkClass));
	globalTS->Set(String::NewFromUtf8(_Isolate, "registerObject", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_registerObject));
	globalTS->Set(String::NewFromUtf8(_Isolate, "func", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_func));
	globalTS->Set(String::NewFromUtf8(_Isolate, "obj", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_obj));
	Local<ObjectTemplate> SimSet = ObjectTemplate::New(_Isolate);
	SimSet->Set(String::NewFromUtf8(_Isolate, "getObject", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_SimSet_getObject));
	globalTS->Set(_Isolate, "SimSet", SimSet);
	global->Set(_Isolate, "ts", globalTS);
	
	Handle<ObjectTemplate> thisTemplate = ObjectTemplate::New(_Isolate);
	thisTemplate->SetInternalFieldCount(2);
	thisTemplate->SetNamedPropertyHandler(js_getter, js_setter);
	objectHandlerTemp.Reset(_Isolate, thisTemplate);
    Local<v8::Context> context = Context::New(_Isolate, NULL, global);
	_Context.Reset(_Isolate, context);
	_Isolate->Exit();

	ConsoleFunction(NULL, "js_eval", ts_js_eval, "(string script) - Evaluate a script in the JavaScript engine.", 2, 2);
	ConsoleFunction(NULL, "js_exec", ts_js_exec, "(string filename) - Execute a script in the JavaScript engine.", 2, 2);
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
