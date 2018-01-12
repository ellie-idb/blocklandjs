#include <jsapi.h>
#include <js/Initialization.h>

#include "torque.h"

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
		Printf("(object)");
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