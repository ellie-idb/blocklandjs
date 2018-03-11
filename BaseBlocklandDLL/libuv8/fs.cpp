#pragma once
#include "uv8.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>
namespace fs = std::experimental::filesystem;
using namespace v8;
//And if you want me to come home, baby
//That's what I'm gonna do...
Persistent<ObjectTemplate> uvfs;

char initialCWD[_MAX_PATH * 2];

uv_errno_t a;
static int string_to_flags(Isolate* this_, const char* string) {
	bool read = false;
	bool write = false;
	int flags = 0;
	while (string && string[0]) {
		switch (string[0]) {
		case 'r': read = true; break;
		case 'w': write = true; flags |= O_TRUNC | O_CREAT; break;
		case 'a': write = true; flags |= O_APPEND | O_CREAT; break;
		case '+': read = true; write = true; break;
		case 'x': flags |= O_EXCL; break;

#ifndef _WIN32
		case 's': flags |= O_SYNC; break;
#endif

		default:
			ThrowError(this_, "Unknown flag");
			return 0;
		}
		string++;
	}
	flags |= read ? (write ? O_RDWR : O_RDONLY) :
		(write ? O_WRONLY : 0);
	return flags;
}


Handle<Object> uv8_fs_get_timespec(Isolate* this_, const uv_timespec_t* t) {
	Handle<Object> ret = Object::New(this_);
	ret->Set(String::NewFromUtf8(this_, "sec"), Uint32::NewFromUnsigned(this_, t->tv_sec));
	ret->Set(String::NewFromUtf8(this_, "nsec"), Uint32::NewFromUnsigned(this_, t->tv_nsec));
	return ret;
}

Handle<Object> uv8_fs_get_stats(Isolate* this_, const uv_stat_t* s) {
	Handle<Object> ret = Object::New(this_);
	const char* type = NULL;
	ret->Set(String::NewFromUtf8(this_, "mode"), Uint32::NewFromUnsigned(this_, s->st_mode));
	ret->Set(String::NewFromUtf8(this_, "uid"), Uint32::NewFromUnsigned(this_, s->st_uid));
	ret->Set(String::NewFromUtf8(this_, "gid"), Uint32::NewFromUnsigned(this_, s->st_gid));
	ret->Set(String::NewFromUtf8(this_, "size"), Uint32::NewFromUnsigned(this_, s->st_size));
	ret->Set(String::NewFromUtf8(this_, "atime"), uv8_fs_get_timespec(this_, &s->st_atim));
	ret->Set(String::NewFromUtf8(this_, "mtime"), uv8_fs_get_timespec(this_, &s->st_mtim));
	ret->Set(String::NewFromUtf8(this_, "ctime"), uv8_fs_get_timespec(this_, &s->st_ctim));
		if (s->st_mode == S_IFREG) {
			type = "file";
		}
		else if (s->st_mode == S_IFDIR) {
			type = "directory";
		}
		else if (s->st_mode == S_IFLNK) {
			type = "link";
		}
		else if (s->st_mode == _S_IFIFO) {
			type = "fifo";
		}
#ifdef S_ISSOCK
		else if (S_ISSOCK(s->st_mode)) {
			type = "socket";
		}
#endif
		else if (s->st_mode == S_IFCHR) {
			type = "char";
		}

		if (type) {
			ret->Set(String::NewFromUtf8(this_, "type"), String::NewFromUtf8(this_, type));
		}
		return ret;
}

void handleReturnFromFS(uv_fs_t* req, const FunctionCallbackInfo<Value> & args, void* base = nullptr, int size = 0) { 
	Handle<ArrayBuffer> abuf;
	Handle<External> aaa;
	if (req->fs_type == UV_FS_READ) {
		abuf = ArrayBuffer::New(args.GetIsolate(), size);
	}
	if (req->fs_type == UV_FS_SCANDIR) {
		aaa = External::New(args.GetIsolate(), (void*)req);
	}
	switch (req->fs_type) {
	case UV_FS_CLOSE:
	case UV_FS_RENAME:
	case UV_FS_UNLINK:
	case UV_FS_RMDIR:
	case UV_FS_MKDIR:
	case UV_FS_FTRUNCATE:
	case UV_FS_FSYNC:
	case UV_FS_FDATASYNC:
	case UV_FS_LINK:
	case UV_FS_SYMLINK:
	case UV_FS_CHMOD:
	case UV_FS_FCHMOD:
	case UV_FS_CHOWN:
	case UV_FS_FCHOWN:
	case UV_FS_UTIME:
	case UV_FS_FUTIME:
		args.GetReturnValue().Set(Boolean::New(args.GetIsolate(), true));
		//argc++;
		break;

	case UV_FS_OPEN:
	case UV_FS_SENDFILE:
	case UV_FS_WRITE:
		args.GetReturnValue().Set(Int32::New(args.GetIsolate(), req->result));
		break;

	case UV_FS_STAT:
	case UV_FS_LSTAT:
	case UV_FS_FSTAT:
		args.GetReturnValue().Set(uv8_fs_get_stats(args.GetIsolate(), &req->statbuf));
		break;

	case UV_FS_READLINK:
		args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), (const char*)req->ptr));
		break;

	case UV_FS_MKDTEMP:
		args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), req->path));
		break;


	case UV_FS_READ:
		memcpy(abuf->GetContents().Data(), base, size);
		uv8_free(args.GetIsolate(), base);
		args.GetReturnValue().Set(Uint8Array::New(abuf, 0, size));
		break;

	case UV_FS_SCANDIR:
		args.GetReturnValue().Set(FunctionTemplate::New(args.GetIsolate(), uv8_fs_scandir_next, aaa)->GetFunction());
		//argc++;
		break;

	default:
		ThrowError(args.GetIsolate(), "WTF??");
		return;

	}

}

int grabTheShit(const char* inPath) {
	uv_fs_t* req = new uv_fs_t;
	int ret = uv_fs_realpath(uv_default_loop(), req, inPath, nullptr);
	fs::path cwd(initialCWD);
	std::string initialCWD((const char*)initialCWD);
	fs::path manip(inPath);
	///int ret = uv_fs_stat(uv_default_loop(), req, inPath, nullptr);
	if (ret < 0) {
		//

		fs::path fp = cwd / manip;
		
		manip = fp;
		//aa.replace(aa.begin(), aa.end(), "/", "\\");
		int ret2 = uv_fs_realpath(uv_default_loop(), req, fp.generic_u8string().c_str(), nullptr);
		if (ret2 < 0) {
			Printf("tried %s", fp.generic_u8string().c_str());
			Printf("also tried %s", inPath);
			Printf("err: %s", uv_strerror(ret));
		}
	}
	else {
		manip = fs::path((const char*)req->ptr);
	}


	Printf("parent path of manip: %s", manip.parent_path().generic_u8string().c_str());
	if (manip.parent_path() == cwd) {
		Printf("ROOT DIRECTORY!!");
		return 0;
	}
	
	//check if the initial part is even good	
	Printf("ptr: %s", req->ptr);
	Printf("initial cwd: %s", initialCWD);
	uv_fs_req_cleanup(req);
	delete req;
	return 1;
}

void uv8_fs_cb(uv_fs_t* req) {
	if (req->data != nullptr) {
		uv8_cb_handle* fuck = (uv8_cb_handle*)req->data;
		Locker locker(fuck->iso);
		Isolate::Scope(fuck->iso);
		fuck->iso->Enter();
		HandleScope handle_scope(fuck->iso);
		v8::Context::Scope cScope(StrongPersistentTL(_Context));
		StrongPersistentTL(_Context)->Enter();
		Handle<Function> goddammit = Handle<Function>::Cast(StrongPersistentTL(fuck->ref));
		Handle<Value> args[3];
		int argc = 0;
		Handle<ArrayBuffer> abuf;
		Handle<External> aaa;
		if (req->fs_type == UV_FS_READ) {
			abuf = ArrayBuffer::New(fuck->iso, fuck->datum_size);
		}
		if (req->fs_type == UV_FS_SCANDIR) {
			aaa = External::New(fuck->iso, (void*)req);
		}

		switch (req->fs_type) {
		case UV_FS_CLOSE:
		case UV_FS_RENAME:
		case UV_FS_UNLINK:
		case UV_FS_RMDIR:
		case UV_FS_MKDIR:
		case UV_FS_FTRUNCATE:
		case UV_FS_FSYNC:
		case UV_FS_FDATASYNC:
		case UV_FS_LINK:
		case UV_FS_SYMLINK:
		case UV_FS_CHMOD:
		case UV_FS_FCHMOD:
		case UV_FS_CHOWN:
		case UV_FS_FCHOWN:
		case UV_FS_UTIME:
		case UV_FS_FUTIME:
			args[0] = Boolean::New(fuck->iso, true);
			argc++;
			break;

		case UV_FS_OPEN:
		case UV_FS_SENDFILE:
		case UV_FS_WRITE:
			args[0] = Int32::New(fuck->iso, req->result);
			argc++;
			break;

		case UV_FS_STAT:
		case UV_FS_LSTAT:
		case UV_FS_FSTAT:
			args[0] = uv8_fs_get_stats(fuck->iso, &req->statbuf);
			argc++;
			break;

		case UV_FS_READLINK:
			args[0] = String::NewFromUtf8(fuck->iso, (const char*)req->ptr);
			argc++;
			break;

		case UV_FS_MKDTEMP:
			args[0] = String::NewFromUtf8(fuck->iso, req->path);
			argc++;
			break;


		case UV_FS_READ:
			memcpy(abuf->GetContents().Data(), fuck->datum, fuck->datum_size);
			uv8_free(fuck->iso, fuck->datum);
			args[0] = Uint8Array::New(abuf, 0, fuck->datum_size);
			argc++;
			break;

		case UV_FS_SCANDIR:
			args[0] = FunctionTemplate::New(fuck->iso, uv8_fs_scandir_next, aaa)->GetFunction();
			argc++;
			break;

		default:
			ThrowError(fuck->iso, "WTF??");
			return;

		}

		goddammit->Call(StrongPersistentTL(_Context)->Global(), argc, args);

		StrongPersistentTL(_Context)->Exit();
		fuck->iso->Exit();
		Unlocker unlocker(fuck->iso);
		uv_fs_req_cleanup(req);
		delete fuck;
		delete req;
	}
}

uv8_efunc(uv8_fs_close) {
	//ThrowArgsNotVal(2);
	if (args.Length() >= 1) {
		if (!args[0]->IsInt32() || !args[0]->IsNumber()) {
			ThrowBadArg();
		}
	}
	else {
		ThrowError(args.GetIsolate(), "Not enough arguments.");
	}

	int fd = args[0]->Int32Value();
	uv_fs_t* req = new uv_fs_t;
	int ret;
	if (args.Length() == 2) {
		if (!args[1]->IsFunction()) {
			ThrowBadArg();
		}
		Handle<Function> cbfunc = Handle<Function>::Cast(args[1]->ToObject());
		uv8_cb_handle* hand = new uv8_cb_handle;
		hand->argc = 0;
		hand->iso = args.GetIsolate();
		hand->ref.Reset(args.GetIsolate(), cbfunc);
		req->data = (void*)hand;
		ret = uv_fs_close(uv_default_loop(), req, fd, uv8_fs_cb);
	}
	else {
		ret = uv_fs_close(uv_default_loop(), req, fd, nullptr);
		handleReturnFromFS(req, args);
	}
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_open) {
	if (args.Length() < 2) {
		ThrowError(args.GetIsolate(), "Not enough arguments.");
		return;
	}

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	if (!args[1]->IsString()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);

	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	String::Utf8Value c_flags(args[1]->ToString());
	int flags = string_to_flags(args.GetIsolate(), ToCString(c_flags));
	uv_fs_t* req = new uv_fs_t;
	int ret;
	if (args.Length() == 3) {
		if (!args[2]->IsFunction()) {
			ThrowBadArg();

		}
		Handle<Function> cbfunc = Handle<Function>::Cast(args[2]->ToObject());
		uv8_cb_handle* hand = new uv8_cb_handle;
		hand->argc = 0;
		hand->iso = args.GetIsolate();
		hand->ref.Reset(args.GetIsolate(), cbfunc);
		req->data = (void*)hand;
		ret = uv_fs_open(uv_default_loop(), req, path, flags, 0, uv8_fs_cb);
	}
	else {
		ret = uv_fs_open(uv_default_loop(), req, path, flags, 0, nullptr);
		handleReturnFromFS(req, args);
		uv_fs_req_cleanup(req);
	}
	ThrowOnUV(ret);
	//uv8_unfinished_args();
}

uv8_efunc(uv8_fs_read) {
	if (args.Length() < 3) {
		ThrowError(args.GetIsolate(), "Not enough arguments passed");
		return;
	}

	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[1]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[2]->IsNumber() || !args[2]->IsInt32()) {
		ThrowBadArg();
	}

	int fd = args[0]->Int32Value();
	int size = args[1]->Int32Value();
	int offset = args[2]->Int32Value();
	//char* buffer_memory;
	uv_buf_t buf = uv_buf_init((char*)uv8_malloc(args.GetIsolate(), size), size);
	uv_fs_t* req = new uv_fs_t;
	int ret;
	if (args.Length() == 4) {
		if (!args[3]->IsFunction()) {
			ThrowBadArg();
		}
		uv8_cb_handle* hand = new uv8_cb_handle;
		Handle<Function> cbfunc = Handle<Function>::Cast(args[3]->ToObject());
		hand->argc = 0;
		hand->iso = args.GetIsolate();
		hand->ref.Reset(args.GetIsolate(), cbfunc);
		hand->datum = (void*)buf.base;
		hand->datum_size = size;
		req->data = (void*)hand;
		ret = uv_fs_read(uv_default_loop(), req, fd, &buf, 1, offset, uv8_fs_cb);
	}
	else {
		ret = uv_fs_read(uv_default_loop(), req, fd, &buf, 1, offset, nullptr);
		handleReturnFromFS(req, args, (void*)buf.base, size);
		uv_fs_req_cleanup(req);
	}
	ThrowOnUV(ret);
	//uv8_unfinished_args();
}

uv8_efunc(uv8_fs_unlink) {
	if (args.Length() < 1) {
		ThrowError(args.GetIsolate(), "Not enough arguments passed");
		return;
	}

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());

	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	uv_fs_t* req = new uv_fs_t;
	int ret;
	if (args.Length() == 2) {
		if (!args[1]->IsFunction()) {
			ThrowBadArg();
		}
		Handle<Function> cbfunc = Handle<Function>::Cast(args[1]->ToObject());
		uv8_cb_handle* hand = new uv8_cb_handle;
		hand->argc = 0;
		hand->iso = args.GetIsolate();
		hand->ref.Reset(args.GetIsolate(), cbfunc);
		req->data = (void*)hand;
		ret = uv_fs_unlink(uv_default_loop(), req, path, uv8_fs_cb);
	}
	else {
		ret = uv_fs_unlink(uv_default_loop(), req, path, nullptr);
		handleReturnFromFS(req, args);
		uv_fs_req_cleanup(req);
	}
	ThrowOnUV(ret);
	//uv8_unfinished_args();
}

uv8_efunc(uv8_fs_write) {
	if (args.Length() < 3) {
		ThrowError(args.GetIsolate(), "Not enough arguments passed");
		return;
	}
	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[1]->IsUint8Array()) {
		ThrowBadArg();
	}
	if (!args[2]->IsNumber() || !args[2]->IsInt32()) {
		ThrowBadArg();
	}

	int fd = args[0]->Int32Value();
	Handle<Uint8Array> writeout = Handle<Uint8Array>::Cast(args[1]->ToObject());
	Handle<ArrayBuffer> actualstuff = writeout->Buffer();
	int offset = args[2]->Int32Value();
	uv_fs_t* req = new uv_fs_t;
	uv_buf_t buf = uv_buf_init((char*)actualstuff->GetContents().Data(), actualstuff->ByteLength());
	int ret;
	if (args.Length() == 4) {
		if (!args[3]->IsFunction()) {
			ThrowBadArg();
		}
		uv8_cb_handle* hand = new uv8_cb_handle;
		hand->argc = 0;
		hand->iso = args.GetIsolate();
		req->data = (void*)hand;
		Handle<Function> cbfunc = Handle<Function>::Cast(args[3]->ToObject());
		hand->ref.Reset(args.GetIsolate(), cbfunc);
		ret = uv_fs_write(uv_default_loop(), req, fd, &buf, 1, offset, uv8_fs_cb);
	}
	else {
		ret = uv_fs_write(uv_default_loop(), req, fd, &buf, 1, offset, nullptr);
		handleReturnFromFS(req, args);
		uv_fs_req_cleanup(req);
	}
	ThrowOnUV(ret);
	//uv8_unfinished_args();
}

uv8_efunc(uv8_fs_mkdir) {
	if (args.Length() < 1) {
		ThrowError(args.GetIsolate(), "Not enough arguments passed");
		return;
	}

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	uv_fs_t* req = new uv_fs_t;
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	int ret;
	if (args.Length() == 2) {
		if (!args[1]->IsFunction()) {
			ThrowBadArg();
		}
		Handle<Function> cbfunc = Handle<Function>::Cast(args[1]->ToObject());
		uv8_cb_handle* hand = new uv8_cb_handle;
		hand->argc = 0;
		hand->iso = args.GetIsolate();
		hand->ref.Reset(args.GetIsolate(), cbfunc);
		req->data = (void*)hand;
		ret = uv_fs_mkdir(uv_default_loop(), req, path, 0, uv8_fs_cb);
	}
	else {
		ret = uv_fs_mkdir(uv_default_loop(), req, path, 0, nullptr);
		handleReturnFromFS(req, args);
		uv_fs_req_cleanup(req); 
	}
	ThrowOnUV(ret);

}

uv8_efunc(uv8_fs_mkdtemp) {
	ThrowArgsNotVal(2);

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	if (!args[1]->IsFunction()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	Handle<Function> cbfunc = Handle<Function>::Cast(args[1]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_mkdtemp(uv_default_loop(), req, path, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_rmdir) {
	ThrowArgsNotVal(2);

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	if (!args[1]->IsFunction()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	Handle<Function> cbfunc = Handle<Function>::Cast(args[1]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_rmdir(uv_default_loop(), req, path, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_scandir) {
	ThrowArgsNotVal(2);

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	if (!args[1]->IsFunction()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	Handle<Function> cbfunc = Handle<Function>::Cast(args[1]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_scandir(uv_default_loop(), req, path, 0, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_scandir_next) {
	uv_fs_t* req;
	uv_dirent_t* ent = new uv_dirent_t;
	int ret;
	const char* type = "";
	Handle<External> bbb = Handle<External>::Cast(args.Data());
	req = (uv_fs_t*)bbb->Value();
	ret = uv_fs_scandir_next(req, ent);
	if (ret == UV_EOF) {
		delete (uv8_cb_handle*)req->data;
		delete ent;
		uv8_free(args.GetIsolate(), (void*)req);
		//uv_fs_req_cleanup(req);
		args.GetReturnValue().Set(Undefined(args.GetIsolate()));
		return;
	}
	ThrowOnUV(ret);
	Handle<Object> aaa = Object::New(args.GetIsolate());
	aaa->Set(String::NewFromUtf8(args.GetIsolate(), "name"), String::NewFromUtf8(args.GetIsolate(), ent->name));
	switch (ent->type) {
	case UV_DIRENT_UNKNOWN: type = NULL;     break;
	case UV_DIRENT_FILE:    type = "file";   break;
	case UV_DIRENT_DIR:     type = "directory";    break;
	case UV_DIRENT_LINK:    type = "link";   break;
	case UV_DIRENT_FIFO:    type = "fifo";   break;
	case UV_DIRENT_SOCKET:  type = "socket"; break;
	case UV_DIRENT_CHAR:    type = "char";   break;
	case UV_DIRENT_BLOCK:   type = "block";  break;
	}
	if (type)
	{
		aaa->Set(String::NewFromUtf8(args.GetIsolate(), "type"), String::NewFromUtf8(args.GetIsolate(), type));
	}
	delete ent;
	args.GetReturnValue().Set(aaa);
}

uv8_efunc(uv8_fs_stat) {
	if (args.Length() < 1) {
		ThrowError(args.GetIsolate(), "Not enough arguments passed");
		return;
	}
	if (!args[0]->IsString()) {
		ThrowBadArg();
	}

	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	uv_fs_t* req = new uv_fs_t;
	int ret;
	if (args.Length() == 2) {
		if (!args[1]->IsFunction()) {
			ThrowBadArg();
		}
		Handle<Function> cbfunc = Handle<Function>::Cast(args[1]->ToObject());
		uv8_cb_handle* hand = new uv8_cb_handle;
		hand->argc = 0;
		hand->iso = args.GetIsolate();
		hand->ref.Reset(args.GetIsolate(), cbfunc);
		req->data = (void*)hand;
		ret = uv_fs_stat(uv_default_loop(), req, path, uv8_fs_cb);
	}
	else {
		ret = uv_fs_stat(uv_default_loop(), req, path, nullptr);
		handleReturnFromFS(req, args);
		uv_fs_req_cleanup(req);
	}
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_fstat) {
	if (args.Length() < 1) {
		ThrowError(args.GetIsolate(), "Not enough arguments passed");
		return;
	}
	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}
	int fd = args[0]->Int32Value();
	int ret;
	uv_fs_t* req = new uv_fs_t;

	if (args.Length() == 2) {
		if (!args[1]->IsFunction()) {
			ThrowBadArg();
		}
		Handle<Function> cbfunc = Handle<Function>::Cast(args[1]->ToObject());
		uv8_cb_handle* hand = new uv8_cb_handle;
		hand->argc = 0;
		hand->iso = args.GetIsolate();
		hand->ref.Reset(args.GetIsolate(), cbfunc);
		req->data = (void*)hand;
		ret = uv_fs_fstat(uv_default_loop(), req, fd, uv8_fs_cb);
	}
	else {
		ret = uv_fs_fstat(uv_default_loop(), req, fd, nullptr);
		handleReturnFromFS(req, args);
		uv_fs_req_cleanup(req);
	}
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_lstat) {
	if (args.Length() < 1) {
		ThrowError(args.GetIsolate(), "Not enough arguments passed");
		return;
	}
	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	uv_fs_t* req = new uv_fs_t;
	int ret;
	if (args.Length() == 2) {
		if (!args[1]->IsFunction()) {
			ThrowBadArg();
		}
		Handle<Function> cbfunc = Handle<Function>::Cast(args[1]->ToObject());
		uv8_cb_handle* hand = new uv8_cb_handle;
		hand->argc = 0;
		hand->iso = args.GetIsolate();
		hand->ref.Reset(args.GetIsolate(), cbfunc);
		req->data = (void*)hand;
		ret = uv_fs_lstat(uv_default_loop(), req, path, uv8_fs_cb);
	}
	else {
		ret = uv_fs_lstat(uv_default_loop(), req, path, nullptr);
		handleReturnFromFS(req, args);
		uv_fs_req_cleanup(req);
	}
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_rename) {
	ThrowArgsNotVal(3);

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	if (!args[1]->IsString()) {
		ThrowBadArg();
	}
	if (!args[2]->IsFunction()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	String::Utf8Value c_newpath(args[1]->ToString());
	const char* newpath = ToCString(c_newpath);
	if (!grabTheShit(newpath)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	Handle<Function> cbfunc = Handle<Function>::Cast(args[2]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_rename(uv_default_loop(), req, path, newpath, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_fsync) {
	ThrowArgsNotVal(2);

	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[1]->IsFunction()) {
		ThrowBadArg();
	}
	int fd = args[0]->Int32Value();
	Handle<Function> cbfunc = Handle<Function>::Cast(args[1]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_fsync(uv_default_loop(), req, fd, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_fdatasync) {
	ThrowArgsNotVal(2);

	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[1]->IsFunction()) {
		ThrowBadArg();
	}
	int fd = args[0]->Int32Value();
	Handle<Function> cbfunc = Handle<Function>::Cast(args[1]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_fdatasync(uv_default_loop(), req, fd, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_ftruncate) {
	ThrowArgsNotVal(2);

	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[1]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[2]->IsFunction()) {
		ThrowBadArg();
	}
	int fd = args[0]->Int32Value();
	int offset = args[1]->Int32Value();
	Handle<Function> cbfunc = Handle<Function>::Cast(args[2]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_ftruncate(uv_default_loop(), req, fd, offset, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_sendfile) {
	ThrowArgsNotVal(5);

	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[1]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[2]->IsNumber() || !args[2]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[3]->IsNumber() || !args[3]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[4]->IsFunction()) {
		ThrowBadArg();
	}
	int fd = args[0]->Int32Value();
	int out_fd = args[1]->Int32Value();
	int in_offset = args[2]->Int32Value();
	int length = args[3]->Int32Value();

	Handle<Function> cbfunc = Handle<Function>::Cast(args[4]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_sendfile(uv_default_loop(), req, fd, out_fd, in_offset, length, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_access) {
	ThrowArgsNotVal(3);

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	if (!args[1]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[2]->IsFunction()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	int flags = args[1]->Int32Value();
	Handle<Function> cbfunc = Handle<Function>::Cast(args[2]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_access(uv_default_loop(), req, path, flags, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_chmod) {
	if (args.Length() < 2) {
		ThrowError(args.GetIsolate(), "Not enough arguments passed");
		return;
	}
	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	if (!args[1]->IsNumber()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	int flags = args[1]->Int32Value();
	int ret;
	uv_fs_t* req = new uv_fs_t;

	if (args.Length() == 3) {
		if (!args[2]->IsFunction()) {
			ThrowBadArg();
		}
		Handle<Function> cbfunc = Handle<Function>::Cast(args[2]->ToObject());
		uv8_cb_handle* hand = new uv8_cb_handle;
		hand->argc = 0;
		hand->iso = args.GetIsolate();
		hand->ref.Reset(args.GetIsolate(), cbfunc);
		req->data = (void*)hand;
		ret = uv_fs_chmod(uv_default_loop(), req, path, flags, uv8_fs_cb);
	}
	else {
		ret = uv_fs_chmod(uv_default_loop(), req, path, flags, nullptr);
		handleReturnFromFS(req, args);
		uv_fs_req_cleanup(req);
	}
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_fchmod) {
	if (args.Length() < 2) {
		ThrowError(args.GetIsolate(), "Not enough arguments passed");
		return;
	}
	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[1]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	int fd = args[0]->Int32Value();
	int mode = args[1]->Int32Value();
	int ret;
	uv_fs_t* req = new uv_fs_t;
	if (args.Length() == 3) {
		if (!args[2]->IsFunction()) {
			ThrowBadArg();
		}
		Handle<Function> cbfunc = Handle<Function>::Cast(args[2]->ToObject());
		uv8_cb_handle* hand = new uv8_cb_handle;
		hand->argc = 0;
		hand->iso = args.GetIsolate();
		hand->ref.Reset(args.GetIsolate(), cbfunc);
		req->data = (void*)hand;
		ret = uv_fs_fchmod(uv_default_loop(), req, fd, mode, uv8_fs_cb);
	}
	else {
		ret = uv_fs_fchmod(uv_default_loop(), req, fd, mode, nullptr);
		handleReturnFromFS(req, args);
		uv_fs_req_cleanup(req);
	}

	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_utime) {
	ThrowArgsNotVal(4);

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	if (!args[1]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[2]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[3]->IsFunction()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	double atime = args[1]->NumberValue();
	double mtime = args[2]->NumberValue();
	Handle<Function> cbfunc = Handle<Function>::Cast(args[3]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_utime(uv_default_loop(), req, path, atime, mtime, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_futime) {
	ThrowArgsNotVal(4);

	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[1]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[2]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[3]->IsFunction()) {
		ThrowBadArg();
	}
	int fd = args[0]->Int32Value();
	double atime = args[1]->NumberValue();
	double mtime = args[2]->NumberValue();
	Handle<Function> cbfunc = Handle<Function>::Cast(args[3]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_futime(uv_default_loop(), req, fd, atime, mtime, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_link) {
	ThrowArgsNotVal(3);

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	if (!args[1]->IsString()) {
		ThrowBadArg();
	}
	if (!args[2]->IsFunction()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	String::Utf8Value c_newpath(args[1]->ToString());
	const char* newpath = ToCString(c_newpath);
	if (!grabTheShit(newpath)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	Handle<Function> cbfunc = Handle<Function>::Cast(args[2]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_link(uv_default_loop(), req, path, newpath, uv8_fs_cb);
	ThrowOnUV(ret);
}
uv8_efunc(uv8_fs_symlink) {
	ThrowArgsNotVal(4);

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	if (!args[1]->IsString()) {
		ThrowBadArg();
	}
	if (!args[2]->IsString()) {
		ThrowBadArg();
	}
	if (!args[3]->IsFunction()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	String::Utf8Value c_newpath(args[1]->ToString());
	const char* newpath = ToCString(c_newpath);
	if (!grabTheShit(newpath)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	String::Utf8Value c_flags(args[2]->ToString());
	int flags = string_to_flags(args.GetIsolate(), *c_flags);
	Handle<Function> cbfunc = Handle<Function>::Cast(args[3]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_symlink(uv_default_loop(), req, path, newpath, flags, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_readlink) {
	ThrowArgsNotVal(2);

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	if (!args[1]->IsFunction()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	Handle<Function> cbfunc = Handle<Function>::Cast(args[1]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_readlink(uv_default_loop(), req, path, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_chown) {
	ThrowArgsNotVal(4);

	if (!args[0]->IsString()) {
		ThrowBadArg();
	}
	if (!args[1]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[2]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[3]->IsFunction()) {
		ThrowBadArg();
	}
	String::Utf8Value c_path(args[0]->ToString());
	const char* path = ToCString(c_path);
	if (!grabTheShit(path)) {
		ThrowError(args.GetIsolate(), "Operation is not within the sandbox.");
		return;
	}
	int uid = args[1]->Int32Value();
	int gid = args[1]->Int32Value();
	Handle<Function> cbfunc = Handle<Function>::Cast(args[3]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_chown(uv_default_loop(), req, path, uid, gid, uv8_fs_cb);
	ThrowOnUV(ret);
}

uv8_efunc(uv8_fs_fchown) {
	ThrowArgsNotVal(4);

	if (!args[0]->IsNumber() || !args[0]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[1]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[2]->IsNumber() || !args[1]->IsInt32()) {
		ThrowBadArg();
	}
	if (!args[3]->IsFunction()) {
		ThrowBadArg();
	}
	int fd = args[0]->Int32Value();
	int uid = args[1]->Int32Value();
	int gid = args[1]->Int32Value();
	Handle<Function> cbfunc = Handle<Function>::Cast(args[3]->ToObject());
	uv_fs_t* req = new uv_fs_t;
	uv8_cb_handle* hand = new uv8_cb_handle;
	hand->argc = 0;
	hand->iso = args.GetIsolate();
	hand->ref.Reset(args.GetIsolate(), cbfunc);
	req->data = (void*)hand;
	int ret = uv_fs_fchown(uv_default_loop(), req, fd, uid, gid, uv8_fs_cb);
	ThrowOnUV(ret);
}

Handle<ObjectTemplate> uv8_bind_fs(Isolate* this_){
	Handle<ObjectTemplate> fs = ObjectTemplate::New(this_);
	fs->Set(this_, "close", FunctionTemplate::New(this_, uv8_fs_close));
	fs->Set(this_, "open", FunctionTemplate::New(this_, uv8_fs_open));
	fs->Set(this_, "read", FunctionTemplate::New(this_, uv8_fs_read));
	fs->Set(this_, "unlink", FunctionTemplate::New(this_, uv8_fs_unlink));
	fs->Set(this_, "write", FunctionTemplate::New(this_, uv8_fs_write));
	fs->Set(this_, "mkdir", FunctionTemplate::New(this_, uv8_fs_mkdir));
	fs->Set(this_, "mkdtemp", FunctionTemplate::New(this_, uv8_fs_mkdtemp));
	fs->Set(this_, "rmdir", FunctionTemplate::New(this_, uv8_fs_rmdir));
	fs->Set(this_, "scandir", FunctionTemplate::New(this_, uv8_fs_scandir));
	//fs->Set(this_, "scandir_next", FunctionTemplate::New(this_, uv8_fs_scandir_next));
	fs->Set(this_, "stat", FunctionTemplate::New(this_, uv8_fs_stat));
	fs->Set(this_, "fstat", FunctionTemplate::New(this_, uv8_fs_fstat));
	fs->Set(this_, "lstat", FunctionTemplate::New(this_, uv8_fs_lstat));
	fs->Set(this_, "rename", FunctionTemplate::New(this_, uv8_fs_rename));
	fs->Set(this_, "fsync", FunctionTemplate::New(this_, uv8_fs_fsync));
	fs->Set(this_, "fdatasync", FunctionTemplate::New(this_, uv8_fs_fdatasync));
	fs->Set(this_, "ftruncate", FunctionTemplate::New(this_, uv8_fs_ftruncate));
	fs->Set(this_, "sendfile", FunctionTemplate::New(this_, uv8_fs_sendfile));
	fs->Set(this_, "access", FunctionTemplate::New(this_, uv8_fs_access));
	fs->Set(this_, "chmod", FunctionTemplate::New(this_, uv8_fs_chmod));
	fs->Set(this_, "fchmod", FunctionTemplate::New(this_, uv8_fs_fchmod));
	fs->Set(this_, "utime", FunctionTemplate::New(this_, uv8_fs_utime));
	fs->Set(this_, "futime", FunctionTemplate::New(this_, uv8_fs_futime));
	fs->Set(this_, "link", FunctionTemplate::New(this_, uv8_fs_link));
	fs->Set(this_, "symlink", FunctionTemplate::New(this_, uv8_fs_symlink));
	fs->Set(this_, "readlink", FunctionTemplate::New(this_, uv8_fs_readlink));
	fs->Set(this_, "chown", FunctionTemplate::New(this_, uv8_fs_chown));
	fs->Set(this_, "fchown", FunctionTemplate::New(this_, uv8_fs_fchown));
	size_t sz = _MAX_PATH * 2;
	uv_cwd((char*)initialCWD, &sz);
	return fs;
}