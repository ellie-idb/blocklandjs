#include <jsapi.h>
#include <stdlib.h>
#include <direct.h>
#include <process.h>
#include <string.h>
#include <js/Initialization.h>
#include <js/Proxy.h>
#include "torque.h"
#include <string>
#include <vector>
#include <js/Conversions.h>


static std::map<int, SimObject**> garbagec_ids;
static std::map<int, SimObject**> garbagec_objs; //Saving these because we have to.
bool js_getObjectVariable(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::Value* vp);

enum JSShellErrNum {
#define MSG_DEF(name, count, exception, format) \
    name,
#include <jsshell.msg>
#undef MSG_DEF
	JSShellErr_Limit
};

static JSClassOps global_ops = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    JS_GlobalObjectTraceHook
};

static JSClass global_class = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    &global_ops
};

static JSClassOps ts_ops {
	nullptr, //Add prop
	nullptr, //Del prop
	nullptr, //Get prop
	nullptr, //
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr
};

static JSClass ts_obj = {
	"ts_obj",
	JSCLASS_HAS_PRIVATE,
	&ts_ops
};

JSContext *_Context;
JS::RootedObject *_Global;
JS::RootedFunction* getter;
JS::RootedFunction* setter;
JS::RootedObject* handler;
JS::RootedObject* getObj;
JS::RootedObject* setObj;
JS::RootedObject* ireallydo;
JS::RootedObject* proxyStuff;
JS::RootedValue* proxyConstructor;

static const JSErrorFormatString jsShell_ErrorFormatString[JSShellErr_Limit] = {
#define MSG_DEF(name, count, exception, format) \
    { #name, format, count, JSEXN_ERR } ,
#include <jsshell.msg>
#undef MSG_DEF
};

const JSErrorFormatString* my_GetErrorMessage(void* userRef, const unsigned errorNumber)
{
	if (errorNumber == 0 || errorNumber >= JSShellErr_Limit)
		return nullptr;

	return &jsShell_ErrorFormatString[errorNumber];
}

SimObject** getPointer(JSContext* cx, JS::MutableHandleObject eck) {
	JS::RootedValue ptr(cx);
	if (JS_GetProperty(cx, eck, "__ptr__", &ptr)) {
		return (SimObject**)ptr.toPrivate();
	}
	return nullptr;
}

void js_finalizeCb(JSContext* op, JSGCStatus status, void* data) {
	Printf("GC free called");
	SimObject** safePtr = (SimObject**)data;
	if (safePtr != NULL && safePtr != nullptr) {
		SimObject* this_ = *safePtr;
		if (this_ != NULL && this_ != nullptr) {
			Printf("Freeing %d", this_->id);
			SimObject__unregisterReference(this_, safePtr);
		}
	}
}

int SimSet__getCount(DWORD set)
{
	return *(DWORD*)(set + 0x34);
}


SimObject* SimSet__getObject(DWORD set, int index)
{
	DWORD ptr1 = *(DWORD*)(set + 0x3C);
	SimObject* ptr2 = (SimObject*)*(DWORD*)(ptr1 + 0x4 * index);
	return ptr2;
}


bool SS_gO(JSContext* cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	if (a.length() != 2) {
		Printf("SimSet_getObject called with wrong number of args");
		return false;
	}
	if (!a[0].isObject() || !a[1].isInt32() || !a[1].isNumber()) {
		Printf("SimSet_getObject called with wrong type of args");
		return false;
	}
	JS::RootedObject* fuck = new JS::RootedObject(cx, &a[0].toObject());
		SimObject** gay = getPointer(cx, fuck);
		if(gay == nullptr || gay == NULL) {
			return false;
		}
		SimObject* simSet = *gay;
		SimObject* ret = SimSet__getObject((DWORD)simSet, a[1].toNumber());
		SimObject** seriously = (SimObject**)JS_malloc(cx, sizeof(SimObject*));
		*seriously = ret;
		SimObject__registerReference(ret, seriously);
		JSObject* out = JS_NewObject(cx, &ts_obj);
		JS_SetPrivate(out, (void*)seriously);
		JS::RootedObject* sudo = new JS::RootedObject(cx, out);
		JS::RootedValue* val = new JS::RootedValue(cx, JS::PrivateValue((void*)seriously));
		JS_DefineProperty(cx, *sudo, "__ptr__", *val, 0);
		JS::AutoValueArray<2> args(cx);
		args[0].setObject(*out);
		args[1].setObject(**handler);
		JS::RootedObject proxiedObject(cx);
		if (!JS::Construct(cx, *proxyConstructor, JS::HandleValueArray(args), &proxiedObject)) {
			Printf("Failed to construct");
			return false;
		}
		a.rval().setObject(*proxiedObject);
		return true;
}




void getPrintable(JSContext* cx, JS::MutableHandleValue val, char* in, size_t size) {
	if (val.isNull()) {
		strcat_s(in, size, "(null)");
	}
	else if (val.isUndefined()) {
		strcat_s(in, size, "(undefined)");
	}
	else if (val.isBoolean()) {
		strcat_s(in, size, val.toBoolean() ? "true" : "false");
	}
	else if (val.isInt32()) {
		sprintf_s(in, size, "%s%d", in, val.toInt32());
	}
	else if (val.isNumber()) {
		sprintf_s(in, size, "%s%u", in, val.toNumber());
	}
	else if (val.isString()) {
		strcat_s(in, size, JS_EncodeString(cx, val.toString()));
	}
	else if (val.isObject()) {
		strcat_s(in, size, "(object)");
	}
	else if (val.isSymbol()) {
		strcat_s(in, size, "(symbol)");
	}
	else {
		strcat_s(in, size, "(unhandled)");
	}
}


bool js_print(JSContext* cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	if (a.length() == 0) {
		return false;
	}
	char buf[1024] = "";
	for (int i = 0; i < a.length(); i++) {
		if (i > 0) {
			strcat_s(buf, " ");
		}
		getPrintable(cx, a[i], buf, 1024);
	}
	Printf("%s", buf);
	return true;
}

/* SLOW SLOW SLOW */
bool ts_isMethod(Namespace* nm, const char* methodName)
{
	for (; nm; nm = nm->mParent)
	{
		for (auto walk = nm->mEntryList; walk; walk = walk->mNext)
		{
			//does this even work
			auto funcName = walk->mFunctionName;
			auto etype = walk->mType;
			if (etype >= Namespace::Entry::ScriptFunctionType || etype == Namespace::Entry::OverloadMarker)
			{
				if (etype == Namespace::Entry::OverloadMarker)
				{
					etype = 8;
					for (Namespace::Entry* eseek = nm->mEntryList; eseek; eseek = eseek->mNext)
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
				if (_stricmp(funcName, methodName) == 0) {
					return true;
				}
			}
		}
	}
	return false;
}

bool js_plainCall(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	if (a.length() > 1 || !a[0].isString()) {
		return false;
	}

	JSString* fnName = a[0].toString();
	const char* passC = JS_EncodeString(cx, fnName);
	JS::RootedObject eck(cx);
	SimObject** eh = nullptr;
	SimObject* aaaa = nullptr;
	bool gotObject = false;
	if (a.thisv().isObject()) {
		gotObject = JS_ValueToObject(cx, a.thisv(), &eck);
		if (gotObject) {
			JS::RootedValue ptr(cx);
			//JS::Value fnName = JS_GetReservedSlot(&qq.toObject(), 1);
			//JS::Value ptr = JS_GetReservedSlot(&qq.toObject(), 0);
			if (!a.thisv().isNullOrUndefined()) {
				eh = getPointer(cx, &eck);
				if (eh != nullptr) {
					aaaa = *eh;
				}
			}
		}
	}
	Namespace::Entry* us;
	int argcc = 0;
	const char* argv[21];
	argv[argcc++] = passC;
	std::string ayy;
	if (eh == nullptr || aaaa == nullptr || !gotObject) {
		//Printf("Invalid (this) reference.");
		us = fastLookup("", passC);
		//return false;
	}
	//Printf("%s::%s", aaaa->mNameSpace->mName, JS_EncodeString(cx, obj));
	else
	{
		us = fastLookup(aaaa->mNameSpace->mName, passC);
		ayy = std::to_string(aaaa->id);
		argv[argcc++] = ayy.c_str();
	}
	if (us == NULL || us == nullptr) {
		Printf("lookup failure");
		return false;
	}
	for (int i = 1; i < a.length(); i++) {
		if (a[i].isObject()) {
			JS::RootedObject* possibleObject = new JS::RootedObject(cx, &a[i].toObject());
			void* magic = JS_GetInstancePrivate(cx, *possibleObject, &ts_obj, NULL);
			if (magic == NULL || magic == nullptr) {
				Printf("Passed a non-magical object.");
			}
			else {
				SimObject* obj = *(SimObject**)magic;
				ayy = std::to_string(obj->id);
				argv[argcc++] = ayy.c_str();
			}
		}
		else if (a[i].isNumber()) {
			ayy = std::to_string(a[i].toInt32());
			argv[argcc++] = ayy.c_str();
		}
		else if (a[i].isInt32()) {
			ayy = std::to_string(a[i].toNumber());
			argv[argcc++] = ayy.c_str();
		}
		else if (a[i].isBoolean()) {
			argv[argcc++] = a[i].toBoolean() ? "true" : "false";
		}
		else if (a[i].isString()) {
			argv[argcc++] = JS_EncodeString(cx, a[i].toString());
		}
		else {
			Printf("could not pass arg %d to ts", i);
		}
	}
	if (us->mType == Namespace::Entry::ScriptFunctionType)
	{
		if (us->mFunctionOffset)
		{
			a.rval().setString(JS_NewStringCopyZ(cx, CodeBlock__exec(
				us->mCode, us->mFunctionOffset,
				us->mNamespace, us->mFunctionName, argcc, argv,
				false, us->mPackage, 0)));
			return true;
		}
		else
			return false;
	}
	S32 mMinArgs = us->mMinArgs;
	S32 mMaxArgs = us->mMaxArgs;
	if ((mMinArgs && argcc < mMinArgs) || (mMaxArgs && argcc > mMaxArgs))
	{
		Printf("Expected args between %d and %d, got %d", mMinArgs, mMaxArgs, argcc);
		a.rval().setBoolean(false);
		//duk_pop(ctx);
		return true;
	}
	switch (us->mType) {
	case Namespace::Entry::StringCallbackType:
		a.rval().setString(JS_NewStringCopyZ(cx, us->cb.mStringCallbackFunc(aaaa, argcc, argv)));
		return true;
	case Namespace::Entry::IntCallbackType:
		a.rval().setInt32(us->cb.mIntCallbackFunc(aaaa, argcc, argv));
		return true;
	case Namespace::Entry::FloatCallbackType:
		a.rval().setNumber(us->cb.mFloatCallbackFunc(aaaa, argcc, argv));
		return true;
	case Namespace::Entry::BoolCallbackType:
		a.rval().setBoolean(us->cb.mBoolCallbackFunc(aaaa, argcc, argv));
		return true;
	case Namespace::Entry::VoidCallbackType:
		us->cb.mVoidCallbackFunc(aaaa, argcc, argv);
		return false;
	}
	return false;
}

bool js_doTheCall(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	JSFunction* idk = JS_ValueToFunction(cx, a.calleev());
	JSString* obj = JS_GetFunctionId(idk);
	JS::RootedObject eck(cx);
	SimObject** eh = nullptr;
	SimObject* aaaa = nullptr;
	if (JS_ValueToObject(cx, a.thisv(), &eck)) {
		JS::RootedValue ptr(cx);
		//JS::Value fnName = JS_GetReservedSlot(&qq.toObject(), 1);
		//JS::Value ptr = JS_GetReservedSlot(&qq.toObject(), 0);
		if (!a.thisv().isNullOrUndefined()) {
			eh = getPointer(cx, &eck);
			if (eh != nullptr) {
				aaaa = *eh;
			}
		}
	}
	Namespace::Entry* us;
	int argcc = 0;
	const char* argv[21];
	argv[argcc++] = JS_EncodeString(cx, obj);
	std::string ayy;
	if (eh == NULL || eh == nullptr || aaaa == NULL || aaaa == nullptr) {
		//Printf("Invalid (this) reference.");
		us = fastLookup("", JS_EncodeString(cx, obj));
		//return false;
	}
	//Printf("%s::%s", aaaa->mNameSpace->mName, JS_EncodeString(cx, obj));
	else
	{
		us = fastLookup(aaaa->mNameSpace->mName, JS_EncodeString(cx, obj));
		ayy = std::to_string(aaaa->id);
		argv[argcc++] = ayy.c_str();
	}
	if (us == NULL || us == nullptr) {
		Printf("lookup failure");
		return false;
	}
	for (int i = 0; i < a.length(); i++) {
		if (a[i].isObject()) {
			JS::RootedObject* possibleObject = new JS::RootedObject(cx, &a[i].toObject());
			void* magic = JS_GetInstancePrivate(cx, *possibleObject, &ts_obj, NULL);
			if (magic == NULL || magic == nullptr) {
				Printf("Passed a non-magical object.");
			}
			else {
				SimObject* obj = *(SimObject**)magic;
				ayy = std::to_string(obj->id);
				argv[argcc++] = ayy.c_str();
			}
		}
		else if (a[i].isNumber()) {
			ayy = std::to_string(a[i].toInt32());
			argv[argcc++] = ayy.c_str();
		}
		else if (a[i].isInt32()) {
			ayy = std::to_string(a[i].toNumber());
			argv[argcc++] = ayy.c_str();
		}
		else if (a[i].isBoolean()) {
			argv[argcc++] = a[i].toBoolean() ? "true" : "false";
		}
		else if (a[i].isString()) {
			argv[argcc++] = JS_EncodeString(cx, a[i].toString());
		}
		else {
			Printf("\x03could not pass arg %d to ts", i);
		}
	}
	if (us->mType == Namespace::Entry::ScriptFunctionType)
	{
		if (us->mFunctionOffset)
		{
			a.rval().setString(JS_NewStringCopyZ(cx, CodeBlock__exec(
				us->mCode, us->mFunctionOffset,
				us->mNamespace, us->mFunctionName, argcc, argv,
				false, us->mPackage, 0)));
			return true;
		}
		else
			return false;
	}
	S32 mMinArgs = us->mMinArgs;
	S32 mMaxArgs = us->mMaxArgs;
	if ((mMinArgs && argcc < mMinArgs) || (mMaxArgs && argcc > mMaxArgs))
	{
		Printf("\x03Expected args between %d and %d, got %d", mMinArgs, mMaxArgs, argcc);
		a.rval().setBoolean(false);
		//duk_pop(ctx);
		return true;
	}
	switch (us->mType) {
	case Namespace::Entry::StringCallbackType:
		a.rval().setString(JS_NewStringCopyZ(cx, us->cb.mStringCallbackFunc(aaaa, argcc, argv)));
		return true;
	case Namespace::Entry::IntCallbackType:
		a.rval().setInt32(us->cb.mIntCallbackFunc(aaaa, argcc, argv));
		return true;
	case Namespace::Entry::FloatCallbackType:
		a.rval().setNumber(us->cb.mFloatCallbackFunc(aaaa, argcc, argv));
		return true;
	case Namespace::Entry::BoolCallbackType:
		a.rval().setBoolean(us->cb.mBoolCallbackFunc(aaaa, argcc, argv));
		return true;
	case Namespace::Entry::VoidCallbackType:
		us->cb.mVoidCallbackFunc(aaaa, argcc, argv);
		return false;
	}
	return false;
}

bool js_getObjectVariable(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	//Printf("Called");
	JS::RootedObject* fuck = new JS::RootedObject(cx, &a[0].toObject());
	JSString* val = a[1].toString();
	int32_t res;
	void* magic = JS_GetInstancePrivate(cx, *fuck, &ts_obj, NULL);
	//JS_GetProperty(cx, *fuck, "__ptr__", &ptr);
	
	JS::RootedValue ptr(cx, JS::PrivateValue(magic));
	JS_CompareStrings(cx, val, JS_NewStringCopyZ(cx, "__ptr__"), &res);
	if (res == 0) {
			a.rval().set(ptr);
			return true;
	}
	else {
		SimObject** safePtr = (SimObject**)ptr.toPrivate();
		if (safePtr == NULL || safePtr == nullptr) {
			return false;
		}
		SimObject* passablePtr = *safePtr;
		if (passablePtr == NULL || passablePtr == nullptr) {
			return false;
		}
		if (fastLookup(passablePtr->mNameSpace->mName, JS_EncodeString(cx, val)) != nullptr) {
			//JS::AutoValueArray<3> args(cx);
			//args[0].setString(JS_NewStringCopyZ(cx, passablePtr->mNameSpace->mName));
			//args[1].setString(val);
			//args[2].setPrivate(passablePtr);
			JSFunction* func = JS_NewFunction(cx, js_doTheCall, 0, 0, JS_EncodeString(cx, val));
			JSObject* whuh = JS_GetFunctionObject(func);
			//JS_SetReservedSlot(whuh, 0, args[1]);
			//JS_SetReservedSlot(whuh, 1, args[2]);
			a.rval().setObject(*whuh);
		}
		else {
			const char* ret = SimObject__getDataField(passablePtr, JS_EncodeString(cx, val), StringTableEntry(""));
			JSString* jsRet = JS_NewStringCopyZ(cx, ret);
			a.rval().setString(jsRet);
		}
		return true;
	}
	//void* eek = JS_GetInstancePrivate(cx, *fuck, &ts_obj, NULL);
	//if (eek != NULL && eek != nullptr) {
	//}
}


bool ts_func(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	JSString* val = a[0].toString();
	JSFunction* func = JS_NewFunction(cx, js_doTheCall, 0, 0, JS_EncodeString(cx, val));
	JSObject* whuh = JS_GetFunctionObject(func);
	//JS_SetReservedSlot(whuh, 0, args[1]);
	//JS_SetReservedSlot(whuh, 1, args[2]);
	a.rval().setObject(*whuh);
	return true;
}


bool js_setObjectVariable(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	//Printf("Called");
	JS::RootedObject* fuck = new JS::RootedObject(cx, &a[0].toObject());
	SimObject** safePtr = getPointer(cx, fuck);
	if (safePtr != NULL && safePtr != nullptr) {
		SimObject* this_ = *safePtr;
		if (this_ != NULL && this_ != nullptr) {
			char buf[1024] = "";
			getPrintable(cx, a[2], buf, 1024);
			SimObject__setDataField(this_, JS_EncodeString(cx, a[1].toString()), StringTableEntry(""), StringTableEntry(buf));
			a.rval().setBoolean(true);
			return true;
		}
		a.rval().setBoolean(false);
		return true;
	}
	a.rval().setBoolean(false);
	return true;
}

bool js_setGlobalVariable(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	if (a.length() != 2) {
		a.rval().setBoolean(false);
		return false;
	}
	if (!a[0].isString() | !a[1].isString()) {
		a.rval().setBoolean(false);
		return false;
	}
	JSString* key = a[0].toString();
	JSString* val = a[1].toString();
	SetGlobalVariable(JS_EncodeString(cx, key), JS_EncodeString(cx, val));
	a.rval().setBoolean(true);
	return true;
}

bool js_getGlobalVariable(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	if (a.length() != 1 || !a[0].isString()) {
		a.rval().setBoolean(false);
		return false;
	}
	JSString* key = a[0].toString();
	const char* val = GetGlobalVariable(JS_EncodeString(cx, key));
	JSString* vala = JS_NewStringCopyZ(cx, val);
	a.rval().setString(vala);
	return true;
}

bool js_ts_const(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	//if (a.length() != 1 || !a[0].isString()) {
	//	a.rval().setBoolean(false);
	//	return false;
	//}
	JSFunction* idk = JS_ValueToFunction(cx, a.calleev());
	JSString* obj = JS_GetFunctionId(idk);
	//JSString* obj = a[0].toString();
	const char* type = JS_EncodeString(cx, obj);
	SimObject* simobj = (SimObject*)AbstractClassRep_create_className(type);
	if (simobj == NULL) {
		a.rval().setBoolean(false);
		return false;
	}
	SimObject** safePtr = (SimObject**)JS_malloc(cx, sizeof(SimObject*));
	*safePtr = simobj;
	SimObject__registerReference(simobj, safePtr);
	//JS_SetGCCallback(cx, js_finalizeCb, (void*)safePtr);
	simobj->mFlags |= SimObject::ModStaticFields;
	simobj->mFlags |= SimObject::ModDynamicFields;
	std::map<int, SimObject**>::iterator it;
	it = garbagec_objs.find(simobj->id);
	if (it == garbagec_objs.end())
	{
		garbagec_objs.insert(garbagec_objs.end(), std::make_pair(simobj->id, safePtr));
	}
	JSObject* out = JS_NewObject(cx, &ts_obj);
	JS_SetPrivate(out, (void*)safePtr);
	JS::RootedObject* sudo = new JS::RootedObject(cx, out);
	JS::RootedValue* val = new JS::RootedValue(cx, JS::PrivateValue((void*)safePtr));
	JS_DefineProperty(cx, *sudo, "__ptr__", *val, 0);
	JS::AutoValueArray<2> args(cx);
	args[0].setObject(*out); //Our raw SimObject* ptr
	JSObject* eeeee = *handler;
	args[1].setObject(*eeeee);

	JS::RootedObject proxiedObject(cx);
	JS::RootedValue* proxyConstructor = new JS::RootedValue(cx, JS::ObjectValue(**proxyStuff));
	if (!JS::Construct(cx, *proxyConstructor, JS::HandleValueArray(args), &proxiedObject)) {
		Printf("Failed to construct");
		return false;
	}
	a.rval().setObject(*proxiedObject);
	return true;
}

//Code duplication. Whatever.
bool js_ts_findObj(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	if (a.length() != 1) {
		JS_ReportErrorNumberASCII(cx, my_GetErrorMessage, nullptr, JSSMSG_INVALID_ARGS,
			"obj");
		return false;
	}
	SimObject* obj;
	if (a[0].isInt32()) {
		obj = Sim__findObject_id(a[0].toInt32());
	}
	else if (a[0].isNumber()) {
		obj = Sim__findObject_id(a[0].toNumber());
	}
	else if (a[0].isString()) {
		obj = Sim__findObject_name(JS_EncodeString(cx, a[0].toString()));
	}
	else {
		Printf("Invalid type passed to search..");
		return false;
	}

	if (obj == NULL) {
		Printf("Was not able to find object");
		return false;
	}

	SimObject** aa = NULL;
	std::map<int, SimObject**>::iterator it;
	it = garbagec_ids.find(obj->id);
	if (it != garbagec_ids.end())
	{
		aa = garbagec_ids.find(obj->id)->second;
		//Printf("using cached pointer for %d", obj->id);
		if (aa == NULL)
		{
			//Printf("CACHE MISS!!!");
			aa = (SimObject**)JS_malloc(cx, sizeof(SimObject*));
			*aa = obj;
			SimObject__registerReference(obj, aa);
			//JS_SetGCCallback(cx, js_finalizeCb, (void*)aa);
			garbagec_ids.insert(garbagec_ids.end(), std::make_pair(obj->id, aa));
		}
	}
	else {
		aa = (SimObject**)JS_malloc(cx, sizeof(SimObject*));
		*aa = obj;
		SimObject__registerReference(obj, aa);
		//JS_SetGCCallback(cx, js_finalizeCb, (void*)aa);
		//JS_SetGCCallback(cx, js_finalizeCb, (void*)aa);
		garbagec_ids.insert(garbagec_ids.end(), std::make_pair(obj->id, aa));
	}

	JSObject* out = JS_NewObject(cx, &ts_obj);
	JS_SetPrivate(out, (void*)aa);
	JS::RootedObject* sudo = new JS::RootedObject(cx, out);
	JS::RootedValue* val = new JS::RootedValue(cx, JS::PrivateValue((void*)aa));
	JS_DefineProperty(cx, *sudo, "__ptr__", *val, 0);
	JS::AutoValueArray<2> args(cx);
	args[0].setObject(*out); 
	args[1].setObject(**handler);
	JS::RootedObject proxiedObject(cx);
	if (!JS::Construct(cx, *proxyConstructor, JS::HandleValueArray(args), &proxiedObject)) {
		Printf("Failed to construct");
		return false;
	}
	//JS_AddFinalizeCallback(cx, js_finalizeCb, (void*)aa);
	a.rval().setObject(*proxiedObject);
	return true;
}

bool js_ts_reg(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	if (a.length() != 1 || !a[0].isObject()) {
		a.rval().setBoolean(false);
		return false;
	}
	//Go out young..
	//A flash of lightning
	//Clips the sun
	JS::RootedObject* ab = new JS::RootedObject(_Context, &a[0].toObject());
	JS::RootedValue ptr(cx);
	JS_GetProperty(cx, *ab, "__ptr__", &ptr);
	void* eek = ptr.toPrivate();
	if (eek != NULL && eek != nullptr) {
		SimObject** safePtr = (SimObject**)eek;
		if (safePtr != NULL && safePtr != nullptr) {
			SimObject* this_ = *safePtr;
			if (this_ != NULL && this_ != nullptr) {
				a.rval().setBoolean(SimObject__registerObject(this_));
				return true;
			}
			Printf("Getting the normal pointer failed");
			a.rval().setBoolean(false);
		}
		Printf("Getting the safe pointer failed");
		a.rval().setBoolean(false);
	}
	Printf("Getting instance failed");
	a.rval().setBoolean(false);
	return true;
}

static const char* ts_jsCallExpose(SimObject* obj, int argc, const char* argv[]) {

}

bool js_ts_addCallback(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	if (a.length() != 1 || !a[0].isObject()) {
		return false;
	}
	JS::RootedObject* ab = new JS::RootedObject(cx, &a[0].toObject());
	if (JS_ObjectIsFunction(cx, *ab)) {
		JS::RootedFunction cb(cx, ab);
		JSString* name = JS_GetFunctionId(cb);
		ConsoleFunction("", JS_EncodeString(cx, name), ts_jsCallExpose, "() - defined by javascript", 1, 20);
	}
}

bool js_ts_linkClass(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	if (a.length() != 1 || !a[0].isString()) {
		return false;
	}
	JSString* className = a[0].toString();
	//I bashed in your window
	//Set fire to all you love
	//When you realize you're lonely
	//I'll let it all go~
	//But I feel alive...
	//Next to you~ 
	JSFunction* func = JS_NewFunction(cx, js_ts_const, 0, JSFUN_CONSTRUCTOR, JS_EncodeString(cx, className));
	a.rval().setObject(*JS_GetFunctionObject(func));
	return true;
}


void js_errorHandler(JSContext* cx, JSErrorReport* rep) {
	Printf("\x03%s:%u: %s",
		rep->filename ? rep->filename : "<no filename>",
		(unsigned int)rep->lineno,
		rep->message().c_str());
}

const char PathSeparator = '\\';

static bool
IsAbsolutePath(const JSAutoByteString& filename)
{
	const char* pathname = filename.ptr();

	if (pathname[0] == PathSeparator)
		return true;

	// On Windows there are various forms of absolute paths (see
	// http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247%28v=vs.85%29.aspx
	// for details):
	//
	//   "\..."
	//   "\\..."
	//   "C:\..."
	//
	// The first two cases are handled by the test above so we only need a test
	// for the last one here.

	if ((strlen(pathname) > 3 &&
		isalpha(pathname[0]) && pathname[1] == ':' && pathname[2] == '\\'))
	{
		return true;
	}

	return false;
}

JSString*
ResolvePath(JSContext* cx, JS::HandleString filenameStr, bool scriptRelative)
{
	if (!filenameStr) {
		return JS_NewStringCopyZ(cx, "nul");
	}

	JSAutoByteString filename(cx, filenameStr);
	if (!filename)
		return nullptr;

	if (IsAbsolutePath(filename))
		return filenameStr;

	JS::AutoFilename scriptFilename;
	if (scriptRelative) {
		// Get the currently executing script's name.
		if (!DescribeScriptedCaller(cx, &scriptFilename))
			return nullptr;

		if (!scriptFilename.get())
			return nullptr;

		if (strcmp(scriptFilename.get(), "-e") == 0 || strcmp(scriptFilename.get(), "typein") == 0)
			scriptRelative = false;
	}

	char buffer[MAX_PATH + 1];
	if (scriptRelative) {
		// The docs say it can return EINVAL, but the compiler says it's void
		_splitpath(scriptFilename.get(), nullptr, buffer, nullptr, nullptr);
	}
	else {
		const char* cwd = _getcwd(buffer, MAX_PATH);
		if (!cwd)
			return nullptr;
	}

	size_t len = strlen(buffer);
	buffer[len] = '\\';
	strncpy(buffer + len + 1, filename.ptr(), sizeof(buffer) - (len + 1));
	if (buffer[MAX_PATH] != '\0')
		return nullptr;

	return JS_NewStringCopyZ(cx, buffer);
}


static bool LoadScript(JSContext* cx, unsigned argc, JS::Value* vp, bool scriptRelative)
{
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	JS::RootedString bleh(cx);
	for (unsigned i = 0; i < a.length(); i++) {
		bleh = JS::ToString(cx, a[i]);
		if (!bleh) {
			JS_ReportErrorNumberASCII(cx, my_GetErrorMessage, nullptr, JSSMSG_INVALID_ARGS,
				"load");
			return false;
		}
		bleh = ResolvePath(cx, bleh, scriptRelative);
		if (!bleh) {
			JS_ReportErrorASCII(cx, "unable to resolve path");
			return false;
		}
		JSAutoByteString filename(cx, bleh);
		if (!filename)
			return false;

		JS::CompileOptions opts(cx);
		opts.setIntroductionType("js shell load")
			.setUTF8(true)
			.setIsRunOnce(true)
			.setNoScriptRval(true);
		JS::RootedScript script(cx);
		JS::RootedValue unused(cx);
		if (!JS::Compile(cx, opts, filename.ptr(), &script) ||
			!JS::Evaluate(cx, opts, filename.ptr(), &unused))
		{
			return false;
		}
	}
}
JSObject*
FileAsTypedArray(JSContext* cx, JS::HandleString pathnameStr)
{
	JSAutoByteString pathname(cx, pathnameStr);
	if (!pathname)
		return nullptr;

	FILE* file = fopen(pathname.ptr(), "rb");
	if (!file) {
		/*
		* Use Latin1 variant here because the encoding of the return value of
		* strerror function can be non-UTF-8.
		*/
		JS_ReportErrorLatin1(cx, "can't open %s: %s", pathname.ptr(), strerror(errno));
		return nullptr;
	}
	//JS::AutoCloseFile autoClose(file);

	JS::RootedObject obj(cx);
	JS::RootedObject aaa(cx);
	if (fseek(file, 0, SEEK_END) != 0) {
		pathname.clear();
		if (!pathname.encodeUtf8(cx, pathnameStr))
			return nullptr;
		JS_ReportErrorUTF8(cx, "can't seek end of %s", pathname.ptr());
	}
	else {
		size_t len = ftell(file);
		if (fseek(file, 0, SEEK_SET) != 0) {
			pathname.clear();
			if (!pathname.encodeUtf8(cx, pathnameStr))
				return nullptr;
			JS_ReportErrorUTF8(cx, "can't seek start of %s", pathname.ptr());
		}
		else {
			obj = JS_NewUint8Array(cx, 0);
			if (!obj)
				return nullptr;
			char* buf = (char*)JS_malloc(cx, len);
			size_t cc = fread(buf, 1, len, file);
			fclose(file);
			if (cc != len) {
				if (ptrdiff_t(cc) < 0) {
					/*
					* Use Latin1 variant here because the encoding of the return
					* value of strerror function can be non-UTF-8.
					*/
					JS_ReportErrorUTF8(cx, "can't read file");
				}
				else {
					pathname.clear();
					if (!pathname.encodeUtf8(cx, pathnameStr))
						return nullptr;
					JS_ReportErrorUTF8(cx, "can't read %s: short read", pathname.ptr());
				}
				obj = nullptr;
				js_free(buf);
			}
			else {
				//obj = JS_NewArrayBufferWithExternalContents(cx, len * 2, buf);
				obj = JS_NewArrayBuffer(cx, len * 2);
				for (int i = 0; i < len; i++) {
					JS_SetElement(cx, obj, i, buf[i]);
				}
			}
		}
	}
	return obj;
}

JSString*
FileAsString(JSContext* cx, JS::HandleString pathnameStr)
{
	JSAutoByteString pathname(cx, pathnameStr);
	if (!pathname)
		return nullptr;

	FILE* file;

	file = fopen(pathname.ptr(), "rb");
	if (!file) {
		return nullptr;
	}


	if (fseek(file, 0, SEEK_END) != 0) {
		pathname.clear();
		if (!pathname.encodeUtf8(cx, pathnameStr))
			return nullptr;
		JS_ReportErrorUTF8(cx, "can't seek end of %s", pathname.ptr());
		return nullptr;
	}

	size_t len = ftell(file);
	if (fseek(file, 0, SEEK_SET) != 0) {
		pathname.clear();
		if (!pathname.encodeUtf8(cx, pathnameStr))
			return nullptr;
		JS_ReportErrorUTF8(cx, "can't seek start of %s", pathname.ptr());
		return nullptr;
	}

	JS::UniqueChars buf(static_cast<char*>(JS_malloc(cx, len + 1)));
	if (!buf)
		return nullptr;

	size_t cc = fread(buf.get(), 1, len, file);
	if (cc != len) {
		if (ptrdiff_t(cc) < 0) {
		}
		else {
			pathname.clear();
			if (!pathname.encodeUtf8(cx, pathnameStr))
				return nullptr;
			JS_ReportErrorUTF8(cx, "can't read %s: short read", pathname.ptr());
		}
		return nullptr;
	}

	JS::UniqueTwoByteChars ucbuf(
		JS::LossyUTF8CharsToNewTwoByteCharsZ(cx, JS::UTF8Chars(buf.get(), len), &len).get()
	);
	if (!ucbuf) {
		pathname.clear();
		if (!pathname.encodeUtf8(cx, pathnameStr))
			return nullptr;
		JS_ReportErrorUTF8(cx, "Invalid UTF-8 in file '%s'", pathname.ptr());
		return nullptr;
	}

	return JS_NewUCStringCopyN(cx, ucbuf.get(), len);
}

struct Export {
	const char* src;
	const char* dst;
};
const Export osfile_exports[] = {
	{ "readFile", "read" },
{ "readFile", "snarf" },
{ "readRelativeToScript", "readRelativeToScript" },
};

static bool
ospath_isAbsolute(JSContext* cx, unsigned argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	if (args.length() != 1 || !args[0].isString()) {
		JS_ReportErrorNumberASCII(cx, my_GetErrorMessage, nullptr, JSSMSG_INVALID_ARGS,
			"isAbsolute");
		return false;
	}

	JSAutoByteString path(cx, args[0].toString());
	if (!path)
		return false;

	args.rval().setBoolean(IsAbsolutePath(path));
	return true;
}

static bool
ospath_join(JSContext* cx, unsigned argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	if (args.length() < 1) {
		JS_ReportErrorNumberASCII(cx, my_GetErrorMessage, nullptr, JSSMSG_INVALID_ARGS, "join");
		return false;
	}

	// This function doesn't take into account some aspects of Windows paths,
	// e.g. the drive letter is always reset when an absolute path is appended.
	std::string a("");
	//StringBuffer buffer(cx);
	for (unsigned i = 0; i < args.length(); i++) {
		if (!args[i].isString()) {
			JS_ReportErrorASCII(cx, "join expects string arguments only");
			return false;
		}

		JSAutoByteString path(cx, args[i].toString());
		if (!path)
			return false;

		else if (i != 0) {
			std::string delim("\\");
			a += delim;
		}

		a += std::string(JS_EncodeString(cx, args[i].toString()));
	}

	args.rval().setString(JS_NewStringCopyZ(cx, a.c_str()));
	return true;
}



static bool
ReadFile(JSContext* cx, unsigned argc, JS::Value* vp, bool scriptRelative)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	if (args.length() < 1 || args.length() > 2) {
		JS_ReportErrorNumberASCII(cx, my_GetErrorMessage, nullptr,
			args.length() < 1 ? JSSMSG_NOT_ENOUGH_ARGS : JSSMSG_TOO_MANY_ARGS,
			"snarf");
		return false;
	}

	if (!args[0].isString() || (args.length() == 2 && !args[1].isString())) {
		JS_ReportErrorNumberASCII(cx, my_GetErrorMessage, nullptr, JSSMSG_INVALID_ARGS,
			"snarf");
		return false;
	}

	JS::RootedString givenPath(cx, args[0].toString());
	JS::RootedString str(cx, ResolvePath(cx, givenPath, scriptRelative));
	if (!str)
		return false;

	if (args.length() > 1) {
		JSString* opt = JS::ToString(cx, args[1]);
		if (!opt)
			return false;
		bool match;
		if (!JS_StringEqualsAscii(cx, opt, "binary", &match))
			return false;
		if (match) {
			JSObject* obj;
			if (!(obj = FileAsTypedArray(cx, str)))
				return false;
			args.rval().setObject(*obj);
			return true;
		}
	}

	if (!(str = FileAsString(cx, str)))
		return false;
	args.rval().setString(str);
	return true;
}

static bool
osfile_readFile(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return ReadFile(cx, argc, vp, false);
}

static bool
osfile_readRelativeToScript(JSContext* cx, unsigned argc, JS::Value* vp)
{
	return ReadFile(cx, argc, vp, true);
}

bool js_load(JSContext* cx, unsigned argc, JS::Value* vp) {
	return LoadScript(cx, argc, vp, false);
}

bool js_ScanFunc(JSContext* cx, unsigned argc, JS::Value* vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	if (a.length() != 2) {
		return false;
	}
	if (!a[0].isObject() || !a[1].isString()) {
		return false;
	}
	if (!JS_IsUint8Array(&a[0].toObject()))
	{
		return false;
	}
	JS::RootedObject arr(cx, &a[0].toObject());
	uint32_t out = 0;
	JS_GetArrayLength(cx, arr, &out);
	char* charA;
	charA = (char*)JS_malloc(cx, sizeof(char) * out);
	for (int i = 0; i < out; i++) {
		JS::RootedValue val(cx);
		JS_GetElement(cx, arr, i, &val);
		charA[i] = val.toInt32();
	}
	DWORD outPtr = ScanFunc((const char*)charA, JS_EncodeString(cx, a[1].toString()));
	a.rval().setInt32(outPtr);
	return true;
}

static const char* ts__js_exec(SimObject* obj, int argc, const char* argv[]) {
	JS::RootedString givenPath(_Context, JS_NewStringCopyZ(_Context, argv[1]));
	JS::RootedString str(_Context, ResolvePath(_Context, givenPath, false));
	if (!(str = FileAsString(_Context, str))) {
		return false;
	}
		JS::CompileOptions copts(_Context);
		copts.setFileAndLine(argv[1], 1);
		JS::RootedValue rval(_Context);
		bool success = JS::Evaluate(_Context, copts, JS_EncodeString(_Context, str), JS_GetStringLength(str), &rval);
		if (success) {
			return "true";
		}
		else {
		//	Printf("Eval fail");
			if (JS_IsExceptionPending(_Context)) {
				//Printf("exception is pending");
				JS::RootedValue excVal(_Context);
				JS_GetPendingException(_Context, &excVal);
				if (!excVal.isNull() && excVal.isObject()) {
					JS::RootedObject exc(_Context, &excVal.toObject());
					JSErrorReport* aaa = JS_ErrorFromException(_Context, exc);
					if (aaa) {
						//Printf("got error report");
						js_errorHandler(_Context, aaa);
					}
				}
				JS_ClearPendingException(_Context);
			}
			return "false";
		}
}

static const char* ts__js_eval(SimObject* obj, int argc, const char* argv[]) {
	
	//Printf("I'l be the king of me...");
	bool success = false;
	const char *filename = "noname";
	int lineno = 1;
	JS::CompileOptions copts(_Context);
	copts.setFileAndLine(filename, lineno)
		.setMutedErrors(false);
	JS::RootedValue rval(_Context);
	success = JS::Evaluate(_Context, copts, argv[1], strlen(argv[1]), &rval);
	if (success) {
		//Printf("eval");
		if (rval.isString()) {
			return JS_EncodeString(_Context, rval.toString());
		}
		return "true";
		//Printf("%s\n", JS_EncodeString(_Context, str));
	}
	else {
		//Printf("bleh");
		if (JS_IsExceptionPending(_Context)) {
			//Printf("exception is pending");
			JS::RootedValue excVal(_Context);
			JS_GetPendingException(_Context, &excVal);
			if (!excVal.isNull() && excVal.isObject()) {
				JS::RootedObject exc(_Context, &excVal.toObject());
				JSErrorReport* aaa = JS_ErrorFromException(_Context, exc);
				if (aaa) {
					//Printf("got error report");
					js_errorHandler(_Context, aaa);
				}
				JS_ClearPendingException(_Context);
			}
			else {
				JS_ClearPendingException(_Context);
			}
		}
	}
	return "false";
}

static bool
GetModuleLoadPath(JSContext* cx, unsigned argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

	char buffer[MAX_PATH + 1];
	const char* cwd = _getcwd(buffer, MAX_PATH);
	if (!cwd)
		return false;
	JS::RootedString sss(cx, JS_NewStringCopyZ(cx, cwd));
	if (!sss)
		return false;

	args.rval().setString(sss);
	return true;
}

static bool
ParseModule(JSContext* cx, unsigned argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (args.length() == 0) {
		JS_ReportErrorNumberASCII(cx, my_GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
			"parseModule", "0", "s");
		return false;
	}

	if (!args[0].isString()) {
		const char* typeName = InformalValueTypeName(args[0]);
		JS_ReportErrorASCII(cx, "expected string to compile, got %s", typeName);
		return false;
	}

	JSString* scriptContents = args[0].toString();
	if (!scriptContents)
		return false;

	JS::UniqueChars filename;
	JS::CompileOptions options(cx);
	if (args.length() > 1) {
		if (!args[1].isString()) {
			const char* typeName = InformalValueTypeName(args[1]);
			JS_ReportErrorASCII(cx, "expected filename string, got %s", typeName);
			return false;
		}

		JS::RootedString str(cx, args[1].toString());
		filename.reset(JS_EncodeString(cx, str));
		if (!filename)
			return false;

		options.setFileAndLine(filename.get(), 1);
	}
	else {
		options.setFileAndLine("<string>", 1);
	}

		const char16_t* chars = JS_GetTwoByteExternalStringChars(scriptContents);
	JS::SourceBufferHolder srcBuf(chars, JS_GetStringLength(scriptContents),
		JS::SourceBufferHolder::NoOwnership);

	JS::RootedObject module(cx);
	JS::CompileModule(cx, options, srcBuf, &module);
	if (!module)
		return false;

	args.rval().setObject(*module);
	return true;
}

static bool
SetModuleResolveHook(JSContext* cx, unsigned argc, JS::Value* vp)
{
	JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
	if (args.length() != 1) {
		JS_ReportErrorNumberASCII(cx, my_GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
			"setModuleResolveHook", "0", "s");
		return false;
	}

	if (!args[0].isObject() || !JS_ObjectIsFunction(cx, &args[0].toObject())) {
		const char* typeName = InformalValueTypeName(args[0]);
		JS_ReportErrorASCII(cx, "expected hook function, got %s", typeName);
		return false;
	}

	JS::RootedObject obj(cx, &args[0].toObject());
	JS::RootedFunction hook(cx, JS_GetObjectFunction(obj));
	JS::SetModuleResolveHook(_Context, hook);
	args.rval().setUndefined();
	return true;
}

bool
CreateAlias(JSContext* cx, const char* dstName, JS::HandleObject namespaceObj, const char* srcName)
{
	JS::RootedObject global(cx, JS_GetGlobalForObject(cx, namespaceObj));
	if (!global)
		return false;

	JS::RootedValue val(cx);
	if (!JS_GetProperty(cx, namespaceObj, srcName, &val))
		return false;

	if (!val.isObject()) {
		JS_ReportErrorASCII(cx, "attempted to alias nonexistent function");
		return false;
	}

	JS::RootedObject function(cx, &val.toObject());
	if (!JS_DefineProperty(cx, global, dstName, function, 0))
		return false;

	return true;
}

static const JSFunctionSpecWithHelp osfile_functions[] = {
	JS_FN_HELP("readFile", osfile_readFile, 1, 0,
	"readFile(filename, [\"binary\"])",
	"  Read entire contents of filename. Returns a string, unless \"binary\" is passed\n"
	"  as the second argument, in which case it returns a Uint8Array. Filename is\n"
		"  relative to the current working directory."),

	JS_FN_HELP("readRelativeToScript", osfile_readRelativeToScript, 1, 0,
		"readRelativeToScript(filename, [\"binary\"])",
		"  Read filename into returned string. Filename is relative to the directory\n"
		"  containing the current script."),

	JS_FS_HELP_END
};

static const JSFunctionSpecWithHelp ospath_functions[] = {
	JS_FN_HELP("isAbsolute", ospath_isAbsolute, 1, 0,
	"isAbsolute(path)",
	"  Return whether the given path is absolute."),

	JS_FN_HELP("join", ospath_join, 1, 0,
	"join(paths...)",
	"  Join one or more path components in a platform independent way."),

	JS_FS_HELP_END
};

static const JSFunctionSpecWithHelp global_funcs[] = {
	JS_FN_HELP("print", js_print, 1, 0,
	"print(arg[, arg])",
	"  Print."),
	JS_FN_HELP("load", js_load, 1, 0,
	"load(file ...)",
	"  Load file relative to current working directory."),
	JS_FN_HELP("parseModule", ParseModule, 1, 0,
	"parseModule(code)",
	"  Parses source text as a module and returns a Module object."),

	JS_FN_HELP("setModuleResolveHook", SetModuleResolveHook, 1, 0,
	"setModuleResolveHook(function(module, specifier) {})",
	"  Set the HostResolveImportedModule hook to |function|.\n"
	"  This hook is used to look up a previously loaded module object.  It should\n"
		"  be implemented by the module loader."),
	JS_FN_HELP("getModuleLoadPath", GetModuleLoadPath, 0, 0,
		"getModuleLoadPath()",
		"  Return any --module-load-path argument passed to the shell.  Used by the\n"
		"  module loader.\n"),
	JS_FS_HELP_END
};

static const JSFunctionSpecWithHelp simset_funcs[] = {
	JS_FN_HELP("SimSet_getObject", SS_gO, 2, 0,
	"SimSet_getObject(simset, index)",
	"  Get an object in a SimSet (torque type) at index."),
	JS_FS_HELP_END
};

static const JSFunctionSpecWithHelp js_funcs[] = {
	JS_FN_HELP("getVariable", js_getGlobalVariable, 1, 0,
"ts_getVariable(name)",
"  Get a global variable from Torque."),
	JS_FN_HELP("setVariable", js_setGlobalVariable, 2, 0,
"ts_setVariable(name, value)",
"  Set a global variable in Torque to value."),
	JS_FN_HELP("linkClass", js_ts_linkClass, 1, 0,
"ts_linkClass(class)",
"  Get an constructor to a class in Torque."),
	JS_FN_HELP("register", js_ts_reg, 1, 0,
"ts_registerObject(object)",
"  Register a Torque object, allowing functions on it to be called."),
	JS_FN_HELP("func", ts_func, 1, 0,
"ts_func(function_name)",
"  Get a callable pointer to an TorqueScript object."),
	JS_FN_HELP("plainCall", js_plainCall, 1, 0,
"ts_call(name[, args]",
"  Call a function on 'this', or the global namespace, if the object does not exist."),
	JS_FN_HELP("obj", js_ts_findObj, 1, 0,
"ts_obj(id/name)",
"  Get a object representing a Torque object."),
	JS_FN_HELP("ScanFunc", js_ScanFunc, 2, 0,
"ScanFunc(pattern, mask)",
"  Scan for a function in Blockland's memory."),
	JS_FS_HELP_END
};

bool init() {
	if (!torque_init())
		return false;

	Printf("BlocklandJS || Init");

	ConsoleFunction(NULL, "js_eval", ts__js_eval, "(<script>) - eval js stuff", 2, 2);

	ConsoleFunction(NULL, "js_exec", ts__js_exec, "(<path to file>) - exec js file", 2, 2);


	//Printf("JS_Init");
	if (!JS_Init()) {
		Printf("JavaScript initialization failed..");
		return false;
	}
	//Printf("New context");
	_Context = JS_NewContext(JS::DefaultHeapMaxBytes);
	if (!_Context) {
		Printf("Failed to create a new JS context");
		return false;
	}

	//Printf("Self-hosted code init");
	if (!JS::InitSelfHostedCode(_Context)) {
		Printf("Failed to init self hosted code..");
		return false;
	}

	//Printf("Beginning JS request");
	JS::SetWarningReporter(_Context, js_errorHandler);
	JS_BeginRequest(_Context);
	JS::CompartmentOptions opts;
	//Printf("Setting up the global object");
	//_Global = JS_NewObject(_Context, &global_class);
	_Global = new JS::RootedObject(_Context, JS_NewGlobalObject(_Context, &global_class, nullptr, JS::FireOnNewGlobalHook, opts));
	if (!_Global) {
		//Printf("JS ERROR: Was not able to init global..");
		return false;
	}
	//Printf("We're done here.");
	//Printf("Entering compartment");
	JS_EnterCompartment(_Context, *_Global);
	JS_InitStandardClasses(_Context, *_Global);

	//Printf("Registering a whole lot of stuff");
	setter = new JS::RootedFunction(_Context, JS_NewFunction(_Context, js_setObjectVariable, 3, 0, NULL));
	getter = new JS::RootedFunction(_Context, JS_NewFunction(_Context, js_getObjectVariable, 2, 0, NULL));

	handler = new JS::RootedObject(_Context, JS_NewPlainObject(_Context));
	getObj = new JS::RootedObject(_Context, JS_GetFunctionObject(*getter));
    setObj = new JS::RootedObject(_Context, JS_GetFunctionObject(*setter));
	ireallydo = new JS::RootedObject(_Context, *handler);
	JS::RootedObject fuck(_Context);
	JS_GetClassObject(_Context, JSProto_Proxy, &fuck);
	proxyStuff = new JS::RootedObject(_Context, fuck);
	proxyConstructor = new JS::RootedValue(_Context, JS::ObjectValue(**proxyStuff));
	/*
	if (JS_DefineFunction(_Context, *_Global, "print", js_print, 1, JSPROP_READONLY | JSPROP_PERMANENT) == NULL) {
		//Printf("Was not able to define print..");
		return false;
	}

	if (JS_DefineFunction(_Context, *_Global, "ts_getVariable", js_getGlobalVariable, 1, JSPROP_READONLY | JSPROP_PERMANENT) == NULL) {
		return false;
	}

	if (JS_DefineFunction(_Context, *_Global, "ts_setVariable", js_setGlobalVariable, 2, JSPROP_READONLY | JSPROP_PERMANENT) == NULL) {
		return false;
	}


	JS_DefineFunction(_Context, *_Global, "ts_linkClass", js_ts_linkClass, 1, JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(_Context, *_Global, "ts_registerObject", js_ts_reg, 1, JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(_Context, *_Global, "ts_func", ts_func, 1, JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(_Context, *_Global, "ts_call", js_plainCall, 1, JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(_Context, *_Global, "ts_obj", js_ts_findObj, 1, JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(_Context, *_Global, "ts_addCallback", js_ts_addCallback, 1, JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(_Context, *_Global, "SimSet_getObject", SS_gO, 2, JSPROP_READONLY | JSPROP_PERMANENT);
	*/
	//Printf("Defining functions..");
	JS::RootedObject ts(_Context, JS_NewPlainObject(_Context));
	if (!ts || !JS_DefineProperty(_Context, *_Global, "ts", ts, 0)) {
		return false;
	}
	if (!JS_DefineFunctionsWithHelp(_Context, *_Global, global_funcs)) {
		Printf("\x03Unable to define global functions.");
		return false;
	}
	if (!JS_DefineFunctionsWithHelp(_Context, ts, js_funcs))
	{
		Printf("\x03Unable to define functions with help.");
		return false;
	}
	JS::RootedObject tssimset(_Context, JS_NewPlainObject(_Context));
	if (!JS_DefineFunctionsWithHelp(_Context, ts, simset_funcs) ||
		!JS_DefineProperty(_Context, ts, "SimSet", tssimset, 0))
	{
		Printf("\x03Unable to define SimSet..");
		return false;
	}
	JS::RootedObject obj(_Context, JS_NewPlainObject(_Context));
	if (!obj || !JS_DefineProperty(_Context, *_Global, "os", obj, 0))
	{
		Printf("\x03Unable to define OS object.");
		return false;
	}
	JS::RootedObject osfile(_Context, JS_NewPlainObject(_Context));
	if (!osfile ||
		!JS_DefineFunctionsWithHelp(_Context, osfile, osfile_functions) ||
		!JS_DefineProperty(_Context, obj, "file", osfile, 0))
	{
		return false;
	}

	JS::RootedObject ospath(_Context, JS_NewPlainObject(_Context));
	if (!ospath ||
		!JS_DefineFunctionsWithHelp(_Context, ospath, ospath_functions) ||
		!JS_DefineProperty(_Context, obj, "path", ospath, 0))
	{
		return false;
	}
	//Printf("Defining properties");
	JS_DefineProperty(_Context, *ireallydo, "get", *getObj, 0);
	JS_DefineProperty(_Context, *ireallydo, "set", *setObj, 0);
	for (auto pair : osfile_exports) {
		if (!CreateAlias(_Context, pair.dst, osfile, pair.src))
			return false;
	}
	Printf("BlocklandJS || Injected");
	//JS_Comp


	//JS_InitClass(_Context, *_Global, nullptr, &ts_obj, js_ts_const, 1, NULL, NULL, NULL, NULL);
	//JS_NewFunction(_Context, js_print, 1, 0, "print");
	//Printf("Or are we?"); 
	/*

	JSAutoRequest ar(_Context);
	JS::CompartmentOptions opts;
	JS::RootedObject global(_Context, JS_NewGlobalObject(_Context, &global_class, nullptr, JS::FireOnNewGlobalHook, opts));
	if (!global) {
		Printf("Failed to setup a new global object");
		return false;
	}

	JS::RootedValue rval(_Context);
	{
		JSAutoCompartment ar(_Context, global);
		JS_InitStandardClasses(_Context, global);
		const char *script = "'hello'+'world, it is '+new Date()";
		const char *filename = "noname";
		int lineno = 1;
		JS::CompileOptions copts(_Context);
		copts.setFileAndLine(filename, lineno);
		bool ok = JS::Evaluate(_Context, copts, script, strlen(script), &rval);
		if (!ok) {
			Printf("Was not able to evaluate..");
			return false;
		}
	}
	JSString *str = rval.toString();
	Printf("%s\n", JS_EncodeString(_Context, str));
	*/
	return true;
}

bool deinit() {
	//Printf("Shutting down..");
	JS::ShutdownAsyncTasks(_Context);
	//Printf("Ending request");
	//JS_GC(_Context);
	JS_EndRequest(_Context);
	//_Global = NULL;
	//Printf("Destroying context");
	//JS_DestroyContext(_Context);
	//Printf("Calling shutdown");
	//JS_ShutDown(); 
	//Printf("BlocklandJS || detached");
	//The OS should.
	return true;
}

int __stdcall DllMain(HINSTANCE hInstance, unsigned long reason, void* reserved){
	if(reason == DLL_PROCESS_ATTACH)
		return init();
	else if (reason == DLL_PROCESS_DETACH)
		return deinit();

	return true;
}

extern "C" void __declspec(dllexport) __cdecl placeholder(void) { }
