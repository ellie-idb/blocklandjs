#define WIN32_LEAN_AND_MEAN
#pragma once
#include "uv8.h"
#include "include\v8.h"
#include "Torque.h"
#include "sqlite3.h"

Persistent<ObjectTemplate> sqliteconstructor;

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

void sqlite_driver_init(Isolate* this_, Local<ObjectTemplate> global) {
	Local<FunctionTemplate> sqlite = FunctionTemplate::New(_Isolate, js_sqlite_new);
	sqlite->SetClassName(String::NewFromUtf8(_Isolate, "sqlite"));
	sqlite->InstanceTemplate()->Set(_Isolate, "open", FunctionTemplate::New(_Isolate, js_sqlite_open));
	sqlite->InstanceTemplate()->Set(_Isolate, "close", FunctionTemplate::New(_Isolate, js_sqlite_close));
	sqlite->InstanceTemplate()->Set(_Isolate, "exec", FunctionTemplate::New(_Isolate, js_sqlite_exec));
	sqlite->InstanceTemplate()->SetInternalFieldCount(2);
	global->Set(_Isolate, "sqlite", sqlite);
	sqliteconstructor.Reset(_Isolate, sqlite->InstanceTemplate());
}