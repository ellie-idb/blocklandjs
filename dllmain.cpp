#include <jsapi.h>
#include <js/Initialization.h>
#include <js/Proxy.h>
#include "torque.h"

bool js_getObjectVariable(JSContext* cx, JS::HandleObject obj, JS::HandleId id, JS::Value* vp);

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

bool js_print(JSContext* cx, unsigned argc, JS::Value *vp) {
	JS::CallArgs a = JS::CallArgsFromVp(argc, vp);
	if (a.length() == 0) {
		return false;
	}
	if (a[0].isString()) {
		JSString *str = a[0].toString();
		Printf("%s", JS_EncodeString(cx, str));
	}
	else if (a[0].isNull()) {
		Printf("(null)");
	}
	else if(a[0].isObject()) 
	{
		bool t;
		JS::RootedObject* obj = new JS::RootedObject(cx, &a[0].toObject());
		if (JS_IsArrayObject(cx, *obj, &t)) {
			if (t) {
				uint32_t len;
				if (!JS_GetArrayLength(cx, *obj, &len)) {
					return false;
				}
				for (uint32_t i = 0; i < len; ++i) {
					JS::RootedValue aa(cx);
					if (JS_GetElement(cx, *obj, i, &aa))
					{
						if (aa.isString()) {
							Printf("%s", JS_EncodeString(cx, aa.toString()));
						}
						else {
							Printf("found element in array..");
						}
					}
				}
			}
		}
		else {
			Printf("(object)");
		}
	}
	else if (a[0].isBoolean()) {
		Printf("%s", a[0].toBoolean() ? "true" : "false");
	}
	else if (a[0].isInt32()) {
		Printf("%d", a[0].toInt32());
	}
	else if (a[0].isNumber()) {
		Printf("%d", a[0].toNumber());
	}
	else if (a[0].isMagic()) {
		Printf("(fucking magic)");
	}
	else
	{
		Printf("(unhandled type)");
	}
	return true;
}

/*
bool js_getObjectVariable(JSContext* cx, unsigned argc, JS::Value* vp) {
	void* eek = JS_GetInstancePrivate(cx, obj, &ts_obj, NULL);
	if (eek != NULL && eek != nullptr) {
		SimObject** safePtr = (SimObject**)eek;
		if (safePtr != NULL && safePtr != nullptr) {
			SimObject* this_ = *safePtr;
			if (this_ != NULL && this_ != nullptr) {
				JS::RootedValue key(cx);
				JS_IdToValue(cx, id, &key);
				if (key.isString() || key.isNumber()) {
					JSString* out;
					const char* data;
					if (_stricmp(JS_EncodeString(cx, key.toString()), "id")) {
						char buf[7];
						sprintf_s(buf, "%d", this_->id);
						data = const_cast<char*>(buf);
					}
					else {
						data = SimObject__getDataField(this_, JS_EncodeString(cx, key.toString()), StringTableEntry(""));
					}
					out = JS_NewStringCopyZ(cx, data);
					vp = &JS::StringValue(out);
					return true;
				}
			}

		}
	}
	return false;
}
*/

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
	simobj->mFlags |= SimObject::ModStaticFields;
	simobj->mFlags |= SimObject::ModDynamicFields;
	JSObject* out = JS_NewObject(cx, &ts_obj);
	//JS::RootedObject* proxyHandler = new JS::RootedObject(cx, JS_NewPlainObject(cx));
	//JSFunction* get = JS_NewFunction(cx, js_getObjectVariable, 0, 0, "");
	//JS::RootedObject* lol = new JS::RootedObject(cx, JS_GetFunctionObject(get));
	//JS_DefineProperty(cx, *proxyHandler, "get", *lol, 0);
	//Ugh.
	//JS::AutoValueArray<2> argss(cx);
	//argss[0].setObject(*out);
	//argss[1].setObject(*proxyHandler->get());
	//JS::HandleValueArray jesus(argss);
	//JSString* wtflol = JS_NewStringCopyZ(cx, "Proxy");
	//JS::HandleValue name();
	//JS::MutableHandleObject outtt();
//	JS::Construct(cx, name, jesus, &outtt);


	//JS_DefineProperty()
	JS_SetPrivate(out, (void*)safePtr);
	a.rval().setObject(*out);
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
	void* eek = JS_GetInstancePrivate(cx, *ab, &ts_obj, NULL);
	if (eek != NULL && eek != nullptr) {
		SimObject** safePtr = (SimObject**)eek;
		if (safePtr != NULL && safePtr != nullptr) {
			SimObject* this_ = *safePtr;
			if (this_ != NULL && this_ != nullptr) {
				a.rval().setBoolean(SimObject__registerObject(this_));
				return true;
			}
			a.rval().setBoolean(false);
		}
		a.rval().setBoolean(false);
	}
	a.rval().setBoolean(false);
	return true;
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


void js_errorHandler(JSContext* cx, const char* message, JSErrorReport* rep) {
	Printf("JS Error: %s", message);
}

static const char* ts__js_eval(SimObject* obj, int argc, const char* argv[]) {
	
	//Printf("I'll be the king of me...");
	bool success;
	const char *filename = "noname";
	int lineno = 1;
	JS::CompileOptions copts(_Context);
	copts.setFileAndLine(filename, lineno);
	JS::RootedValue rval(_Context);
	success = JS::Evaluate(_Context, copts, argv[1], strlen(argv[1]), &rval);
	if (success) {
		if (rval.isString()) {
			return JS_EncodeString(_Context, rval.toString());
		}
		return "true";
		//Printf("%s\n", JS_EncodeString(_Context, str));
	}
	if (JS_IsExceptionPending(_Context)) {
		JS_GetPendingException(_Context, &rval);
		if (rval.isString()) {
			JSString* str = rval.toString();
			Printf("JS Exception: %s", JS_EncodeString(_Context, str));
		}
		JS_ClearPendingException(_Context);
	}
	return "false";
}

bool deinit() {
	Printf("Shutting down..");
	JS_EndRequest(_Context);
	JS_DestroyContext(_Context);
	JS_ShutDown();
	return true;
}

bool init() {
	if (!torque_init())
		return false;

	Printf("hi");

	ConsoleFunction(NULL, "js_eval", ts__js_eval, "(<script>) - eval js stuff", 2, 2);


	JS_Init();
	_Context = JS_NewContext(8L * 1024 * 1024);
	
	if (!_Context) {
		Printf("Failed to create a new JS context");
		return false;
	}

	if (!JS::InitSelfHostedCode(_Context)) {
		Printf("Failed to init self hosted code..");
		return false;
	}

	//Printf("Beginning JS request");
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
	JS_EnterCompartment(_Context, *_Global);
	JS_InitStandardClasses(_Context, *_Global);
	if (JS_DefineFunction(_Context, *_Global, "print", js_print, 1, JSPROP_READONLY | JSPROP_PERMANENT) == NULL) {
		//Printf("Was not able to define print..");
		return false;
	}

	if (JS_DefineFunction(_Context, *_Global, "getGlobalVariable", js_getGlobalVariable, 1, JSPROP_READONLY | JSPROP_PERMANENT) == NULL) {
		return false;
	}

	if (JS_DefineFunction(_Context, *_Global, "setGlobalVariable", js_setGlobalVariable, 2, JSPROP_READONLY | JSPROP_PERMANENT) == NULL) {
		return false;
	}

	JS_DefineFunction(_Context, *_Global, "ts_linkClass", js_ts_linkClass, 1, JSPROP_READONLY | JSPROP_PERMANENT);
	JS_DefineFunction(_Context, *_Global, "ts_registerObject", js_ts_reg, 1, JSPROP_READONLY | JSPROP_PERMANENT);

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

int __stdcall DllMain(HINSTANCE hInstance, unsigned long reason, void* reserved){
	if(reason == DLL_PROCESS_ATTACH)
		return init();
	else if (reason == DLL_PROCESS_DETACH)
		return deinit();

	return true;
}
