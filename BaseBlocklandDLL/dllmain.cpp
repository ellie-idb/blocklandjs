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
#include "detours.h"
#include <string.h>
#include <chrono>
#include "Torque.h"
#include <vector>
#include "uv8.h"
#include "include/url.h"
#include "sqlite3.h"
#include "tvector.h"

#pragma warning( push )
#pragma warning( disable : 4946 )

#define BLJS_VERSION "v8.1.8"

using namespace v8;
char* oldbuf;
char* tabbuf;
int lastSuggestion = 0;
bool* isJS = new bool(false);

MologieDetours::Detour<Con__tabCompleteFn>* Con__tabComplete_Detour;

Platform *_Platform;
Isolate *_Isolate;
Persistent<Context> _Context;

Local<Context> ContextL() {
	return StrongPersistentTL(_Context);
}

static uv_loop_t loop;
static uv_idle_t* idle;
static uv_thread_t thread;

int verbosity = 0;

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

void idle_cb(uv_idle_t* handle) {
	if (!immediateMode) {
		Sleep(1); //Sleep for 1ms.
	}
}

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

void js_print(const FunctionCallbackInfo<Value> &args)
{
	js_handlePrint(args, "");
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

void RetStuff(Local<Context> b, Local<Module> c, Local<Object> d) {
	/* 
		This is where the meta field of import will come in. I'm hoping to get this done later.
	*/
	Isolate* isolate = b->GetIsolate(); 
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

U32 Con__tabComplete_hook(char* inputBuf, U32 cursorPos, U32 maxRLen, bool forwardTab) {
	if (*isJS) {
		Locker locker(_Isolate);
		Isolate::Scope iso_scope(_Isolate);
		_Isolate->Enter();
		HandleScope handle_scope(_Isolate);
		ContextL()->Enter();
		v8::Context::Scope cScope(ContextL());

		//Printf("%d", lastSuggestion);
		U32 ret = cursorPos;
		Local<String> key = String::NewFromUtf8(_Isolate, "__autoCompleteJS__");
		Local<Function> ac = Local<Function>::Cast(ContextL()->Global()->Get(key)->ToObject());
		Local<Value> args[1];
		if (_stricmp(inputBuf, tabbuf) == 0) {
			//Make sure that the autocompletion uses the old buffer's contents, since they want to cycle.
			args[0] = String::NewFromUtf8(_Isolate, oldbuf);
		}
		else {
			lastSuggestion = 0; //we got new unique input
			args[0] = String::NewFromUtf8(_Isolate, inputBuf);
		}
		Local<Array> acSuggestion = Local<Array>::Cast(ac->Call(ContextL()->Global(), 1, args));
		if (acSuggestion->Length() == 0) {
			//We got nothing! Clear the tab buffer, just in case, then return the cursor position.
			strncpy(tabbuf, "", 256);
			ContextL()->Exit();
			_Isolate->Exit();
			Unlocker unlocker(_Isolate);
			return cursorPos;
		}
		if (acSuggestion->Length() != 1) {
			//Printf("%s", *String::Utf8Value(acSuggestion->ToString()));
			if (_stricmp(inputBuf, tabbuf) == 0) {
				//The input recieved was the same one that the autocompleter gave
				//This means that they want to cycle through.
				lastSuggestion++;
			}
			else {
				lastSuggestion = 0; //Something happened, and the user probably changed their input.
				//Reset the suggestions, and rebase it around this new input.
				strncpy(oldbuf, inputBuf, 256);
			}
		}

		Local<String> firstVal;
		if (_stricmp(inputBuf, tabbuf) == 0) {
			if (lastSuggestion >= acSuggestion->Length()) {
				//Printf("reset because too long");
				//Make sure we don't exceed the length of the autocompletion array.
				lastSuggestion = 0;
			}
			//Grab the corresponding suggestion value.
			firstVal = acSuggestion->Get(lastSuggestion)->ToString();
		}
		else {
			//They aren't cycling, so we should just grab the 0th value.
			firstVal = acSuggestion->Get(0)->ToString();
		}
		String::Utf8Value c_firstVal(firstVal);
		//Now, copy it into the input buffer (torque asks for this)
		strncpy(inputBuf, *c_firstVal, maxRLen);
		//And also copy it into our persistent buffer so we can check later.
		strncpy(tabbuf, *c_firstVal, maxRLen);
		//Now return the length of the string, so the cursor can update to it's position
		//And we're done!
		ret = c_firstVal.length();

		ContextL()->Exit();
		_Isolate->Exit();
		Unlocker unlocker(_Isolate);
		return ret;
	}
	else {
		//Fallback, in case the user is using TorqueScript, and wants autocomplete there.
		return Con__tabComplete_Detour->GetOriginalFunction()(inputBuf, cursorPos, maxRLen, forwardTab);
	}
}

bool init()
{
	//Ensure that we were able to find all of the functions needed.
	if (!torque_init())
	{
		return false;
	}
	 
	Printf("BlocklandJS || Init");
	Printf("BlocklandJS: version %s", BLJS_VERSION);
	//Startup the libuv loop here
	uv_loop_init(uv_default_loop());
	_Platform = platform::CreateDefaultPlatform(); //Initialize the Platform backing V8.
	V8::InitializePlatform(_Platform);
	const char* v8flags = "--harmony --harmony-dynamic-import"; //Enable some experimental features from V8.

	Con__tabComplete_Detour = new MologieDetours::Detour<Con__tabCompleteFn>(Con__tabComplete, Con__tabComplete_hook);

	/*
	size_t sz = _MAX_PATH * 2;
	char path[_MAX_PATH * 2];
	uv_cwd(path, &sz); 
	V8::InitializeICUDefaultLocation(path); We don't support anything other then ASCII and UTF-8, so we don't need this.
	*/
	oldbuf = new char[256];
	tabbuf = new char[256];
	V8::SetFlagsFromString(v8flags, strlen(v8flags));
	V8::Initialize();

	Isolate::CreateParams create_params;
	create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
	_Isolate = Isolate::New(create_params);
	_Isolate->Enter();

	_Isolate->SetData(0, (void*)uv_default_loop());

	HandleScope scope(_Isolate);

	/* global */
	//Setup the Global object in JavaScript. If anything is to be registered, it should be done here for the global.
	ConsoleVariable("jsEnabled", isJS);
	Local<ObjectTemplate> global = ObjectTemplate::New(_Isolate);
	global->Set(_Isolate, "print", FunctionTemplate::New(_Isolate, js_print));
	global->Set(_Isolate, "load", FunctionTemplate::New(_Isolate, Load));
	global->Set(_Isolate, "version", FunctionTemplate::New(_Isolate, js_version));
	global->Set(_Isolate, "__mappingTable__", ObjectTemplate::New(_Isolate));
	global->Set(_Isolate, "immediateMode", FunctionTemplate::New(_Isolate, js_setImmediateMode));
	global->Set(_Isolate, "__verbosity__", FunctionTemplate::New(_Isolate, js_modifyVerbosity));

	/* console */
	//Setup the console object for ease-of-use.
	Local<ObjectTemplate> console = ObjectTemplate::New(_Isolate);
	console->Set(_Isolate, "log", FunctionTemplate::New(_Isolate, js_console_log));
	console->Set(_Isolate, "warn", FunctionTemplate::New(_Isolate, js_console_warn));
	console->Set(_Isolate, "error", FunctionTemplate::New(_Isolate, js_console_error));
	console->Set(_Isolate, "info", FunctionTemplate::New(_Isolate, js_console_info));

	//Register some things necessary for ES6 modules to work here.
	_Isolate->SetHostImportModuleDynamicallyCallback(ImportModuleDynam);
	_Isolate->SetHostInitializeImportMetaObjectCallback(RetStuff);


	//Bind the LibUV bindings.
	uv8_bind_all(_Isolate, global);
	//Bind the TS bridge.
	ts_bridge_init(_Isolate, global);
	//Bind the SQLite database driver.
	sqlite_driver_init(_Isolate, global);

	//And now register 'console' on the global namespace so it can be called.
	global->Set(_Isolate, "console", console);

	//Create the context, with the given global.
	Local<v8::Context> context = Context::New(_Isolate, NULL, global);
	//Now reset the persistent reference to the context, so we don't have to reconstruct it.
	_Context.Reset(_Isolate, context);

	ContextL()->Enter();

	const char* autocompleteScript =
		"function __autoCompleteJS__(i) {"
		"var pattern = i; var head = '';"
		"pattern.replace(/ \\W*([\\w\\.] + )$ / , function(a, b, c) { head = pattern.substr(0, c + a.length - b.length); pattern = b });"
		"var index = pattern.lastIndexOf('.');"
		"var scope = this;"
		"var left = '';"
		"if (index >= 0) {"
		"	left = pattern.substr(0, index + 1);"
		"	try { scope = eval(pattern.substr(0, index)); }"
		"	catch (e) { scope = null; }"
		"	pattern = pattern.substr(index + 1);"
		"}"
		"result = [];"
		"for (var k in scope) {"
		"	if (k.indexOf(pattern) == 0) {"
		"		result.push(head + left + k);"
		"	}"
		"}"
		"return result;"
		"}";

	Local<String> ac = String::NewFromUtf8(_Isolate, autocompleteScript);
	ScriptCompiler::Source src(ac);
	Local<Script> sc;
	ScriptCompiler::Compile(ContextL(), &src).ToLocal(&sc);
	sc->Run();

	//We're done here, exit the isolate so it can be opened by other threads.
	_Isolate->Exit();

	//Register our TorqueScript API functions after JS has finished initializing.
	ConsoleFunction(NULL, "js_eval", ts_js_eval, "(string script) - Evaluate a script in the JavaScript engine.", 2, 2);
	ConsoleFunction(NULL, "js_exec", ts_js_exec, "(string filename) - Execute a script in the JavaScript engine.", 2, 2);

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
	delete Con__tabComplete_Detour;
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
