#define WIN32_LEAN_AND_MEAN
#pragma once
#include "uv8.h"
#include "Torque.h"
#include "include\v8.h"
#include <map>
#include <string>
#include <unordered_map>
#include "tvector.h"

//Internal variables Torque uses to determine the function to call when converting a ConsoleBaseType to a cstring.
static int* typeSize = (int*)0x752F78;
static int* types = (int*)0x7557A0;

static std::unordered_map<std::string, Field*> Member__field_cache;
static std::unordered_map<std::string, void*> Class__rep_cache; //used to retreive the class representation

static std::unordered_map<int, SimObject**> SimObject__id_cache;

static std::unordered_map<std::string, SimObject**> SimObject__name_cache;  //caching for ts.obj, used to avoid doing more lookups to ts then necessary.s

Persistent<ObjectTemplate> objectHandlerTemp;

void WeakPtrCallback(const v8::WeakCallbackInfo<SimObject*> &data) {
	/*
		Used for garbage collection. We need to ensure that we unregister the reference
		with Torque, or else undefined behavior may happen.
	*/
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
		std::unordered_map<std::string, void*>::iterator it2;
		std::string mName(this_->mNameSpace->mName);
		it2 = Class__rep_cache.find(mName);
		if (it2 != Class__rep_cache.end()) {
			classRep = it2->second;
		}
		else {
			classRep = (*(void*(**)())((DWORD)aaa - 4))();
			Class__rep_cache.insert(std::make_pair(mName, classRep));
		}
		std::unordered_map<std::string, Field*>::iterator it;
		Vector<Field> &fuck = *(Vector<Field>*)((DWORD)classRep + 0x2C); //Retrieve the backing mFieldList from the class representation.
		if (verbosity > 0) {
			Printf("%p", SimObject__getDataField);
			Printf("%p", ((DWORD)classRep + 0x2C));
			Printf("%p", classRep);
			Printf("%d", fuck.size());
			Printf("%p", fuck.address());
			Printf("%d", sizeof(Field));
		}
		Field* f = nullptr;
		std::string id(this_->mNameSpace->mName);
		id.append(field);
		it = Member__field_cache.find(id);
		if (it != Member__field_cache.end()) {
			f = it->second;
		}
		else {
			for (Vector<Field>::iterator itr = fuck.begin(); itr != fuck.end(); itr++) {
				Field* aa = itr;
				if (aa != nullptr) {
					if (aa->pGroupname != nullptr) {
						//Printf(" == GROUP: %s ==", f->pGroupname);
					}
					else {
						if (_stricmp(aa->pFieldname, field) == 0) {
							Member__field_cache.insert(std::make_pair(id, aa));
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
		std::string obj(ToCString(name));
		std::unordered_map<std::string, SimObject**>::iterator it = SimObject__name_cache.find(obj);

		if (it != SimObject__name_cache.end()) {
			SimObject** safe = it->second;
			if (safe != nullptr) {
				if (*safe != nullptr) {
					find = *safe;
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
		it = SimObject__id_cache.find(args[0]->Int32Value());
		if (it != SimObject__id_cache.end()) {
			SimObject** safe = it->second;
			if (safe != nullptr) {
				if (*safe != nullptr) {
					find = *safe;
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
			SimObject__id_cache.insert(std::make_pair(args[0]->Int32Value(), safePtr));
		}
		else {
			String::Utf8Value name(args[0]);
			std::string sname(ToCString(name));
			SimObject__name_cache.insert(std::make_pair(sname, safePtr));
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

void js_switchToTS(const FunctionCallbackInfo<Value> &args) {
	//Revert the ingame console to use TorqueScript.
	Eval("ConsoleEntry.altCommand = \"ConsoleEntry::eval();\";");
}

void js_printSize(const FunctionCallbackInfo<Value> &args) {
	Printf("Member field cache: %d", Member__field_cache.size());
	Printf("Class representation cache: %d", Class__rep_cache.size());
	Printf("SimObject id cache: %d", SimObject__id_cache.size());
	Printf("SimObject name cache: %d", SimObject__name_cache.size());
}

static void ts_switchToJS(SimObject* this_, int argc, const char* argv[]) {
	//Set the ingame console to use JavaScript instead of TorqueScript.
	Eval("function ConsoleEntry::jsEval(){%text = ConsoleEntry.getValue();ConsoleEntry.setText(\"\");echo(\"==>\" @ %text);js_eval(%text);}ConsoleEntry.altCommand = \"ConsoleEntry::jsEval(); \";");
}

void ts_bridge_init(Isolate* this_, Local<ObjectTemplate> global) {
	Local<ObjectTemplate> globalTS = ObjectTemplate::New(this_);
	globalTS->Set(this_, "setVariable", FunctionTemplate::New(this_, js_setGlobalVar));
	globalTS->Set(this_, "getVariable", FunctionTemplate::New(this_, js_getGlobalVar));
	globalTS->Set(this_, "linkClass", FunctionTemplate::New(this_, js_linkClass));
	globalTS->Set(this_, "registerObject", FunctionTemplate::New(this_, js_registerObject));
	globalTS->Set(this_, "func", FunctionTemplate::New(this_, js_func));
	globalTS->Set(this_, "obj", FunctionTemplate::New(this_, js_obj));
	globalTS->Set(this_, "switchToTS", FunctionTemplate::New(this_, js_switchToTS));
	globalTS->Set(this_, "expose", FunctionTemplate::New(this_, js_expose));

	/* ts.simSet */
	//Register our quick SimSet functions here.
	Local<ObjectTemplate> SimSet = ObjectTemplate::New(this_);
	SimSet->Set(this_, "getObject", FunctionTemplate::New(this_, js_SimSet_getObject));
	SimSet->Set(this_, "getCount", FunctionTemplate::New(this_, js_SimSet_getCount));
	globalTS->Set(this_, "SimSet", SimSet);

	global->Set(this_, "ts", globalTS);
	global->Set(this_, "printSize", FunctionTemplate::New(this_, js_printSize));

	Handle<ObjectTemplate> thisTemplate = ObjectTemplate::New(this_);
	thisTemplate->SetInternalFieldCount(2);
	thisTemplate->SetNamedPropertyHandler(js_getter, js_setter);
	objectHandlerTemp.Reset(this_, thisTemplate);

	ConsoleFunction(NULL, "switchToJS", ts_switchToJS, "() - Switch your console to use JavaScript.", 1, 1);
}
