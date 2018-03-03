#define WIN32_LEAN_AND_MEAN
#pragma once
#pragma comment(lib, "v8_nosnapshot.lib")
#pragma comment(lib, "v8_libsampler.lib")
#pragma comment(lib, "v8_libbase.lib")
#pragma comment(lib, "v8_base_0.lib")
#pragma comment(lib, "v8_base_1.lib")
#pragma comment(lib, "v8_libplatform.lib")
#pragma comment(lib, "v8_init.lib")
#pragma comment(lib, "v8_initializers.lib")
#pragma comment(lib, "libuv.lib")

#include <cstdint>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <chrono>
#include "Torque.h"
#include <vector>
#include "uv8.h"
#include "include/url.h"
#include "sqlite3.h"
#include "tvector.h"


struct sqlite_cb_js {
	Isolate* this_;
	Persistent<Function> cbfn;
	Persistent<Object> objref;
};

#pragma warning( push )
#pragma warning( disable : 4946 )

#define BLJS_VERSION "v8.1.8"

using namespace v8;

static uv_loop_t loop;
static uv_idle_t* idle;
static uv_thread_t thread;
static Namespace* SimSet_ns;
static int verbosity = 0;

//std::map<const char*, UniquePersistent<Module>> mapper;

static bool immediateMode = false;

bool* running = new bool(false);

void Watcher_run(void* arg) {
	bool* running = (bool*)arg;
	if (*running == false) {
		*running = true;
		uv_run(uv_default_loop(), UV_RUN_DEFAULT);
		*running = false;
	}
}

Persistent<Function> import_callback;
Persistent<Function> import_meta_callback;
//static std::map<Identifier*, uv8_cb_handle*> cbWrap;

class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
public:
	virtual void* Allocate(size_t length) {
		void* data = AllocateUninitialized(length);
		return data == NULL ? data : memset(data, 0, length);
	}
	virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
	virtual void Free(void* data, size_t) { free(data); }
};

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

std::string ReadFile(uv_file file) {
	std::string contents;
	uv_fs_t req;
	char buffer_memory[4096];
	uv_buf_t buf = uv_buf_init(buffer_memory, sizeof(buffer_memory));
	int r;

	do {
		r = uv_fs_read(uv_default_loop(),
			&req,
			file,
			&buf,
			1,
			contents.length(),  // offset
			nullptr);
		uv_fs_req_cleanup(&req);

		if (r <= 0)
			break;
		contents.append(buf.base, r);
	} while (true);
	return contents;
}

Platform *_Platform;
Isolate *_Isolate;
Persistent<Context> _Context;
//Persistent<Context, CopyablePersistentTraits<Context>> _Context;
Persistent<ObjectTemplate> objectHandlerTemp;
Persistent<ObjectTemplate> sqliteconstructor;
v8::Local<v8::Context> ContextL() { return StrongPersistentTL(_Context); }

void js_handlePrint(const FunctionCallbackInfo<Value> &args, const char* appendBefore) {
	std::stringstream str; //Good for avoiding the trap that is fixed buffers..

	for (int i = 0; i < args.Length(); i++)
	{
		if (i == 0 && (_stricmp(appendBefore, "") != 0)) {
			str << appendBefore;
		}
		if (i > 0)
			str << " ";
		String::Utf8Value s(args[i]->ToString(_Isolate));
		str << ToCString(s);
	}

	if (str.str().length() > 4095) {
		Printf("String omitted for length..");
	}
	else {
		Printf("%s", str.str().c_str());
	}
}
//Random junk we have up here for support functions.

void* js_malloc(size_t reqsize) {
	_Isolate->AdjustAmountOfExternalAllocatedMemory(reqsize);
	return malloc(reqsize);
}

void Load(const FunctionCallbackInfo<Value> &args) {
	//args.This()->Get
	for (int i = 0; i < args.Length(); i++) {
		HandleScope handle_scope(args.GetIsolate());
		String::Utf8Value file(args.GetIsolate(), args[i]);
		if (*file == nullptr) {
			_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Error loading file"));
			return;
		}
	
		Maybe<uv_file> check = CheckFile(std::string(ToCString(file)), LEAVE_OPEN_AFTER_CHECK);
		if (check.IsNothing()) {
			ThrowError(args.GetIsolate(), "Could not find file");
			return;
		}
		Local<String> source = String::NewFromUtf8(_Isolate, ReadFile(check.FromJust()).c_str());
		if (source.IsEmpty()) {
			_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Error loading file"));
			return;
		}
		uv_fs_t req;
		uv_fs_close(nullptr, &req, check.FromJust(), nullptr);
		uv_fs_req_cleanup(&req);
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
		argv[argc++] = StringTableEntry(quickref.c_str());
		actualObj = obj;
	}
	for (int i = 0; i < args.Length(); i++) {
		if (args[i]->IsObject()) {
			Local<Object> bleh = args[i]->ToObject(_Isolate);
			if (bleh->InternalFieldCount() != 2) {
				//delete argv;
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
						argv[argc++] = StringTableEntry(quickref.c_str());
					}
				}
				else {
					//delete argv;
					_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Object not registered!"));
					return;
				}
			}
		}
		else {
			Handle<String> aa = args[i]->ToString();
			//strcpy((char*)argv[argc++], *String::Utf8Value(aa));
			String::Utf8Value aaaa(aa);
			argv[argc++] = StringTableEntry(ToCString(aaaa), true);
			//argv[argc++] = idk;
		}
	}
	if (ourCall->mType == Namespace::Entry::ScriptFunctionType) {
		if (ourCall->mFunctionOffset) {
			const char* retVal = CodeBlock__exec(
				ourCall->mCode, ourCall->mFunctionOffset,
				ourCall->mNamespace, ourCall->mFunctionName,
				argc, argv, false, ourCall->mNamespace->mPackage,
				0);
			//delete argv;
			MaybeLocal<String> ret = String::NewFromUtf8(_Isolate, retVal, v8::NewStringType::kNormal);
			if (!ret.IsEmpty()) {
				args.GetReturnValue().Set(ret.ToLocalChecked());
			}
			return;
		}
		else {
			return;
		}
	}
	U32 mMinArgs = ourCall->mMinArgs, mMaxArgs = ourCall->mMaxArgs;
	if ((mMinArgs && argc < mMinArgs) || (mMaxArgs && argc > mMaxArgs)) {
		Printf("Expected args between %d and %d, got %d", mMinArgs, mMaxArgs, argc);
		//delete argv;
		//_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Got wrong number of arguments"));
		return;
	}
	switch (ourCall->mType) {
	case Namespace::Entry::StringCallbackType: {
		const char* portisacutie = ourCall->cb.mStringCallbackFunc(actualObj, argc, argv);
		//Ran into this issue from BlocklandLua returning a nullptr...
		if (portisacutie == nullptr || portisacutie == NULL) {
			args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, ""));
		}
		else {
			args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, portisacutie));
		}
		//delete argv;
		return;
	}
	case Namespace::Entry::IntCallbackType:
		args.GetReturnValue().Set(Integer::New(_Isolate, ourCall->cb.mIntCallbackFunc(actualObj, argc, argv)));
		//delete argv;
		return;
	case Namespace::Entry::FloatCallbackType: {
		//Wtf?
		args.GetReturnValue().Set(Number::New(_Isolate, ourCall->cb.mFloatCallbackFunc(actualObj, argc, argv)));
		//delete argv;
		return;
	}
	case Namespace::Entry::BoolCallbackType:
		args.GetReturnValue().Set(Boolean::New(_Isolate, ourCall->cb.mBoolCallbackFunc(actualObj, argc, argv)));
		//delete argv;
		return;
	case Namespace::Entry::VoidCallbackType:
		ourCall->cb.mVoidCallbackFunc(actualObj, argc, argv);
		//delete argv;
		return;
	}
	//delete argv;
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
	if (verbosity > 0) {
		Printf("DEBUG: mNameSpace->mName == ", bleh->mNamespace->mName);
	}
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
	String::Utf8Value ff(prop);
	const char* field = ToCString(ff);
	if (trySimObjectRef(SimO)) {

		SimObject* this_ = *SimO;
		if (_stricmp(field, "mTypeMask") == 0) {
			args.GetReturnValue().Set(Integer::New(_Isolate, this_->mTypeMask));
			return;
		}
		else if (_stricmp(field, "uiName") == 0) {
			//do our hack here
			SimDatablock* aaaaa = (SimDatablock*)this_;
			if (!aaaaa->mFieldDictionary) {
				Printf("mFieldDictionary did not exist ???");
			}
			//Okay then... do a stupid hack for the method.
			//Vector<Field> fuck = (*(Vector<Field>(__thiscall **)(SimObject*))(*(DWORD*)aaaaa + 44))(this_);
			//Printf("NEXT OBJECT ID: %d", aaaaa->sNextObjectId);
			//Printf("NEXT MOD KEY: %d", aaaaa->sNextModifiedKey);
			//Printf("MODIFIED KEY: %d", aaaaa->modifiedKey);
			return;
		}
		//Printf("FLAGS: %d", this_->mFlags);
		Printf("%d %d", (this_->mFlags & SimObject::ModDynamicFields), (this_->mFlags & SimObject::ModStaticFields));
		//if (!this_->mFieldDictionary) {
			//Printf("Non existant mFieldDictionary..");
		//}
		this_->mFlags |= SimObject::ModStaticFields;
		const char* var = SimObject__getDataField(this_, StringTableEntry(field), nullptr);
		if (_stricmp(var, "") == 0) {
			if (ptr->GetInternalField(1)->ToBoolean(_Isolate)->BooleanValue()) {
				Namespace::Entry* entry = fastLookup(this_->mNameSpace->mName, field);
				if (entry != nullptr) {
					//It's a function call.
					Handle<External> theLookup = External::New(_Isolate, (void*)entry);
					Local<Function> blah = FunctionTemplate::New(_Isolate, js_handleCall, theLookup)->GetFunction();
					args.GetReturnValue().Set(blah);
					return;
				}
				else {
					args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, var));
				}
			}
		}
		else {
			args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, var));
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
		String::Utf8Value cprop(prop);
		String::Utf8Value cvalue(value->ToString());
		SimObject__setDataField(this_, ToCString(cprop), StringTableEntry(""), ToCString(cvalue));
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

	Handle<Object> ptr = args[0]->ToObject(_Isolate);
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

void WeakPtrCallback(const v8::WeakCallbackInfo<SimObject*> &data) {
	//Printf("Bitch this shit being Garbage Collected..");
	SimObject** safePtr = (SimObject**)data.GetParameter();
	SimObject* this_ = *safePtr;
	if (this_ != nullptr) {
		SimObject__unregisterReference(this_, safePtr);
		free((void*)safePtr);
	}
}

void js_constructObject(const FunctionCallbackInfo<Value> &args) {
	if (!args.IsConstructCall())
	{
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "js_constructObject not called as constructor.."));
		return;
	}
	//MaybeLocal<Value> aaa = args.Holder()->GetPrivate(args.GetIsolate()->GetCurrentContext(), Private::ForApi(args.GetIsolate(), String::NewFromUtf8(args.GetIsolate(), "className")));

	//Printf("?? %s", *String::Utf8Value(args.This()->GetConstructorName()));

	//const char* className = *String::Utf8Value(args.This()->GetConstructorName());

	if (verbosity > 0) {
		//Printf("DEBUG: className == %s", className);
		Printf("DEBUG: thing1 == %s", *String::Utf8Value(args.This()->GetConstructorName()));
		Printf("DEBUG: thing2 == %s", *String::Utf8Value(args.Holder()->GetConstructorName()));
	}
	//Printf("%s", className);

	String::Utf8Value c_className(args.Holder()->GetConstructorName());

	SimObject* simObj = (SimObject*)AbstractClassRep_create_className(*c_className);
	if (simObj == NULL) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "unable to construct new object"));
		return;
	}
	SimObject** safePtr = (SimObject**)js_malloc(sizeof(SimObject*));
	*safePtr = simObj;
	simObj->mFlags |= SimObject::ModStaticFields;
	simObj->mFlags |= SimObject::ModDynamicFields;
	SimObject__registerReference(simObj, safePtr);
	Handle<External> ref = External::New(_Isolate, (void*)safePtr);
	Local<Object> fuck = StrongPersistentTL(objectHandlerTemp)->NewInstance();
	fuck->SetInternalField(0, ref);
	fuck->SetInternalField(1, v8::Boolean::New(_Isolate, false));
	Persistent<Object> out;
	out.Reset(_Isolate, fuck);
	out.SetWeak<SimObject*>(safePtr, WeakPtrCallback, v8::WeakCallbackType::kParameter);
	if (verbosity > 0) {
		Printf("DEBUG: We're finished here.");
	}

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

	String::Utf8Value varName(args[0]->ToString());
	Local<String> ret = String::NewFromUtf8(_Isolate, GetGlobalVariable(*varName));
	args.GetReturnValue().Set(ret);
	return;
}

void js_setGlobalVar(const FunctionCallbackInfo<Value> &args)
{
	if (args.Length() != 2 || !args[0]->IsString() || !args[1]->IsString()) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "invalid arguments passed to ts_setVariable"));
		return;
	}
	String::Utf8Value cVar(args[0]->ToString());
	String::Utf8Value cVal(args[1]->ToString());

	const char* varName = ToCString(cVar);
	const char* value = ToCString(cVal);

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

	if (verbosity > 0) {
		Printf("DEBUG: data == %s", *String::Utf8Value(args[0]->ToString()));
	}
	Local<FunctionTemplate> temp = FunctionTemplate::New(_Isolate, js_constructObject);
	temp->SetClassName(args[0]->ToString());
	Local<Function> blah = temp->GetFunction();
	//blah->SetPrivate(args.GetIsolate()->GetCurrentContext(), Private::ForApi(args.GetIsolate(), String::NewFromUtf8(args.GetIsolate(), "className")), args[0]);
	//blah->SetInternalField(0, args[0]);
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
		String::Utf8Value name(args[0]);
		find = Sim__findObject_name(ToCString(name));
	}
	else if (args[0]->IsNumber()) {
		find = Sim__findObject_id(args[0]->Int32Value());
	}
	if (find == nullptr) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "could not find object"));
		return;
	}
	SimObject** safePtr = (SimObject**)js_malloc(sizeof(SimObject*));
	*safePtr = find;
	SimObject__registerReference(find, safePtr);
	Handle<External> ref = External::New(_Isolate, (void*)safePtr);
	Local<Object> fuck = StrongPersistentTL(objectHandlerTemp)->NewInstance();
	fuck->SetInternalField(0, ref);
	fuck->SetInternalField(1, v8::Boolean::New(_Isolate, true));
	Persistent<Object> out;
	out.Reset(_Isolate, fuck);
	out.SetWeak<SimObject*>(safePtr, WeakPtrCallback, v8::WeakCallbackType::kParameter);
	args.GetReturnValue().Set(out);
}

void js_func(const FunctionCallbackInfo<Value> &args) {
	if (args.Length() != 1) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Bad arguments passed to ts.func"));
		return;
	}
	String::Utf8Value lookupName(args[0]);
	Namespace::Entry* entry = fastLookup("", ToCString(lookupName));
	if (entry == nullptr || entry == NULL) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "TS function not found"));
		return;
	}

	Handle<External> theLookup = External::New(_Isolate, (void*)entry);
	Local<Function> blah = FunctionTemplate::New(_Isolate, js_plainCall, theLookup)->GetFunction();
	args.GetReturnValue().Set(blah);
}

void js_print(const FunctionCallbackInfo<Value> &args)
{
	js_handlePrint(args, "");
}

int SimSet__getCount(DWORD set)
{
	return *(DWORD*)(set + 0x34);
}


void js_SimSet_getCount(const FunctionCallbackInfo<Value> &args) {
	ThrowArgsNotVal(1);

	if (!args[0]->IsObject()) {
		ThrowBadArg();
	}

	Handle<Object> ptr = args[0]->ToObject(_Isolate);
	if (ptr->InternalFieldCount() != 2) {
		ThrowError(args.GetIsolate(), "Not a valid wrapper of a TS object.");
		return;
	}

	Handle<External> ugh = Handle<External>::Cast(ptr->GetInternalField(0));
	SimObject** SimO = static_cast<SimObject**>(ugh->Value());
	if (trySimObjectRef(SimO)) {
		SimObject* this_ = *SimO;
		if (_stricmp(this_->mNameSpace->mName, "SimSet") == 0 || _stricmp(this_->mNameSpace->mName, "SimGroup") == 0) {
			int count = SimSet__getCount((DWORD)this_);
			args.GetReturnValue().Set(Int32::New(args.GetIsolate(), count));
		}
		else {
			Printf("%s", this_->mNameSpace->mName);
			ThrowError(args.GetIsolate(), "Not a SimSet object.");
		}
	}
	else {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Was not able to get safe pointer"));
		return;
	}
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
	Handle<Object> ptr = args[0]->ToObject(_Isolate);
	if (ptr->InternalFieldCount() != 2) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Object not special.."));
		return;
	}
	Handle<External> ugh = Handle<External>::Cast(ptr->GetInternalField(0));
	SimObject** SimO = static_cast<SimObject**>(ugh->Value());
	if (trySimObjectRef(SimO)) {
		SimObject* this_ = *SimO;
		SimObject** safePtr = (SimObject**)js_malloc(sizeof(SimObject*));
		if (_stricmp(this_->mNameSpace->mName, "SimSet") == 0 || _stricmp(this_->mNameSpace->mName, "SimGroup") == 0) {
			int count = SimSet__getCount((DWORD)this_);
			int index = args[1]->Int32Value();
			if (index < count && index >= 0) { //Indexed from 0..
				SimObject* unSafe = SimSet__getObject((DWORD)this_, index);
				*safePtr = unSafe;
				SimObject__registerReference(unSafe, safePtr);
				Handle<External> ref = External::New(_Isolate, (void*)safePtr);
				Local<Object> fuck = StrongPersistentTL(objectHandlerTemp)->NewInstance();
				fuck->SetInternalField(0, ref);
				fuck->SetInternalField(1, v8::Boolean::New(_Isolate, true));
				Persistent<Object> out;
				out.Reset(_Isolate, fuck);
				out.SetWeak<SimObject*>(safePtr, WeakPtrCallback, v8::WeakCallbackType::kParameter);
				args.GetReturnValue().Set(out);
				return;
			}
			else {
				ThrowError(args.GetIsolate(), "Out of range.");
			}
		}
		else {
			ThrowError(args.GetIsolate(), "Not a SimSet object.");
		}
	}
	else {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Was not able to get safe pointer"));
		return;
	}
}

void js_console_log(const FunctionCallbackInfo<Value> &args) {
	js_handlePrint(args, "");
}

void js_console_warn(const FunctionCallbackInfo<Value> &args) {
	js_handlePrint(args, "\x04");
}

void js_console_error(const FunctionCallbackInfo<Value> &args) {
	js_handlePrint(args, "\x03");
}

void js_console_info(const FunctionCallbackInfo<Value> &args) {
	js_handlePrint(args, "\x05");
}

static const char* ts_js_eval(SimObject* this_, int argc, const char* argv[]) {
	Locker locker(_Isolate);
	Isolate::Scope iso_scope(_Isolate);
	_Isolate->Enter();
	HandleScope handle_scope(_Isolate);
	ContextL()->Enter();
	v8::Context::Scope cScope(ContextL());
	//v8::ScriptOrigin origin(String::NewFromUtf8(_Isolate, "<shell>"));
	v8::TryCatch exceptionHandler(_Isolate);
	Local<String> scriptCode = String::NewFromUtf8(_Isolate, argv[1], NewStringType::kNormal).ToLocalChecked();
	ScriptOrigin scriptorigin(String::NewFromUtf8(_Isolate, "<shell>"),
		Integer::New(_Isolate, 0),
		Integer::New(_Isolate, 0),
		False(_Isolate),
		Integer::New(_Isolate, 0),
		String::NewFromUtf8(_Isolate, ""),
		False(_Isolate),
		False(_Isolate),
		False(_Isolate));
	Local<Script> script;
	Local<Value> result;
	ScriptCompiler::Source aaa(scriptCode, scriptorigin);
	const char* retval = "true";
	if (!ScriptCompiler::Compile(ContextL(), &aaa).ToLocal(&script))
	{
		ReportException(_Isolate, &exceptionHandler);
		return "false";
	}
	else {
		//uv_thread_create(&thread, Watcher_run, (void*)running); //Again, start up the event loop. We recieved new I/o.
		result = script->Run();
		if (result.IsEmpty()) {
			ReportException(_Isolate, &exceptionHandler);
			return "false";
		}
		if (result->IsString()) {
			String::Utf8Value a(result->ToString());
			retval = *a;
		}
	}
	ContextL()->Exit();
	_Isolate->Exit();
	Unlocker unlocker(_Isolate);
	return retval;
}

static const char* ts_js_exec(SimObject* this_, int argc, const char* argv[]) {
	Locker locker(_Isolate);
	Isolate::Scope iso_scope(_Isolate);
	_Isolate->Enter();
	HandleScope handle_scope(_Isolate);
	ContextL()->Enter();
	v8::Context::Scope cScope(ContextL());
	v8::ScriptOrigin origin(String::NewFromUtf8(_Isolate, argv[1]));
	v8::TryCatch exceptionHandler(_Isolate);
	Maybe<uv_file> check = CheckFile(std::string(argv[1]), LEAVE_OPEN_AFTER_CHECK);
	if (check.IsNothing()) {
		Printf("\x03 Could not find file..");
		return "false";
	}
	Local<String> scriptCode = String::NewFromUtf8(_Isolate, ReadFile(check.FromJust()).c_str());
	uv_fs_t req;
	uv_fs_close(nullptr, &req, check.FromJust(), nullptr);
	uv_fs_req_cleanup(&req);
	Local<Script> script;
	const char* retval = "true";
	if (!Script::Compile(ContextL(), scriptCode, &origin).ToLocal(&script))
	{
		ReportException(_Isolate, &exceptionHandler);
		return "false";
	}
	else {
		//uv_thread_create(&thread, Watcher_run, (void*)running); //Start up the event loop because we've recieved new I/O.
		if (script->Run(ContextL()).IsEmpty()) {
			ReportException(_Isolate, &exceptionHandler);
			return "false";
		}
	}
	ContextL()->Exit();
	_Isolate->Exit();
	Unlocker unlocker(_Isolate);
	return retval;
}

void js_switchToTS(const FunctionCallbackInfo<Value> &args) {
	Eval("ConsoleEntry.altCommand = \"ConsoleEntry::eval();\";");
}

static void ts_switchToJS(SimObject* this_, int argc, const char* argv[]) {
	Eval("function ConsoleEntry::jsEval(){%text = ConsoleEntry.getValue();ConsoleEntry.setText(\"\");echo(\"==>\" @ %text);js_eval(%text);}ConsoleEntry.altCommand = \"ConsoleEntry::jsEval(); \";");
}
v8::MaybeLocal<v8::Module> ModuleResolveCallback(
	v8::Local<v8::Context> context, v8::Local<v8::String> specifier,
	v8::Local<v8::Module> referrer);

MaybeLocal<Module> getModuleFromSpecifier(Isolate* iso, Handle<String> spec) {
	Handle<String> mod = spec;
	String::Utf8Value js_aaaa(iso, mod);
	const char* aaaa = ToCString(js_aaaa);
	const char* requestedModule;
	std::string fuck;
	if (aaaa[0] == '.' && aaaa[1] == '/') {
		String::Utf8Value wtf_was_i_thinking(String::Concat(String::NewFromUtf8(iso, "./"), String::Concat(mod, String::NewFromUtf8(iso, ".js"))));
		requestedModule = ToCString(wtf_was_i_thinking);
		fuck = std::string(requestedModule);
	}
	else {
		String::Utf8Value wtf_was_i_smoking(String::Concat(String::Concat(String::NewFromUtf8(iso, "Add-Ons/"), mod), String::NewFromUtf8(iso, "/")));
		requestedModule = ToCString(wtf_was_i_smoking);
		fuck = std::string(requestedModule);
		fuck.append("index.js");
	}

	Maybe<uv_file> check = CheckFile(fuck, LEAVE_OPEN_AFTER_CHECK);
	if (check.IsNothing()) {
		Printf("Couldn't find %s", fuck.c_str());
		return MaybeLocal<Module>();
	}
	size_t sz = _MAX_PATH * 2;
	char path[_MAX_PATH * 2];
	uv_cwd(path, &sz);
	std::string file = ReadFile(check.FromJust());
	Handle<String> source = String::NewFromUtf8(iso, file.c_str());
	ScriptOrigin scriptorigin(spec,
		Integer::New(iso, 0),
		Integer::New(iso, 0),
		False(iso),
		Integer::New(iso, 0),
		String::NewFromUtf8(iso, ""),
		False(iso),
		False(iso),
		True(iso));
	Local<Script> script;
	ScriptCompiler::Source aaa(source, scriptorigin);
	//Local<Value> exports;
	v8::TryCatch exceptionHandler(iso);
	Handle<Module> modu;
	if (!ScriptCompiler::CompileModule(iso, &aaa).ToLocal(&modu)) {
		Printf("Compiling failed");
		ReportException(iso, &exceptionHandler);
		return MaybeLocal<Module>();
	}
	/*
	if (!modu->InstantiateModule(ContextL(), ModuleResolveCallback).FromMaybe(false)) {
		ReportException(iso, &exceptionHandler);
		Printf("Unable to instantiate module");
		return MaybeLocal<Module>();
	}
	Handle<Value> exports;
	if (!modu->Evaluate(ContextL()).ToLocal(&exports)) {
		ReportException(iso, &exceptionHandler);
		Printf("Module evaluation fail");
		return MaybeLocal<Module>();
	}
	*/
	//mapper.insert(mapper.end(), std::make_pair(aaaa, UniquePersistent<Module>(_Isolate, modu)));
	uv_fs_t req;
	uv_fs_close(nullptr, &req, check.FromJust(), nullptr);
	uv_fs_req_cleanup(&req);
	return MaybeLocal<Module>(modu);
}

v8::MaybeLocal<v8::Module> ModuleResolveCallback(
	v8::Local<v8::Context> context, v8::Local<v8::String> specifier,
	v8::Local<v8::Module> referrer) {
	//IsolateData* data = IsolateData::FromContext(context);
	Isolate* iso = context->GetIsolate();
	return getModuleFromSpecifier(iso, specifier);
}


void js_require(const FunctionCallbackInfo<Value> &args) {
	auto iso = args.GetIsolate();
	if (args.Length() != 1 || !args[0]->IsString()) {
		ThrowError(iso, "Arguments were bad..");
		return;
	}
	Handle<String> mod = args[0]->ToString(_Isolate);
	MaybeLocal<Module> mod2 = getModuleFromSpecifier(iso, mod);
	Local<Module> modu;
	if (!mod2.ToLocal(&modu)) {
		Printf("Unable to convert module");
		return;
	}
	
	if (!modu->InstantiateModule(ContextL(), ModuleResolveCallback).FromMaybe(false)) {
		//ReportException(iso, &exceptionHandler);
		Printf("Unable to instantiate module");
		return;
	}
	Handle<Value> exports;
	if (!modu->Evaluate(ContextL()).ToLocal(&exports)) {
		//ReportException(iso, &exceptionHandler);
		Printf("Module evaluation fail");
		return;
	}
	
//	const char* aaaaa = *String::Utf8Value(args[0]);
//	Printf("Stored %s!", aaaaa);


	args.GetReturnValue().Set(exports);
}

void js_version(const FunctionCallbackInfo<Value> &args) {
	const char* ver = V8::GetVersion();
	Handle<Object> versions = Object::New(args.GetIsolate());
	versions->Set(String::NewFromUtf8(args.GetIsolate(), "v8"), String::NewFromUtf8(args.GetIsolate(), ver));
	versions->Set(String::NewFromUtf8(args.GetIsolate(), "blocklandjs"), String::NewFromUtf8(args.GetIsolate(), BLJS_VERSION));
	versions->Set(String::NewFromUtf8(args.GetIsolate(), "libuv"), String::NewFromUtf8(args.GetIsolate(), uv_version_string()));
	args.GetReturnValue().Set(versions);
}


static const char* ts_js_bridge(SimObject* this_, int argc, const char* argv[]) {
	Locker locker(_Isolate);
	_Isolate->Enter();
	Isolate::Scope iso_scope(_Isolate);
	HandleScope handle_scope(_Isolate);
	v8::Context::Scope cScope(ContextL());
	v8::TryCatch exceptionHandler(_Isolate);
	ContextL()->Enter();
	//Handle<Array> identifier = Array::New(_Isolate);
	Handle<Object> globalMappingTable = ContextL()->Global()->Get(String::NewFromUtf8(_Isolate, "__mappingTable__"))->ToObject();
	const char* fnName = argv[0];
	bool passThisVal = false;
	const char* nsVal;
	if (this_ != nullptr) {
		//identifier->Set(String::NewFromUtf8(_Isolate, "namespace"), String::NewFromUtf8(_Isolate, this_->mNameSpace->mName));
		nsVal = this_->mNameSpace->mName;
		passThisVal = true;
	}
	else {
		nsVal = "ts";
	}
	Handle<String> identifier = String::Concat(String::NewFromUtf8(_Isolate, nsVal), String::Concat(String::NewFromUtf8(_Isolate, "__"), String::NewFromUtf8(_Isolate, fnName)));
	//identifier->Set(String::NewFromUtf8(_Isolate, "namespace"), String::NewFromUtf8(_Isolate, nsVal));
	//identifier->Set(String::NewFromUtf8(_Isolate, "name"), String::NewFromUtf8(_Isolate, fnName));

	const char* cRet;
	if (globalMappingTable->Has(identifier)) {
		//MaybeLocal<Value> unsafe = ContextL()->Global()->Get(StrongPersistentTL(_Context), String::NewFromUtf8(_Isolate, fnName));

		Handle<Value> args[19];
		if (passThisVal) {
			Handle<Object> thisVal = StrongPersistentTL(objectHandlerTemp)->NewInstance();
			SimObject** safePtr = (SimObject**)js_malloc(sizeof(SimObject*));
			*safePtr = this_;
			SimObject__registerReference(this_, safePtr);
			Handle<External> ref = External::New(_Isolate, (void*)safePtr);
			thisVal->SetInternalField(0, ref);
			thisVal->SetInternalField(1, True(_Isolate));
			Persistent<Object> out(_Isolate, thisVal);
			out.SetWeak<SimObject*>(safePtr, WeakPtrCallback, v8::WeakCallbackType::kParameter);
			args[0] = StrongPersistentTL(out);
			for (int i = 1; i < argc; i++) {
				args[i] = String::NewFromUtf8(_Isolate, argv[i]);
			}
		}
		else {
			for (int i = 1; i < argc; i++) {
				args[i - 1] = String::NewFromUtf8(_Isolate, argv[i]);
			}
		}

		Handle<Function> theCallback = Handle<Function>::Cast(globalMappingTable->Get(identifier)->ToObject());
		Handle<Value> theRet = theCallback->Call(ContextL()->Global(), (passThisVal ? argc : argc - 1), args);

		if (!theRet.IsEmpty()) {
			if (theRet->IsNullOrUndefined()) {
				cRet = "";
			}
			else {
				String::Utf8Value c_theRet(theRet);
				cRet = ToCString(c_theRet);
			}
		}
		else {
			cRet = "";
		}
	}
	else {
		cRet = "";
	}
	ContextL()->Exit();
	_Isolate->Exit();
	Unlocker unlocker(_Isolate);
	return cRet;
}

void js_expose(const FunctionCallbackInfo<Value> &args) {
	//ThrowArgsNotVal(2);
	if (args.Length() != 2) {
		ThrowError(args.GetIsolate(), "Wrong amount of arguments.");
		return;
	}

	if (!args[0]->IsObject()) {
		ThrowBadArg();
	}

	if (!args[1]->IsFunction()) {
		ThrowBadArg();
	}

	Isolate* this_ = args.GetIsolate();
	Handle<String> ns = String::NewFromUtf8(this_, "class");
	Handle<String> fnn = String::NewFromUtf8(this_, "name");
	Handle<String> descKey = String::NewFromUtf8(this_, "description");

	Handle<Object> theArgs = args[0]->ToObject();

	const char* ns1 = "ts";

	const char* fnName = "";

	const char* desc = "registered from BLJS";

	if (theArgs->Has(descKey)) {
		String::Utf8Value c_desc(theArgs->Get(descKey)->ToString());
		desc = *c_desc;
	}

	if (!theArgs->Has(fnn)) {
		ThrowError(this_, "Required field missing.");
	}
	String::Utf8Value c_fnName(theArgs->Get(fnn)->ToString());
	fnName = *c_fnName;

	if (theArgs->Has(ns)) {
		String::Utf8Value c_ns(theArgs->Get(ns)->ToString());
		ns1 = *c_ns;
	}



	//Handle<String> keyn = String::NewFromUtf8(this_, "desc");
	//Handle<String> description = String::NewFromUtf8(this_, "registered from JS");


	Handle<Function> passedFunction = Handle<Function>::Cast(args[1]->ToObject());
	
	Handle<Object> globalMappingTable = ContextL()->Global()->Get(String::NewFromUtf8(this_, "__mappingTable__"))->ToObject();

	//cbWrap.insert(cbWrap.end(), std::make_pair(id, cb));

	if (_stricmp(ns1, fnName) == 0) {
		//Printf("Working around really weird bug...");
		//ns1 = "";
		//ThrowError(args.GetIsolate(), "BUG ENCOUNTERED!");
		//ns1 = ToCString(String::Utf8Value(nsjs));
		//fnName = ToCString(String::Utf8Value(fnJS));
		Printf("WTF?? %s == %s ???", ns1, fnName);
		//Printf("??? %s, %s", *String::Utf8Value(nsjs), *String::Utf8Value(fnJS));
		//if (_stricmp(ns1, fnName) == 0) {
		//	ThrowError(this_, "Giving up.");
		//	return;
		//}
		return;
	}

	if (_stricmp(ns1, "ts") == 0) {
		//ns1 = "ts";
		ConsoleFunction(NULL, fnName, ts_js_bridge, StringTableEntry(desc), 1, 23);
	}
	else {
		//Printf("NS IS %s !!", ns1);
		ConsoleFunction(ns1, fnName, ts_js_bridge, StringTableEntry(desc), 1, 23);
	}
	//Printf("Registered %s::%s", ns1, fnName);
	Handle<String> ide1 = String::Concat(String::NewFromUtf8(this_, ns1), String::NewFromUtf8(this_, "__"));
	Handle<String> ide = String::Concat(ide1, String::NewFromUtf8(this_, fnName));
	globalMappingTable->Set(ide, passedFunction);
	args.GetReturnValue().Set(True(this_));
}


static MaybeLocal<Promise> ImportModuleDynam(Local<Context> ctx, Local<ScriptOrModule> ref, Local<String> spec) {
	Isolate* iso = ctx->GetIsolate();
	EscapableHandleScope handle_scope(iso);
	auto resolv = Promise::Resolver::New(ctx);
	Local<Promise::Resolver> resolver = resolv.ToLocalChecked();
	if (_Context != ctx) {
		Local<Value> err = Exception::Error(String::NewFromUtf8(iso, "import() called outside of main context"));
		if (resolver->Reject(ctx, err).IsJust()) {
			return handle_scope.Escape(resolver.As<Promise>());
		}
	}
	String::Utf8Value c_spec(spec);
	std::string speci = *c_spec;
	//MaybeLocal<Module> modu = mapper[speci.c_str()].Get(_Isolate);
	//Printf("Got module %s", speci.c_str());
	Handle<Module> mod2;
	if(!getModuleFromSpecifier(iso, spec).ToLocal(&mod2)) {
		resolver->Reject(Exception::Error(String::NewFromUtf8(iso, "Could not find file.")));
		return handle_scope.Escape(resolver.As<Promise>());
	}
	Local<Value> result;
	if (!mod2->InstantiateModule(ContextL(), ModuleResolveCallback).FromMaybe(false)) {
		//ReportException(iso, &exceptionHandler);
		resolver->Reject(Exception::Error(String::NewFromUtf8(iso, "Unable to instantiate module")));
		return handle_scope.Escape(resolver.As<Promise>());
	}
	if (!mod2->Evaluate(ContextL()).ToLocal(&result)) {
		resolver->Reject(Exception::Error(String::NewFromUtf8(iso, "Module evaluation failed")));
		return handle_scope.Escape(resolver.As<Promise>());
	}
	resolver->Resolve(mod2->GetModuleNamespace());
	return handle_scope.Escape(resolver.As<Promise>());
}

void idle_cb(uv_idle_t* handle) {
	if (!immediateMode) {
		Sleep(1); //Just sleep so CPU usage doesn't go insane.
	}
}

void RetStuff(Local<Context> b, Local<Module> c, Local<Object> d) {
	Isolate* isolate = b->GetIsolate();
}

void sqlite3gc(const v8::WeakCallbackInfo<sqlite3*> &data) {
	//Printf("Bitch this shit being Garbage Collected..");
	sqlite3** safePtr = (sqlite3**)data.GetParameter();
	sqlite3* this_ = *safePtr;
	if (this_ != nullptr) {
		//Fuck it.
		sqlite3_close(this_);
		//free((void*)this_);
	}
	free((void*)safePtr);
}

static int sqlite3Callback(void *wrapper, int argc, char **argv, char **azColName)
{
	sqlite_cb_js* aaa = (sqlite_cb_js*)wrapper;
	//So I don't know if this is async or sync. I'm just going to do it how I usually do.
	Isolate* this_ = aaa->this_;
	Locker locker(aaa->this_);
	Isolate::Scope iso_scope(this_);
	this_->Enter();
	HandleScope handle_scope(this_);
	ContextL()->Enter();
	v8::Context::Scope cScope(ContextL());
	if (!aaa->cbfn.IsEmpty()) {
		Handle<Value> args[1];
		Handle<Array> results = Array::New(this_);
		for (int i = 0; i < argc; i++) {
			results->Set(i, String::NewFromUtf8(this_, argv[i]));
		}
		args[0] = results;
		StrongPersistentTL(aaa->cbfn)->Call(StrongPersistentTL(aaa->objref), 1, args);
	}
	delete aaa;
	ContextL()->Exit();
	this_->Exit();
	Unlocker unlocker(this_);
	return 0;
}

void js_sqlite_new(const FunctionCallbackInfo<Value> &args) {
	Handle<Object> this_ = StrongPersistentTL(sqliteconstructor)->NewInstance();
	sqlite3* db = nullptr;

	sqlite3** weakPtr = (sqlite3**)uv8_malloc(args.GetIsolate(), sizeof(*weakPtr));
	*weakPtr = db;

	this_->SetInternalField(0, External::New(args.GetIsolate(), (void*)weakPtr));
	this_->SetInternalField(1, False(args.GetIsolate()));
	Persistent<Object> aaaa;
	aaaa.Reset(args.GetIsolate(), this_);
	aaaa.SetWeak<sqlite3*>(weakPtr, sqlite3gc, v8::WeakCallbackType::kParameter);
	args.GetReturnValue().Set(aaaa);
}

sqlite3* getDB(const FunctionCallbackInfo<Value> &args) {
	return *(sqlite3**)Handle<External>::Cast(args.This()->GetInternalField(0))->Value();
}

bool dbOpened(const FunctionCallbackInfo<Value> &args) {
	return args.This()->GetInternalField(1)->BooleanValue();
}

void js_sqlite_open(const FunctionCallbackInfo<Value> &args) {
	ThrowArgsNotVal(1);
	if (!args[0]->IsString()) {
		ThrowBadArg();
	}

	sqlite3* this_ = getDB(args);
	if (dbOpened(args)) {
		sqlite3_close(this_);
	}
	String::Utf8Value c_db(args[0]->ToString());
	const char* db = ToCString(c_db);
	int ret = sqlite3_open(db, (sqlite3**)Handle<External>::Cast(args.This()->GetInternalField(0))->Value());
	if (ret) {
		ThrowError(args.GetIsolate(), "Unable to open sqlite database");
		return;
	}
	else {
		args.This()->SetInternalField(1, True(args.GetIsolate()));
	}
}

void js_sqlite_exec(const FunctionCallbackInfo<Value> &args) {
	if (args.Length() < 1) {
		ThrowError(args.GetIsolate(), "Not enough arguments supplied..");
		return;
	}
	if (!args[0]->IsString()) {
		ThrowBadArg();
	}

	if (args.Length() == 2) {
		if (!args[1]->IsFunction()) {
			ThrowBadArg();
		}
	}

	sqlite3* this_ = getDB(args);
	String::Utf8Value c_query(args[0]->ToString());
	const char* query = ToCString(c_query);
	if (dbOpened(args)) {
		sqlite_cb_js* cbinfo = new sqlite_cb_js;
		if (args.Length() == 2) {
			cbinfo->cbfn.Reset(args.GetIsolate(), Handle<Function>::Cast(args[1]->ToObject()));
		}
		else {
			cbinfo->cbfn.Reset(args.GetIsolate(), FunctionTemplate::New(args.GetIsolate())->GetFunction());
		}
		cbinfo->objref.Reset(args.GetIsolate(), args.This());
		cbinfo->this_ = args.GetIsolate();
		char* err = 0;
		int ret = sqlite3_exec(this_, query, sqlite3Callback, cbinfo, &err);
		if (ret != SQLITE_OK) {
			Printf("Error: %s", err);
			ThrowError(args.GetIsolate(), "SQLite error encountered.");
			sqlite3_free(err);
			delete cbinfo;
			return;
		}
	}
	else {
		ThrowError(args.GetIsolate(), "DB not opened");
		return;
	}
}

void js_sqlite_close(const FunctionCallbackInfo<Value> &args) {
	sqlite3* this_ = getDB(args);
	if (dbOpened(args)) {
		sqlite3_close(this_);
		args.This()->SetInternalField(1, False(args.GetIsolate()));
	}
	else {
		ThrowError(args.GetIsolate(), "SQLite database was never opened..");
		return;
	}
}

void js_modifyVerbosity(const FunctionCallbackInfo<Value> &args) {
	if (args.Length() != 1) {
		ThrowError(args.GetIsolate(), "Wrong amount of arguments given.");
		return;
	}

	if (!args[0]->IsNumber()) {
		ThrowError(args.GetIsolate(), "Wrong type of argument.");
		return;
	}

	verbosity = args[0]->Int32Value();
}

void js_setImmediateMode(const FunctionCallbackInfo<Value> &args) {
	if (args.Length() != 1) {
		ThrowError(args.GetIsolate(), "Wrong amount of args given.");
		return;
	}
	if (!args[0]->IsBoolean()) {
		ThrowError(args.GetIsolate(), "Wrong amount of args given.");
		return;
	}
	immediateMode = args[0]->BooleanValue();
}

bool init()
{
	if (!torque_init())
	{
		return false;
	}


	Printf("BlocklandJS || Init");
	Printf("BlocklandJS: version %s", V8::GetVersion());
	//Startup the libuv loop here
	uv_loop_init(uv_default_loop());
	_Platform = platform::CreateDefaultPlatform();
	V8::InitializePlatform(_Platform);
	const char* v8flags = "--harmony --harmony-dynamic-import";
	V8::SetFlagsFromString(v8flags, strlen(v8flags));
	V8::Initialize();

	Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
	_Isolate = Isolate::New(create_params);
	_Isolate->Enter();

	_Isolate->SetData(0, (void*)uv_default_loop());

	HandleScope scope(_Isolate);

	/* global */
	Local<ObjectTemplate> global = ObjectTemplate::New(_Isolate);
	global->Set(_Isolate, "print", FunctionTemplate::New(_Isolate, js_print));
	global->Set(_Isolate, "load", FunctionTemplate::New(_Isolate, Load));
	//global->Set(_Isolate, "require", FunctionTemplate::New(_Isolate, js_require));
	global->Set(_Isolate, "version", FunctionTemplate::New(_Isolate, js_version));
	global->Set(_Isolate, "__mappingTable__", ObjectTemplate::New(_Isolate));
	global->Set(_Isolate, "immediateMode", FunctionTemplate::New(_Isolate, js_setImmediateMode));
	global->Set(_Isolate, "__verbosity__", FunctionTemplate::New(_Isolate, js_modifyVerbosity));

	Local<FunctionTemplate> sqlite = FunctionTemplate::New(_Isolate, js_sqlite_new);
	sqlite->SetClassName(String::NewFromUtf8(_Isolate, "sqlite"));
	sqlite->InstanceTemplate()->Set(_Isolate, "open", FunctionTemplate::New(_Isolate, js_sqlite_open));
	sqlite->InstanceTemplate()->Set(_Isolate, "close", FunctionTemplate::New(_Isolate, js_sqlite_close));
	sqlite->InstanceTemplate()->Set(_Isolate, "exec", FunctionTemplate::New(_Isolate, js_sqlite_exec));
	sqlite->InstanceTemplate()->SetInternalFieldCount(2);
	global->Set(_Isolate, "sqlite", sqlite);
	sqliteconstructor.Reset(_Isolate, sqlite->InstanceTemplate());
	/* console */
	Local<ObjectTemplate> console = ObjectTemplate::New(_Isolate);
	console->Set(_Isolate, "log", FunctionTemplate::New(_Isolate, js_console_log));
	console->Set(_Isolate, "warn", FunctionTemplate::New(_Isolate, js_console_warn));
	console->Set(_Isolate, "error", FunctionTemplate::New(_Isolate, js_console_error));
	console->Set(_Isolate, "info", FunctionTemplate::New(_Isolate, js_console_info));

	/* ts */
	Local<ObjectTemplate> globalTS = ObjectTemplate::New(_Isolate);
	globalTS->Set(String::NewFromUtf8(_Isolate, "setVariable", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_setGlobalVar)); //Basically, store the function template as a function object here..
	globalTS->Set(String::NewFromUtf8(_Isolate, "getVariable", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_getGlobalVar));
	globalTS->Set(String::NewFromUtf8(_Isolate, "linkClass", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_linkClass));
	globalTS->Set(String::NewFromUtf8(_Isolate, "registerObject", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_registerObject));
	globalTS->Set(String::NewFromUtf8(_Isolate, "func", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_func));
	globalTS->Set(String::NewFromUtf8(_Isolate, "obj", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_obj));
	globalTS->Set(String::NewFromUtf8(_Isolate, "switchToTS", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_switchToTS));
	globalTS->Set(String::NewFromUtf8(_Isolate, "expose", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_expose));
	/* ts.simSet */

	Local<ObjectTemplate> SimSet = ObjectTemplate::New(_Isolate);
	SimSet->Set(String::NewFromUtf8(_Isolate, "getObject", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_SimSet_getObject));
	SimSet->Set(_Isolate, "getCount", FunctionTemplate::New(_Isolate, js_SimSet_getCount));
	globalTS->Set(_Isolate, "SimSet", SimSet);

	_Isolate->SetHostImportModuleDynamicallyCallback(ImportModuleDynam);
	_Isolate->SetHostInitializeImportMetaObjectCallback(RetStuff);

	//Local<String> scriptCode = String::NewFromUtf8(_Isolate, "function LRQ(dirname){");

	SimSet_ns = LookupNamespace("SimSet");
	uv8_bind_all(_Isolate, global);
	global->Set(_Isolate, "ts", globalTS);
	global->Set(_Isolate, "console", console);
	Handle<ObjectTemplate> thisTemplate = ObjectTemplate::New(_Isolate);
	thisTemplate->SetInternalFieldCount(2);

	thisTemplate->SetNamedPropertyHandler(js_getter, js_setter);
	objectHandlerTemp.Reset(_Isolate, thisTemplate);
	Local<v8::Context> context = Context::New(_Isolate, NULL, global);

	//context->Global()->Set(String::NewFromUtf8(_Isolate, "global"), context->Global()->New(_Isolate));
	_Context.Reset(_Isolate, context);
	_Isolate->Exit();
	ConsoleFunction(NULL, "js_eval", ts_js_eval, "(string script) - Evaluate a script in the JavaScript engine.", 2, 2);
	ConsoleFunction(NULL, "js_exec", ts_js_exec, "(string filename) - Execute a script in the JavaScript engine.", 2, 2);
	ConsoleFunction(NULL, "switchToJS", ts_switchToJS, "() - Switch your console to use JavaScript.", 1, 1);
	idle = (uv_idle_t*)malloc(sizeof(*idle));
	uv_idle_init(uv_default_loop(), idle);
	uv_idle_start(idle, idle_cb);
	//I'm way too lazy for this shit.
	//uv_unref((uv_handle_t*)idle);
	uv_thread_create(&thread, Watcher_run, (void*)running);
	//uv_disable_stdio_inheritance();
	//_run(uv_default_loop(), UV_RUN_DEFAULT);
	Printf("BlocklandJS || Attached");
	return true;
}

bool deinit() {
	uv_unref((uv_handle_t*)idle);
	uv_loop_close(uv_default_loop());
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
