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
#pragma comment(lib, "icui18n.lib")
#pragma comment(lib, "icuuc.lib")

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
static int verbosity = 0;

//std::map<const char*, UniquePersistent<Module>> mapper;

static bool immediateMode = false;

static std::unordered_map<const char*, Vector<Field>> opt_1;

static std::unordered_map< std::string ,  Field*> opt_2; //unordered_map is used to store Fields to avoid iterating through the vector again..

static std::unordered_map<const char*, void*> opt_3; //used to retreive the class representation

static std::map<const char*, SimObject**> opt_4; //caching for ts.obj, used to avoid doing more lookups to ts then necessary.s
static std::unordered_map<int, SimObject**> opt_5;

static std::unordered_map<const char*, SimObject**> opt_6; 

static int* typeSize = (int*)0x752F78;
static int* types = (int*)0x7557A0;

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
	if (check != nullptr) {
		if (*check != nullptr) {
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
	Handle<Object> ptr = args.This();
	Handle<External> ugh = Handle<External>::Cast(ptr->GetInternalField(0));
	SimObject** SimO = static_cast<SimObject**>(ugh->Value()); //Grab the wrapping weak reference to the TS object.
	String::Utf8Value ff(prop);
	const char* field = ToCString(ff);
	if (trySimObjectRef(SimO)) {
		SimObject* this_ = *SimO;
		if (field[0] == '_' && field[1] == '_') { //Bypass for a quick function call.
			std::string str(field);
			str.erase(0, 2);
			Namespace::Entry* entry = passThroughLookup(this_->mNameSpace, str.c_str());
			if (entry != nullptr) {
				Handle<External> theLookup = External::New(_Isolate, (void*)entry);
				Local<Function> blah = FunctionTemplate::New(_Isolate, js_handleCall, theLookup)->GetFunction();
				args.GetReturnValue().Set(blah);
			}
			else {
				ThrowError(args.GetIsolate(), "Could not find function.");
			}
	
			return;
		}
		void* aaa = (void*)(*(DWORD*)this_ + 0x4); //Retrieve the class representation from the SimObject.
		void* classRep = nullptr;
		std::unordered_map<const char*, void*>::iterator it2;
		it2 = opt_3.find(this_->mNameSpace->mName);
		if (it2 != opt_3.end()) {
			classRep = it2->second;
		}
		else {
			classRep = (*(void*(**)())((DWORD)aaa - 4))();
			opt_3.insert(std::make_pair(this_->mNameSpace->mName, classRep));
		}
		std::unordered_map<std::string, Field*>::iterator it;
		Vector<Field> &fuck = *(Vector<Field>*)((DWORD)classRep + 0x2C); //Retrieve the backing mFieldList from the class representation.
		if(verbosity > 0) {
			Printf("%p", SimObject__getDataField);
			Printf("%p", ((DWORD)classRep + 0x2C));
			Printf("%p", classRep);
			Printf("%d", fuck.size());
			Printf("%p", fuck.address());
			Printf("%d", sizeof(Field));
		}
		Field* f = nullptr;
		std::string id(this_->mNamespace->mName);
		id.append(field);
		it = opt_2.find(id);
		if (it != opt_2.end()) {
			f = it->second;
			delete id;
		}
		else {
			for (Vector<Field>::iterator itr = fuck.begin(); itr != fuck.end(); itr++) {
				Field* aa = itr;
				if (aa != nullptr) {
					if (aa->pGroupname != nullptr) {
						//Printf(" == GROUP: %s ==", f->pGroupname);
					}
					else {
						if (_stricmp(aa->pFieldname, id->mName) == 0) {
							opt_2.insert(std::make_pair(id, f));
							f = aa;
							break;
						}
					}
				}
				else {
					Printf("encountered nullptr");
				}
			}
		}

		if (f != nullptr) { //Check that we were able to retrieve a member field at all.
			int kkk = 0; //Unused. Will always be 0 until I get more functionality in.
			// TODO: make variable names more descriptive
			int ffff = types[f->type]; //Retrieve the pointer that Torque will internally call to convert the type to a string.
			void* offsetObject = (void*)((((DWORD)this_) + f->offset) + kkk * typeSize[f->type]); //Add an offset to the resulting object, getting the
			//specific fields, then handle it, and return in a JavaScript friendly form.
			if (ffff == 0x48D070) { //Position
				float* aaaa = (float*)offsetObject;
				if (aaaa[15] == 1.0) { //Just ripped from IDA. 
					Handle<Array> ar = Array::New(args.GetIsolate(), 3);
					ar->Set(0, Number::New(args.GetIsolate(), aaaa[3]));
					ar->Set(1, Number::New(args.GetIsolate(), aaaa[7]));
					ar->Set(2, Number::New(args.GetIsolate(), aaaa[11]));
					args.GetReturnValue().Set(ar);
					return;
					//Printf("%g %g %g", aaaa[3], aaaa[7], aaaa[11]);
				}
				else {
					Handle<Array> ar = Array::New(args.GetIsolate(), 4);
					ar->Set(0, Number::New(args.GetIsolate(), aaaa[3]));
					ar->Set(1, Number::New(args.GetIsolate(), aaaa[7]));
					ar->Set(2, Number::New(args.GetIsolate(), aaaa[11]));
					ar->Set(3, Number::New(args.GetIsolate(), aaaa[15]));
					args.GetReturnValue().Set(ar);
					return;
					//Printf("%g %g %g %g", aaaa[3], aaaa[7], aaaa[11], aaaa[15]);
					//Printf("unhandled");
				}
			}
			else if (ffff == 0x04B0530) { //String
				const char** aaaa = (const char**)offsetObject;
				if (aaaa[0]) {
					args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), aaaa[0]));
				}
						//Printf("%s", aaaa[0]);
				return;
			}
			else if (ffff == 0x04B0780) { //Integers
				int* aaaa = (int*)offsetObject;
				//Printf("%d", aaaa[0]);
				args.GetReturnValue().Set(Number::New(args.GetIsolate(), aaaa[0]));
				return;
			}
			else if (ffff == 0x04B0840) { //Boolean
				char* aaaa = (char*)offsetObject;
				if (!*aaaa) {
					args.GetReturnValue().Set(False(args.GetIsolate()));
					//Printf("false");
				}
				else {
					args.GetReturnValue().Set(True(args.GetIsolate()));
					//Printf("true");
				}
				return;
			}
			else if (ffff == 0x048D1A0) { //Rotation shit because idk
				float* aaaa = (float*)offsetObject;

				Printf("%g %g %g %g", aaaa[8], aaaa[9], aaaa[10], aaaa[11] * 180 * 0.3183098861837907);
			}
			else if (ffff == 0x48CC70) { //Type backing obj.scale
				float* aaaa = (float*)offsetObject;
				Handle<Array> ar = Array::New(args.GetIsolate(), 3);
				ar->Set(0, Number::New(args.GetIsolate(), aaaa[0]));
				ar->Set(1, Number::New(args.GetIsolate(), aaaa[1]));
				ar->Set(2, Number::New(args.GetIsolate(), aaaa[2]));
				args.GetReturnValue().Set(ar);
				return;
				//Printf("%g %g %g", aaaa[0], aaaa[1], aaaa[2]);
			}
			else if (ffff == 0x4CBC90) { ///Datablock stuff
				const char* aaaa = *(const char**)(*(DWORD*)offsetObject + 4);
				args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), aaaa));
				return;
				//Printf("%s", aaaa);
			}
			else if (ffff == 0x4B0660) {
				char* aaaa = (char*)offsetObject;
				args.GetReturnValue().Set(Number::New(args.GetIsolate(), *aaaa));
				return;
				//Printf("%d", *aaaa);
			}
			else if (ffff == 0x4B06C0) {
				__int16* aaaa = (__int16*)offsetObject;
				args.GetReturnValue().Set(Number::New(args.GetIsolate(), *aaaa));
				return;
				//Printf("%d", *aaaa);
			}
			else {
				Printf("Unhandled");
				Printf("offsetObject: %p", offsetObject);
				Printf("ffff: %p", ffff);
				Printf("sizeof: %d", typeSize[f->type]);
			}
		}
		//Okay, well, we didn't find a member field corresponding to the value.
		//Maybe it's a function call or just a normal variable?
		if (verbosity > 0) {
			Printf("UNOPTIMIZED LOOKUP %s", field);
		}
		const char* var = SimObject__getDataField(this_, StringTableEntry(field), nullptr); //Call the Torque API for retrieving the field.
		if (_stricmp(var, "") == 0) {
			if (ptr->GetInternalField(1)->ToBoolean(_Isolate)->BooleanValue()) { //Check that the object is registered with Torque, and has a valid ID. 
				Namespace::Entry* entry = passThroughLookup(this_->mNameSpace, field);
				if (entry != nullptr) {
					//It's a function call, wrap the lookup and return it as a function.
					Handle<External> theLookup = External::New(_Isolate, (void*)entry);
					Local<Function> blah = FunctionTemplate::New(_Isolate, js_handleCall, theLookup)->GetFunction();
					args.GetReturnValue().Set(blah);
					return;
				}
				else {
					args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, var)); //Well, we didn't find a entry, so just return the value we got.
				}
			}
		}
		else {
			args.GetReturnValue().Set(String::NewFromUtf8(_Isolate, var)); //The variable wasn't blank, so just return it
		}
	}
	else {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Was unable to get unsafe pointer.."));
	}
	return;
}

void js_setter(Local<String> prop, Local<Value> value, const PropertyCallbackInfo<Value> &args) {
	Handle<Object> ptr = args.This();
	Handle<External> ugh = Handle<External>::Cast(ptr->GetInternalField(0));
	SimObject** SimO = static_cast<SimObject**>(ugh->Value()); //Grab the weak Simobject pointer.
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
	if (verbosity > 0) {
		Printf("== SIMOBJECT GARBAGE COLLECTION ==");
	}
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
	SimObject** safePtr = nullptr;
	SimObject* find = nullptr;
	if (args[0]->IsString()) {
		String::Utf8Value name(args[0]);
		//std::map<const char*, SimObject**>::iterator it;
		//it = opt_4.find(ToCString(name));
		std::unordered_map<const char*, SimObject**>::iterator it = opt_6.find(ToCString(name));
	
		if (it != opt_6.end()) {
			SimObject** safe = it->second;
			if (safe != nullptr) {
				if (*safe != nullptr) {
					find = *safe;
					safePtr = safe;
				}
				else {
					find = Sim__findObject_name(ToCString(name));
				}
			}
			else {
				find = Sim__findObject_name(ToCString(name));
			}
		}
		else {
			find = Sim__findObject_name(ToCString(name));
		}
	}
	else if (args[0]->IsNumber()) {
		std::unordered_map<int, SimObject**>::iterator it;
		it = opt_5.find(args[0]->Int32Value());
		if (it != opt_5.end()) {
			SimObject** safe = it->second;
			if (safe != nullptr) {
				if (*safe != nullptr) {
					find = *safe;
					safePtr = safe;
				}
				else {
					find = Sim__findObject_id(args[0]->Int32Value());
				}
			}
			else {
				find = Sim__findObject_id(args[0]->Int32Value());
			}
		}
		else {
			find = Sim__findObject_id(args[0]->Int32Value());
		}
	}
	if (find == nullptr) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "could not find object"));
		return;
	}
	bool isNull = false;
	if (safePtr == nullptr) {
		safePtr = (SimObject**)js_malloc(sizeof(SimObject*));
		*safePtr = find;
		isNull = true;
		SimObject__registerReference(find, safePtr);
		if (args[0]->IsNumber()) {
			opt_5.insert(std::make_pair(args[0]->Int32Value(), safePtr));
		}
		else {
			String::Utf8Value name(args[0]);
			opt_6.insert(std::make_pair(StringTableEntry(ToCString(name)), safePtr));
			//opt_4.insert(opt_4.end(), std::make_pair((const char*)dest, safePtr));
		}
	}
	Handle<External> ref = External::New(_Isolate, (void*)safePtr);
	Local<Object> fuck = StrongPersistentTL(objectHandlerTemp)->NewInstance();
	fuck->SetInternalField(0, ref);
	fuck->SetInternalField(1, v8::Boolean::New(_Isolate, true));
	Persistent<Object> out;
	out.Reset(_Isolate, fuck);
	if (isNull) {
		out.SetWeak<SimObject*>(safePtr, WeakPtrCallback, v8::WeakCallbackType::kParameter);
	}

	args.GetReturnValue().Set(out);
}

void js_func(const FunctionCallbackInfo<Value> &args) {
	if (args.Length() != 1) {
		_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "Bad arguments passed to ts.func"));
		return;
	}
	if (args[0]->IsString()) {
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
	else if (args[0]->IsObject()) {
		Handle<Object> lookupInfo = args[0]->ToObject();
		Handle<String> nsKey = String::NewFromUtf8(args.GetIsolate(), "class");
		Handle<String> fnKey = String::NewFromUtf8(args.GetIsolate(), "name");
		if (!lookupInfo->Has(nsKey) || !lookupInfo->Has(fnKey)) {
			ThrowError(args.GetIsolate(), "Required key not found.");
			return;
		}

		String::Utf8Value c_ns(lookupInfo->Get(nsKey)->ToString());
		String::Utf8Value c_fn(lookupInfo->Get(fnKey)->ToString());
		Namespace::Entry* entry = fastLookup(ToCString(c_ns), ToCString(c_fn));
		if (entry == nullptr || entry == NULL) {
			_Isolate->ThrowException(String::NewFromUtf8(_Isolate, "TS function not found"));
			return;
		}

		Handle<External> theLookup = External::New(_Isolate, (void*)entry);
		Local<Function> blah = FunctionTemplate::New(_Isolate, js_handleCall, theLookup)->GetFunction();
		args.GetReturnValue().Set(blah);
	}
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
			retval = StringTableEntry(*a, true);
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
	//Revert the ingame console to use TorqueScript.
	Eval("ConsoleEntry.altCommand = \"ConsoleEntry::eval();\";");
}

static void ts_switchToJS(SimObject* this_, int argc, const char* argv[]) {
	//Set the ingame console to use JavaScript instead of TorqueScript.
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
	v8::TryCatch exceptionHandler(iso);
	Handle<Module> modu;
	if (!ScriptCompiler::CompileModule(iso, &aaa).ToLocal(&modu)) {
		Printf("Compiling failed");
		ReportException(iso, &exceptionHandler);
		return MaybeLocal<Module>();
	}
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

void js_version(const FunctionCallbackInfo<Value> &args) {
	/*
		Get the version of misc things internally. 
	*/
	const char* ver = V8::GetVersion();
	Handle<Object> versions = Object::New(args.GetIsolate());
	versions->Set(String::NewFromUtf8(args.GetIsolate(), "v8"), String::NewFromUtf8(args.GetIsolate(), ver));
	versions->Set(String::NewFromUtf8(args.GetIsolate(), "blocklandjs"), String::NewFromUtf8(args.GetIsolate(), BLJS_VERSION));
	versions->Set(String::NewFromUtf8(args.GetIsolate(), "libuv"), String::NewFromUtf8(args.GetIsolate(), uv_version_string()));
	args.GetReturnValue().Set(versions);
}


static const char* ts_js_bridge(SimObject* this_, int argc, const char* argv[]) {
	//This is our callback function registered. 
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
	const char* cRet;
	if (globalMappingTable->Has(identifier)) {
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
	/*
		Expose a JavaScript function to TorqueScript.
		This function takes in two parameters.
		param[0] == Object
			This will be the object containing relevant data. Should have "name" as one of it's keys.
			Can have "class", and "description", for registering on a different namespace, or for setting a description
			that will be visible when you dump the object.

		param[1] == Function
			This will be the Function that will be called when TS calls our callback function. It will look up in the 
			mapping table for the corresponding function, then proceed to call it.
	*/
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

	Handle<Function> passedFunction = Handle<Function>::Cast(args[1]->ToObject());
	
	Handle<Object> globalMappingTable = ContextL()->Global()->Get(String::NewFromUtf8(this_, "__mappingTable__"))->ToObject();

	if (_stricmp(ns1, "ts") == 0) {
		//Is it something that needs to be registered on the global namespace?
		ConsoleFunction(NULL, fnName, ts_js_bridge, StringTableEntry(desc), 1, 23);
	}
	else {
		//Else, register it on some other namespace.
		ConsoleFunction(ns1, fnName, ts_js_bridge, StringTableEntry(desc), 1, 23);
	}
	Handle<String> ide1 = String::Concat(String::NewFromUtf8(this_, ns1), String::NewFromUtf8(this_, "__"));
	Handle<String> ide = String::Concat(ide1, String::NewFromUtf8(this_, fnName));
	globalMappingTable->Set(ide, passedFunction);
	//Store the info that we need for the callback, whenever it eventually comes.
	//Store it in a mapping table.
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
	Handle<Module> mod2;
	if(!getModuleFromSpecifier(iso, spec).ToLocal(&mod2)) { //Grab the Module's source code from the user's input
		resolver->Reject(Exception::Error(String::NewFromUtf8(iso, "Could not find file.")));
		return handle_scope.Escape(resolver.As<Promise>());
	}
	Local<Value> result;
	if (!mod2->InstantiateModule(ContextL(), ModuleResolveCallback).FromMaybe(false)) { //Now instantiate it
		//ReportException(iso, &exceptionHandler);
		resolver->Reject(Exception::Error(String::NewFromUtf8(iso, "Unable to instantiate module")));
		return handle_scope.Escape(resolver.As<Promise>());
	}
	if (!mod2->Evaluate(ContextL()).ToLocal(&result)) { //And evaluate it
		resolver->Reject(Exception::Error(String::NewFromUtf8(iso, "Module evaluation failed")));
		return handle_scope.Escape(resolver.As<Promise>());
	}
	resolver->Resolve(mod2->GetModuleNamespace()); //And then, resolve the Promise with the exports from the module.
	return handle_scope.Escape(resolver.As<Promise>());
}

void idle_cb(uv_idle_t* handle) {
	if (!immediateMode) {
		Sleep(1); //Sleep for 1ms.
	}
}

void RetStuff(Local<Context> b, Local<Module> c, Local<Object> d) {
	/* 
		This is where the meta field of import will come in. I'm hoping to get this done later.
	*/
	Isolate* isolate = b->GetIsolate(); 
}

void sqlite3gc(const v8::WeakCallbackInfo<sqlite3*> &data) {
	sqlite3** safePtr = (sqlite3**)data.GetParameter();
	sqlite3* this_ = *safePtr;
	//This is where Garbage Collection happens.
	if (this_ != nullptr) {
		//If the DB has been initialized at all, be sure to close it.
		sqlite3_close(this_);
	}
	free((void*)safePtr); //Now free the memory we allocated for the wrapping pointer.
}

static int sqlite3Callback(void *wrapper, int argc, char **argv, char **azColName)
{
	sqlite_cb_js* aaa = (sqlite_cb_js*)wrapper; //Grab the info we need to call the callback.
	Isolate* this_ = aaa->this_;
	Locker locker(aaa->this_); //Lock the V8 Isolate to our thread 
	Isolate::Scope iso_scope(this_); 
	this_->Enter(); //Enter it. This is now the thread's current Isolate, and we need to exit after we're done.
	HandleScope handle_scope(this_); //Set a scope for all handles allocated.
	ContextL()->Enter(); //Enter the context.
	v8::Context::Scope cScope(ContextL());
	if (!aaa->cbfn.IsEmpty()) {
		Handle<Value> args[1];
		Handle<Array> results = Array::New(this_); //Create a new Array, which will hold our results
		for (int i = 0; i < argc; i++) {
			results->Set(i, String::NewFromUtf8(this_, argv[i])); //Fill the Array with the results
		}
		args[0] = results;
		StrongPersistentTL(aaa->cbfn)->Call(StrongPersistentTL(aaa->objref), 1, args); //And now call the callback function with the first argument
		//being our results.
	}
	delete aaa; //Free the memory that we allocated for storing some needed things for executing the callback
	ContextL()->Exit(); //Now exit the context
	this_->Exit(); //And now the Isolate
	Unlocker unlocker(this_);  //And unlock it, so other threads can now execute code with V8.
	return 0;
}

void js_sqlite_new(const FunctionCallbackInfo<Value> &args) {
	//Construct a new SQLite DB connector.
	Handle<Object> this_ = StrongPersistentTL(sqliteconstructor)->NewInstance();
	sqlite3* db = nullptr;

	sqlite3** weakPtr = (sqlite3**)uv8_malloc(args.GetIsolate(), sizeof(*weakPtr)); //Create a weak pointer, so we can store nullptr values in it.
	*weakPtr = db;

	this_->SetInternalField(0, External::New(args.GetIsolate(), (void*)weakPtr)); 
	this_->SetInternalField(1, False(args.GetIsolate())); //No DB has been opened, so this should be false.
	Persistent<Object> aaaa;
	aaaa.Reset(args.GetIsolate(), this_); 
	aaaa.SetWeak<sqlite3*>(weakPtr, sqlite3gc, v8::WeakCallbackType::kParameter); //Set a callback for when garbage collections happens to this object.
	args.GetReturnValue().Set(aaaa);
}

sqlite3* getDB(const FunctionCallbackInfo<Value> &args) {
	return *(sqlite3**)Handle<External>::Cast(args.This()->GetInternalField(0))->Value(); //Grab the safe pointer from the internal fields of the object.
}

bool dbOpened(const FunctionCallbackInfo<Value> &args) {
	return args.This()->GetInternalField(1)->BooleanValue();
}

void js_sqlite_open(const FunctionCallbackInfo<Value> &args) {
	//Open a database on a SQLite object.
	ThrowArgsNotVal(1);
	if (!args[0]->IsString()) {
		ThrowBadArg();
	}

	sqlite3* this_ = getDB(args);
	if (dbOpened(args)) { //Is there already one open?
		sqlite3_close(this_); //Close it.
	}
	String::Utf8Value c_db(args[0]->ToString()); 
	const char* db = ToCString(c_db);
	int ret = sqlite3_open(db, (sqlite3**)Handle<External>::Cast(args.This()->GetInternalField(0))->Value()); //Open it, retrieving the pointer from the depths of the object.
	if (ret) {
		ThrowError(args.GetIsolate(), "Unable to open sqlite database");
		return;
	}
	else {
		args.This()->SetInternalField(1, True(args.GetIsolate())); //Set an internal flag, letting us know that there has been a database opened on this object.
	}
}

void js_sqlite_exec(const FunctionCallbackInfo<Value> &args) {
	//Execute a SQLite query on an opened database.
	if (args.Length() < 1) {
		ThrowError(args.GetIsolate(), "Not enough arguments supplied..");
		return;
	}
	if (!args[0]->IsString()) {
		ThrowBadArg();
	}

	if (args.Length() == 2) {
		if (!args[1]->IsFunction()) { //Is this an asynchronous execution?
			ThrowBadArg();
		}
	}

	sqlite3* this_ = getDB(args);
	String::Utf8Value c_query(args[0]->ToString());
	const char* query = ToCString(c_query);
	if (dbOpened(args)) {
		sqlite_cb_js* cbinfo = new sqlite_cb_js;
		if (args.Length() == 2) { //It is an asynchronous call, so setup the reference to the function that should be called.
			cbinfo->cbfn.Reset(args.GetIsolate(), Handle<Function>::Cast(args[1]->ToObject()));
		}
		else {
			cbinfo->cbfn.Reset(args.GetIsolate(), FunctionTemplate::New(args.GetIsolate())->GetFunction());
			//Else, do nothing and just fill it with a blank function.
		}
		cbinfo->objref.Reset(args.GetIsolate(), args.This()); //Get the 'this' variable, and store it.
		cbinfo->this_ = args.GetIsolate(); //And store the current Isolate that we're in.
		char* err = 0; 
		int ret = sqlite3_exec(this_, query, sqlite3Callback, cbinfo, &err); //Now, execute the query!
		if (ret != SQLITE_OK) {
			Printf("Error: %s", err);
			ThrowError(args.GetIsolate(), "SQLite error encountered.");
			sqlite3_free(err);
			delete cbinfo;
			return;
		}
		//The callback will be called with the result in an Array, shortly after.
	}
	else {
		ThrowError(args.GetIsolate(), "DB not opened");
		return;
	}
}
void js_sqlite_close(const FunctionCallbackInfo<Value> &args) {
	//Close an opened SQLite database
	sqlite3* this_ = getDB(args); //Grab the database from the first object passed to this function.
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
	//Modify the verbosity of functions. Used in debugging..
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
	//Disable sleeping for 1 millisecond on the event loop. 
	//You shouldn't have to do this ever.
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

	//Ensure that we were able to find all of the functions needed.
	if (!torque_init())
	{
		return false;
	}
	 
	Printf("BlocklandJS || Init");
	Printf("BlocklandJS: version %s", V8::GetVersion());
	//Startup the libuv loop here
	uv_loop_init(uv_default_loop());
	_Platform = platform::CreateDefaultPlatform(); //Initialize the Platform backing V8.
	V8::InitializePlatform(_Platform);
	const char* v8flags = "--harmony --harmony-dynamic-import"; //Enable some experimental features from V8.
	/*
	size_t sz = _MAX_PATH * 2;
	char path[_MAX_PATH * 2];
	uv_cwd(path, &sz); 
	V8::InitializeICUDefaultLocation(path); We don't support anything other then ASCII and UTF-8, so we don't need this.
	*/
	V8::SetFlagsFromString(v8flags, strlen(v8flags));
	V8::Initialize();

	Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
	_Isolate = Isolate::New(create_params);
	_Isolate->Enter();

	_Isolate->SetData(0, (void*)uv_default_loop());

	HandleScope scope(_Isolate);

	/* IGNORE */
	//PatchByte((BYTE*)0x4B45B0, 0xEB); //Bypass the entire fucking thing
	//PatchByte((BYTE*)0x4B45B1, 0x38); //Initiate an accelerated backhop to the driveway behind you..
	//PatchByte((BYTE*)0x4B45B2, 0x90); //Also known as jump to the return so we don't crash
	//PatchByte((BYTE*)0x04B45B3, 0x90); //Now, you might be asking yourself.
	//PatchByte((BYTE*)0x04B45B4, 0x90); //"Metario, why are you patching bytes"?
	//PatchByte((BYTE*)0x04B45B5, 0x90); //Well, fucker, it's because somehow in really rare occasions, 
	//PatchByte((BYTE*)0x04B45B6, 0x90); //a xor ecx, ecx register is called before a mov operation.
	//PatchByte((BYTE*)0x04B45B7, 0x90); //This leads to a nullptr exception, since the ecx register is used as the base.
	//PatchByte((BYTE*)0x4B45D4, 0x90); //SO, these byte changes are meant to nop out the instruction that jumps,
	//PatchByte((BYTE*)0x4B45D5, 0x90); //And the instruction that causes the null pointer exception.


	/* global */
	//Setup the Global object in JavaScript. If anything is to be registered, it should be done here for the global.
	Local<ObjectTemplate> global = ObjectTemplate::New(_Isolate);
	global->Set(_Isolate, "print", FunctionTemplate::New(_Isolate, js_print));
	global->Set(_Isolate, "load", FunctionTemplate::New(_Isolate, Load));
	global->Set(_Isolate, "version", FunctionTemplate::New(_Isolate, js_version));
	global->Set(_Isolate, "__mappingTable__", ObjectTemplate::New(_Isolate));
	global->Set(_Isolate, "immediateMode", FunctionTemplate::New(_Isolate, js_setImmediateMode));
	global->Set(_Isolate, "__verbosity__", FunctionTemplate::New(_Isolate, js_modifyVerbosity));


	//Setup, and register the sqlite implementation in JS.
	Local<FunctionTemplate> sqlite = FunctionTemplate::New(_Isolate, js_sqlite_new);
	sqlite->SetClassName(String::NewFromUtf8(_Isolate, "sqlite"));
	sqlite->InstanceTemplate()->Set(_Isolate, "open", FunctionTemplate::New(_Isolate, js_sqlite_open));
	sqlite->InstanceTemplate()->Set(_Isolate, "close", FunctionTemplate::New(_Isolate, js_sqlite_close));
	sqlite->InstanceTemplate()->Set(_Isolate, "exec", FunctionTemplate::New(_Isolate, js_sqlite_exec));
	sqlite->InstanceTemplate()->SetInternalFieldCount(2);
	global->Set(_Isolate, "sqlite", sqlite);
	sqliteconstructor.Reset(_Isolate, sqlite->InstanceTemplate()); //Reset the Persistent constructor with the template.

	/* console */
	//Setup the console object for ease-of-use.
	Local<ObjectTemplate> console = ObjectTemplate::New(_Isolate);
	console->Set(_Isolate, "log", FunctionTemplate::New(_Isolate, js_console_log));
	console->Set(_Isolate, "warn", FunctionTemplate::New(_Isolate, js_console_warn));
	console->Set(_Isolate, "error", FunctionTemplate::New(_Isolate, js_console_error));
	console->Set(_Isolate, "info", FunctionTemplate::New(_Isolate, js_console_info));

	/* ts */
	//Setup the TorqueScript bridge. If anything is to be registered to ts.*, do it here.
	Local<ObjectTemplate> globalTS = ObjectTemplate::New(_Isolate);
	globalTS->Set(_Isolate, "setVariable". FunctionTemplate::New(_Isolate, js_setGlobalVar));
	globalTS->Set(_Isolate, "getVariable", FunctionTemplate::New(_Isolate, js_getGlobalVar));
	globalTS->Set(_Isolate, "linkClass", FunctionTemplate::New(_Isolate, js_linkClass));
	globalTS->Set(_Isolate, "registerObject", FunctionTemplate::New(_Isolate, js_registerObject));
	globalTS->Set(_Isolate, "func", FunctionTemplate::New(_Isolate, js_func));
	globalTS->Set(_Isolate, "obj", FunctionTemplate::New(_Isolate, js_obj));
	globalTS->Set(_Isolate, "switchToTS", FunctionTemplate::New(_Isolate, js_switchToTS));
	globalTS->Set(_Isolate, "expose", FunctionTemplate::New(_Isolate, js_expose));

	/* ts.simSet */
	//Register our quick SimSet functions here.
	Local<ObjectTemplate> SimSet = ObjectTemplate::New(_Isolate);
	SimSet->Set(String::NewFromUtf8(_Isolate, "getObject", NewStringType::kNormal).ToLocalChecked(), FunctionTemplate::New(_Isolate, js_SimSet_getObject));
	SimSet->Set(_Isolate, "getCount", FunctionTemplate::New(_Isolate, js_SimSet_getCount));
	globalTS->Set(_Isolate, "SimSet", SimSet);

	//Register some things necessary for ES6 modules to work here.
	_Isolate->SetHostImportModuleDynamicallyCallback(ImportModuleDynam);
	_Isolate->SetHostInitializeImportMetaObjectCallback(RetStuff);


	//Bind the LibUV bindings.
	uv8_bind_all(_Isolate, global);

	//And now register 'ts' and 'console' on the global namespace so they can be called.
	global->Set(_Isolate, "ts", globalTS);
	global->Set(_Isolate, "console", console);

	//Setup an template for all objects that are internally TorqueScript objects
	Handle<ObjectTemplate> thisTemplate = ObjectTemplate::New(_Isolate);
	thisTemplate->SetInternalFieldCount(2);
	thisTemplate->SetNamedPropertyHandler(js_getter, js_setter);
	objectHandlerTemp.Reset(_Isolate, thisTemplate);

	//Create the context, with the given global.
	Local<v8::Context> context = Context::New(_Isolate, NULL, global);
	//Now reset the persistent reference to the context, so we don't have to reconstruct it.
	_Context.Reset(_Isolate, context);

	//We're done here, exit the isolate so it can be opened by other threads.
	_Isolate->Exit();

	//Register our TorqueScript API functions after JS has finished initializing.
	ConsoleFunction(NULL, "js_eval", ts_js_eval, "(string script) - Evaluate a script in the JavaScript engine.", 2, 2);
	ConsoleFunction(NULL, "js_exec", ts_js_exec, "(string filename) - Execute a script in the JavaScript engine.", 2, 2);
	ConsoleFunction(NULL, "switchToJS", ts_switchToJS, "() - Switch your console to use JavaScript.", 1, 1);

	//Setup an idle hook to ensure that LibUV's event loop continues running, and to reduce CPU usage resulting from it.
	//All this does is sleep for 1 millisecond, unless immediateMode is set to true.
	idle = (uv_idle_t*)malloc(sizeof(*idle));
	uv_idle_init(uv_default_loop(), idle);
	uv_idle_start(idle, idle_cb);

	//Then, create a seperate thread for LibUV to run, so the initialization thread of this doesn't get hung.
	uv_thread_create(&thread, Watcher_run, (void*)running);
	//And, finally, we're done.
	Printf("BlocklandJS || Attached");
	return true;
}

bool deinit() {
	//Unreference the idle hook so the LibUV event loop can spin down.
	uv_unref((uv_handle_t*)idle);
	//Then close the handle, because we're done here.
	uv_loop_close(uv_default_loop());
	//Dispose of the wrapping Isolate
	_Isolate->Dispose();
	//Then shut down V8.
	V8::Dispose();
	V8::ShutdownPlatform();
	delete _Platform;
	//And we're done.
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
