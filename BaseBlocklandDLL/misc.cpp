#include "uv8.h"

using namespace v8;

uv8_efunc(uv8_guess_handle) {
	uv8_unfinished_args();
}

uv8_efunc(uv8_version) {
	args.GetReturnValue().Set(Integer::New(args.GetIsolate(), uv_version()));
	return;
}

uv8_efunc(uv8_version_string) {
	args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), uv_version_string()));
	return;
}

uv8_efunc(uv8_get_process_title) {
	char procTitle[1024];
	uv_get_process_title(procTitle, 1024);
	args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), procTitle));
	return;
}

uv8_efunc(uv8_set_process_title) {
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	if (args.Length() != 1) {
		ThrowError(args.GetIsolate(), "Wrong number of arguments");
		return;
	}
	if (!args[0]->IsString()) {
		ThrowError(args.GetIsolate(), "Wrong type of arguments");
		return;
	}
	uv_set_process_title(ToCString(String::Utf8Value(args[0]->ToString())));
	return;
}

uv8_efunc(uv8_resident_set_memory) {
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	size_t residentsize;
	uv_resident_set_memory(&residentsize);
	args.GetReturnValue().Set(Number::New(args.GetIsolate(), residentsize));
	return;
}

uv8_efunc(uv8_uptime) {
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	double uptime;
	uv_uptime(&uptime);
	args.GetReturnValue().Set(Number::New(args.GetIsolate(), uptime));
	return;
}

uv8_efunc(uv8_getrusage) {
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	uv_rusage_t rusage;
	uv_getrusage(&rusage);
	Handle<Object> ru = Object::New(args.GetIsolate());
	Handle<Object> time1 = Object::New(args.GetIsolate());
	time1->Set(String::NewFromUtf8(args.GetIsolate(), "usec"), Int32::NewFromUnsigned(args.GetIsolate(), rusage.ru_utime.tv_usec));
	time1->Set(String::NewFromUtf8(args.GetIsolate(), "sec"), Int32::NewFromUnsigned(args.GetIsolate(), rusage.ru_utime.tv_sec));
	Handle<Object> time2 = Object::New(args.GetIsolate());
	time2->Set(String::NewFromUtf8(args.GetIsolate(), "usec"), Int32::NewFromUnsigned(args.GetIsolate(), rusage.ru_stime.tv_usec));
	time2->Set(String::NewFromUtf8(args.GetIsolate(), "sec"), Int32::NewFromUnsigned(args.GetIsolate(), rusage.ru_stime.tv_sec));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "utime"), time1);
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "stime"), time2);
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "maxrss"), convert64To32(args.GetIsolate(), rusage.ru_maxrss));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "ixrss"), convert64To32(args.GetIsolate(), rusage.ru_ixrss));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "idrss"), convert64To32(args.GetIsolate(), rusage.ru_idrss));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "isrss"), convert64To32(args.GetIsolate(), rusage.ru_isrss));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "minflt"), convert64To32(args.GetIsolate(), rusage.ru_minflt));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "majflt"), convert64To32(args.GetIsolate(), rusage.ru_majflt));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "nswap"), convert64To32(args.GetIsolate(), rusage.ru_nswap));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "inblock"), convert64To32(args.GetIsolate(), rusage.ru_inblock));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "oublock"), convert64To32(args.GetIsolate(), rusage.ru_oublock));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "msgsnd"), convert64To32(args.GetIsolate(), rusage.ru_msgsnd));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "msgrcv"), convert64To32(args.GetIsolate(), rusage.ru_msgrcv));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "nsignals"), convert64To32(args.GetIsolate(), rusage.ru_nsignals));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "nvcsw"), convert64To32(args.GetIsolate(), rusage.ru_nvcsw));
	ru->Set(String::NewFromUtf8(args.GetIsolate(), "nivcsw"), convert64To32(args.GetIsolate(), rusage.ru_nivcsw));
	args.GetReturnValue().Set(ru);
	return;
}

uv8_efunc(uv8_cpu_info) {
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	uv_cpu_info_t* cpus;
	int count;
	uv_cpu_info(&cpus, &count);
	Handle<Array> ee = Array::New(args.GetIsolate(), count);
	for (int i = 0; i < count; i++) {
		Handle<Object> acpu = Object::New(args.GetIsolate());
		acpu->Set(String::NewFromUtf8(args.GetIsolate(), "model"), String::NewFromUtf8(args.GetIsolate(), cpus[i].model));
		acpu->Set(String::NewFromUtf8(args.GetIsolate(), "speed"), Int32::New(args.GetIsolate(), cpus[i].speed));
		Handle<Object> times = Object::New(args.GetIsolate());
		times->Set(String::NewFromUtf8(args.GetIsolate(), "user"), convert64To32(args.GetIsolate(), cpus[i].cpu_times.user));
		times->Set(String::NewFromUtf8(args.GetIsolate(), "nice"), convert64To32(args.GetIsolate(), cpus[i].cpu_times.nice));
		times->Set(String::NewFromUtf8(args.GetIsolate(), "sys"), convert64To32(args.GetIsolate(), cpus[i].cpu_times.sys));
		times->Set(String::NewFromUtf8(args.GetIsolate(), "idle"), convert64To32(args.GetIsolate(), cpus[i].cpu_times.idle));
		times->Set(String::NewFromUtf8(args.GetIsolate(), "irq"), convert64To32(args.GetIsolate(), cpus[i].cpu_times.irq));
		acpu->Set(String::NewFromUtf8(args.GetIsolate(), "times"), times);
		ee->Set(i, acpu);
	}
	uv_free_cpu_info(cpus, count);
	args.GetReturnValue().Set(ee);

	return;
}

uv8_efunc(uv8_interface_addresses) {
	uv_interface_address_t* ifaces;
	char addr[INET6_ADDRSTRLEN];
	int count;
	uv_interface_addresses(&ifaces, &count);
	Handle<Array> bleh = Array::New(args.GetIsolate(), count);
	for (int i = 0; i < count; i++) {
		Handle<Object> holder = Object::New(args.GetIsolate());
		holder->Set(String::NewFromUtf8(args.GetIsolate(), "name"), String::NewFromUtf8(args.GetIsolate(), ifaces[i].name));
		holder->Set(String::NewFromUtf8(args.GetIsolate(), "internal"), Boolean::New(args.GetIsolate(), ifaces[i].is_internal));
		const char* protostring;
		if (ifaces[i].address.address4.sin_family == AF_INET)
		{
			protostring = "ipv4";
			uv_ip4_name(&ifaces[i].address.address4, addr, sizeof(addr));
		}
		else if (ifaces[i].address.address6.sin6_family == AF_INET6) {
			protostring = "ipv6";
			uv_ip6_name(&ifaces[i].address.address6, addr, sizeof(addr));
		}
		else {
			protostring = "unknown";
			strncpy(addr, "<uknown addr family>", sizeof(addr));
		}
		holder->Set(String::NewFromUtf8(args.GetIsolate(), "address"), String::NewFromUtf8(args.GetIsolate(), addr));
		holder->Set(String::NewFromUtf8(args.GetIsolate(), "protocol"), String::NewFromUtf8(args.GetIsolate(), protostring));
		bleh->Set(i, holder);
	}
	uv_free_interface_addresses(ifaces, count);
	args.GetReturnValue().Set(bleh);
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

uv8_efunc(uv8_loadavg) {
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	double average[3];
	uv_loadavg(average);
	Handle<Array> avg = Array::New(args.GetIsolate(), 3);
	for (int i = 0; i < 3; i++) {
		avg->Set(i, Number::New(args.GetIsolate(), average[i]));
	}
	args.GetReturnValue().Set(avg);
	return;
}

uv8_efunc(uv8_exepath) {
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	size_t sz = _MAX_PATH * 2;
	char path[_MAX_PATH * 2];
	if (uv_exepath(path, &sz) != UV_ENOBUFS)
	{
		args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), path));
	}
	else {
		args.GetReturnValue().SetEmptyString();
	}
	return;
}

uv8_efunc(uv8_cwd) {
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	size_t sz = _MAX_PATH * 2;
	char path[_MAX_PATH * 2];
	if (uv_cwd(path, &sz) != UV_ENOBUFS) {
		args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), path));
	}
	else {
		args.GetReturnValue().SetEmptyString();
	}
	return;
}

uv8_efunc(uv8_os_homedir) {
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	size_t sz = _MAX_PATH * 2;
	char path[_MAX_PATH * 2];
	if (uv_os_homedir(path, &sz) != UV_ENOBUFS) {
		args.GetReturnValue().Set(String::NewFromUtf8(args.GetIsolate(), path));
	}
	else {
		args.GetReturnValue().SetEmptyString();
	}
	return;
}

uv8_efunc(uv8_chdir) {
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	if (!args[0]->IsString()) {
		ThrowError(args.GetIsolate(), "Expected string as first argument");
		return;
	}
	args.GetReturnValue().Set(Integer::New(args.GetIsolate(), uv_chdir(ToCString(String::Utf8Value(args[0]->ToString())))));
	return;
}

uv8_efunc(uv8_get_total_memory) {
	args.GetReturnValue().Set(convert64To32(args.GetIsolate(), uv_get_total_memory()));
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

uv8_efunc(uv8_hrtime) {
	Handle<Array> bleh = convert64To32(args.GetIsolate(), uv_hrtime());
	args.GetReturnValue().Set(bleh);
	//args.GetReturnValue().Set(Int32::NewFromUnsigned(args.GetIsolate(), uv_hrtime()));
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

uv8_efunc(uv8_update_time) {
	uv_update_time(uv_default_loop());
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

uv8_efunc(uv8_now) {
	uv_update_time(uv_default_loop());
	uint64_t highprec = uv_now(uv_default_loop());
	Handle<Array> bleh = convert64To32(args.GetIsolate(), highprec);
	args.GetReturnValue().Set(bleh);
	//args.GetIsolate()->ThrowException(String::NewFromUtf8(args.GetIsolate(), "Unfinished"));
	return;
}

Handle<ObjectTemplate> uv8_bind_misc(Isolate* this_) {
	Handle<ObjectTemplate> misc = ObjectTemplate::New(this_);
	misc->Set(this_, "guess_handle", FunctionTemplate::New(this_, uv8_guess_handle));
	misc->Set(this_, "version", FunctionTemplate::New(this_, uv8_version));
	misc->Set(this_, "version_string", FunctionTemplate::New(this_, uv8_version_string));
	misc->Set(this_, "get_process_title", FunctionTemplate::New(this_, uv8_get_process_title));
	misc->Set(this_, "set_process_title", FunctionTemplate::New(this_, uv8_set_process_title));
	misc->Set(this_, "resident_set_memory", FunctionTemplate::New(this_, uv8_resident_set_memory));
	misc->Set(this_, "uptime", FunctionTemplate::New(this_, uv8_uptime));
	misc->Set(this_, "getrusage", FunctionTemplate::New(this_, uv8_getrusage));
	misc->Set(this_, "cpu_info", FunctionTemplate::New(this_, uv8_cpu_info));
	misc->Set(this_, "interface_addresses", FunctionTemplate::New(this_, uv8_interface_addresses));
	misc->Set(this_, "loadavg", FunctionTemplate::New(this_, uv8_loadavg));
	misc->Set(this_, "exepath", FunctionTemplate::New(this_, uv8_exepath));
	misc->Set(this_, "cwd", FunctionTemplate::New(this_, uv8_cwd));
	misc->Set(this_, "os_homedir", FunctionTemplate::New(this_, uv8_os_homedir));
	misc->Set(this_, "get_total_memory", FunctionTemplate::New(this_, uv8_get_total_memory));
	misc->Set(this_, "hrtime", FunctionTemplate::New(this_, uv8_hrtime));
	misc->Set(this_, "update_time", FunctionTemplate::New(this_, uv8_update_time));
	misc->Set(this_, "now", FunctionTemplate::New(this_, uv8_now));
	return misc;
}