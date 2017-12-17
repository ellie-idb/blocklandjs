#include "duktape.h"
#include "blah.h"
extern "C" {
#include "duv.h"
#include "refs.h"
#include "utils.h"
#include "loop.h"
#include "req.h"
#include "handle.h"
#include "timer.h"
#include "stream.h"
#include "tcp.h"
#include "pipe.h"
#include "tty.h"
#include "fs.h"
#include "misc.h"
#include "miniz.h"
}

static const duk_function_list_entry duv_funcs[] = {
  // loop.c
  {"run", duv_run, 0},
  {"walk", duv_walk, 1},

  // req.c
  {"cancel", duv_cancel, 1},

  // handle.c
  {"close", duv_close, 2},

  // timer.c
  {"new_timer", duv_new_timer, 0},
  {"timer_start", duv_timer_start, 4},
  {"timer_stop", duv_timer_stop, 1},
  {"timer_again", duv_timer_again, 1},
  {"timer_set_repeat", duv_timer_set_repeat, 2},
  {"timer_get_repeat", duv_timer_get_repeat, 1},

  // stream.c
  {"shutdown", duv_shutdown, 2},
  {"listen", duv_listen, 3},
  {"accept", duv_accept, 2},
  {"read_start", duv_read_start, 2},
  {"read_stop", duv_read_stop, 1},
  {"write", duv_write, 3},
  {"is_readable", duv_is_readable, 1},
  {"is_writable", duv_is_writable, 1},
  {"stream_set_blocking", duv_stream_set_blocking, 2},

  // tcp.c
  {"new_tcp", duv_new_tcp, 0},
  {"tcp_open", duv_tcp_open, 2},
  {"tcp_nodelay", duv_tcp_nodelay, 2},
  {"tcp_keepalive", duv_tcp_keepalive, 3},
  {"tcp_simultaneous_accepts", duv_tcp_simultaneous_accepts, 2},
  {"tcp_bind", duv_tcp_bind, 3},
  {"tcp_getpeername", duv_tcp_getpeername, 1},
  {"tcp_getsockname", duv_tcp_getsockname, 1},
  {"tcp_connect", duv_tcp_connect, 4},

  // pipe.c
  {"new_pipe", duv_new_pipe, 1},
  {"pipe_open", duv_pipe_open, 2},
  {"pipe_bind", duv_pipe_bind, 2},
  {"pipe_connect", duv_pipe_connect, 3},
  {"pipe_getsockname", duv_pipe_getsockname, 1},
  {"pipe_pending_instances", duv_pipe_pending_instances, 2},
  {"pipe_pending_count", duv_pipe_pending_count, 1},
  {"pipe_pending_type", duv_pipe_pending_type, 1},

  // tty.c
  {"new_tty", duv_new_tty, 2},
  {"tty_set_mode", duv_tty_set_mode, 2},
  {"tty_reset_mode", duv_tty_reset_mode, 0},
  {"tty_get_winsize", duv_tty_get_winsize, 1},

  // fs.c
  {"fs_close", duv_fs_close, 2},
  {"fs_open", duv_fs_open, 4},
  {"fs_read", duv_fs_read, 4},
  {"fs_unlink", duv_fs_unlink, 2},
  {"fs_write", duv_fs_write, 4},
  {"fs_mkdir", duv_fs_mkdir, 3},
  {"fs_mkdtemp", duv_fs_mkdtemp, 2},
  {"fs_rmdir", duv_fs_rmdir, 2},
  {"fs_scandir", duv_fs_scandir, 2},
  {"fs_scandir_next", duv_fs_scandir_next, 1},
  {"fs_stat", duv_fs_stat, 2},
  {"fs_fstat", duv_fs_fstat, 2},
  {"fs_lstat", duv_fs_lstat, 2},
  {"fs_rename", duv_fs_rename, 3},
  {"fs_fsync", duv_fs_fsync, 2},
  {"fs_fdatasync", duv_fs_fdatasync, 2},
  {"fs_ftruncate", duv_fs_ftruncate, 3},
  {"fs_sendfile", duv_fs_sendfile, 5},
  {"fs_access", duv_fs_access, 3},
  {"fs_chmod", duv_fs_chmod, 3},
  {"fs_fchmod", duv_fs_fchmod, 3},
  {"fs_utime", duv_fs_utime, 4},
  {"fs_futime", duv_fs_futime, 4},
  {"fs_link", duv_fs_link, 3},
  {"fs_symlink", duv_fs_symlink, 4},
  {"fs_readlink", duv_fs_readlink, 2},
  {"fs_chown", duv_fs_chown, 4},
  {"fs_fchown", duv_fs_fchown, 4},

  // misc.c
  {"guess_handle", duv_guess_handle, 1},
  {"version", duv_version, 0},
  {"version_string", duv_version_string, 0},
  {"get_process_title", duv_get_process_title, 0},
  {"set_process_title", duv_set_process_title, 1},
  {"resident_set_memory", duv_resident_set_memory, 0},
  {"uptime", duv_uptime, 0},
  {"getrusage", duv_getrusage, 0},
  {"cpu_info", duv_cpu_info, 0},
  {"interface_addresses", duv_interface_addresses, 0},
  {"loadavg", duv_loadavg, 0},
  {"exepath", duv_exepath, 0},
  {"cwd", duv_cwd, 0},
  {"os_homedir", duv_os_homedir, 0},
  {"chdir", duv_chdir, 1},
  {"get_total_memory", duv_get_total_memory, 0},
  {"hrtime", duv_hrtime, 0},
  {"update_time", duv_update_time, 0},
  {"now", duv_now, 0},
  {"argv", duv_argv, 0},

  // miniz.c
  {"inflate", duv_tinfl, 2},
  {"deflate", duv_tdefl, 2},

  {NULL, NULL, 0},
};

duk_ret_t duv_cwd(duk_context *ctx) {
  size_t size = 2*PATH_MAX;
  char path[2*PATH_MAX];
  duv_check(ctx, uv_cwd(path, &size));
  duk_push_lstring(ctx, path, size);
  return 1;
}


duk_ret_t dukopen_uv(duk_context *ctx) {

  duv_ref_setup(ctx);

  // Create a uv table on the global
  duk_push_object(ctx);
  duk_put_function_list(ctx, -1, duv_funcs);
  return 1;
}

duk_bool_t dschema_is_data(duk_context* ctx, duk_idx_t index) {
  return duk_is_string(ctx, index) || duk_is_buffer(ctx, index);
}

duk_bool_t dschema_is_continuation(duk_context* ctx, duk_idx_t index) {
  return !duk_is_valid_index(ctx, index) ||
          duk_is_function(ctx, index) ||
          duk_is_undefined(ctx, index);
}

void dschema_check(duk_context *ctx, const duv_schema_entry schema[]) {
  int i;
  int top = duk_get_top(ctx);
  for (i = 0; schema[i].name; ++i) {
    // printf("Checking arg %d-%s\n", i, schema[i].name);
    if (schema[i].checker(ctx, i)) continue;
    duk_error(ctx, DUK_ERR_TYPE_ERROR, "Invalid argument type for %s", schema[i].name);
  }
  if (top > i) {
    duk_error(ctx, DUK_ERR_TYPE_ERROR, "Too many arguments. Expected at %d, but got %d", i, top);
  }
}

static duk_ret_t duv_loadfile(duk_context *ctx) {
  const char* path = duk_require_string(ctx, 0);
  uv_fs_t req;
  int fd = 0;
  uint64_t size;
  char* chunk;
  uv_buf_t buf;

  if (uv_fs_open(&loop, &req, path, O_RDONLY, 0644, NULL) < 0) goto fail;
  uv_fs_req_cleanup(&req);
  fd = req.result;
  if (uv_fs_fstat(&loop, &req, fd, NULL) < 0) goto fail;
  uv_fs_req_cleanup(&req);
  size = req.statbuf.st_size;
  chunk = (char*)duk_alloc(ctx, size);
  buf = uv_buf_init(chunk, size);
  if (uv_fs_read(&loop, &req, fd, &buf, 1, 0, NULL) < 0) {
    duk_free(ctx, chunk);
    goto fail;
  }
  uv_fs_req_cleanup(&req);
  duk_push_lstring(ctx, chunk, size);
  duk_free(ctx, chunk);
  uv_fs_close(&loop, &req, fd, NULL);
  uv_fs_req_cleanup(&req);

  return 1;

  fail:
  uv_fs_req_cleanup(&req);
  if (fd) uv_fs_close(&loop, &req, fd, NULL);
  uv_fs_req_cleanup(&req);
  duk_error(ctx, DUK_ERR_ERROR, "%s: %s: %s", uv_err_name(req.result), uv_strerror(req.result), path);
}

static duv_list_t* duv_list_node(const char* part, int start, int end, duv_list_t* next) {
  duv_list_t *node = (duv_list_t*)malloc(sizeof(*node));
  node->part = part;
  node->offset = start;
  node->length = end - start;
  node->next = next;
  return node;
}

static duk_ret_t duv_path_join(duk_context *ctx) {
  duv_list_t *list = NULL;
  int absolute = 0;

  // Walk through all the args and split into a linked list
  // of segments
  {
    // Scan backwards looking for the the last absolute positioned path.
    int top = duk_get_top(ctx);
    int i = top - 1;
    while (i > 0) {
      const char* part = duk_require_string(ctx, i);
      if (part[0] == '/') break;
      i--;
    }
    for (; i < top; ++i) {
      const char* part = duk_require_string(ctx, i);
      int j;
      int start = 0;
      int length = strlen(part);
      if (part[0] == '/') {
        absolute = 1;
      }
      while (start < length && part[start] == 0x2f) { ++start; }
      for (j = start; j < length; ++j) {
        if (part[j] == 0x2f) {
          if (start < j) {
            list = duv_list_node(part, start, j, list);
            start = j;
            while (start < length && part[start] == 0x2f) { ++start; }
          }
        }
      }
      if (start < j) {
        list = duv_list_node(part, start, j, list);
      }
    }
  }

  // Run through the list in reverse evaluating "." and ".." segments.
  {
    int skip = 0;
    duv_list_t *prev = NULL;
    while (list) {
      duv_list_t *node = list;

      // Ignore segments with "."
      if (node->length == 1 &&
          node->part[node->offset] == 0x2e) {
        goto skip;
      }

      // Ignore segments with ".." and grow the skip count
      if (node->length == 2 &&
          node->part[node->offset] == 0x2e &&
          node->part[node->offset + 1] == 0x2e) {
        ++skip;
        goto skip;
      }

      // Consume the skip count
      if (skip > 0) {
        --skip;
        goto skip;
      }

      list = node->next;
      node->next = prev;
      prev = node;
      continue;

      skip:
        list = node->next;
        free(node);
    }
    list = prev;
  }

  // Merge the list into a single `/` delimited string.
  // Free the remaining list nodes.
  {
    int count = 0;
    if (absolute) {
      duk_push_string(ctx, "/");
      ++count;
    }
    while (list) {
      duv_list_t *node = list;
      duk_push_lstring(ctx, node->part + node->offset, node->length);
      ++count;
      if (node->next) {
        duk_push_string(ctx, "/");
        ++count;
      }
      list = node->next;
      free(node);
    }
    duk_concat(ctx, count);
  }
  return 1;
}

static duk_ret_t duv_require(duk_context *ctx) {
  int is_main = 0;

  dschema_check(ctx, (const duv_schema_entry[]) {
    {"id", duk_is_string},
    {NULL}
  });

  // push Duktape
  duk_get_global_string(ctx, "Duktape");

  // id = Duktape.modResolve(this, id);
  duk_get_prop_string(ctx, -1, "modResolve");
  duk_push_this(ctx);
  {
    // Check if we're in main
    duk_get_prop_string(ctx, -1, "exports");
    if (duk_is_undefined(ctx, -1)) { is_main = 1; }
    duk_pop(ctx);
  }
  duk_dup(ctx, 0);
  duk_call_method(ctx, 1);
  duk_replace(ctx, 0);

  // push Duktape.modLoaded
  duk_get_prop_string(ctx, -1, "modLoaded");

  // push Duktape.modLoaded[id];
  duk_dup(ctx, 0);
  duk_get_prop(ctx, -2);

  // if (typeof Duktape.modLoaded[id] === 'object') {
  //   return Duktape.modLoaded[id].exports;
  // }
  if (duk_is_object(ctx, -1)) {
    duk_get_prop_string(ctx, -1, "exports");
    return 1;
  }

  // pop Duktape.modLoaded[id]
  duk_pop(ctx);

  // push module = { id: id, exports: {} }
  duk_push_object(ctx);
  duk_dup(ctx, 0);
  duk_put_prop_string(ctx, -2, "id");
  duk_push_object(ctx);
  duk_put_prop_string(ctx, -2, "exports");

  // Set module.main = true if we're the first script
  if (is_main) {
    duk_push_boolean(ctx, 1);
    duk_put_prop_string(ctx, -2, "main");
  }

  // Or set module.parent = parent if we're a child.
  else {
    duk_push_this(ctx);
    duk_put_prop_string(ctx, -2, "parent");
  }

  // Set the prototype for the module to access require.
  duk_push_global_stash(ctx);
  duk_get_prop_string(ctx, -1, "modulePrototype");
  duk_set_prototype(ctx, -3);
  duk_pop(ctx);

  // Duktape.modLoaded[id] = module
  duk_dup(ctx, 0);
  duk_dup(ctx, -2);
  duk_put_prop(ctx, -4);

  // remove Duktape.modLoaded
  duk_remove(ctx, -2);

  // push Duktape.modLoad(module)
  duk_get_prop_string(ctx, -2, "modLoad");
  duk_dup(ctx, -2);
  duk_call_method(ctx, 0);

  // if ret !== undefined module.exports = ret;
  if (duk_is_undefined(ctx, -1)) {
    duk_pop(ctx);
  }
  else {
    duk_put_prop_string(ctx, -2, "exports");
  }

  duk_get_prop_string(ctx, -1, "exports");

  return 1;
}

// Default implementation for modResolve
// Duktape.modResolve = function (parent, id) {
//   return pathJoin(parent.id, "..", id);
// };
static duk_ret_t duv_mod_resolve(duk_context *ctx) {
  dschema_check(ctx, (const duv_schema_entry[]) {
    {"id", duk_is_string},
    {NULL}
  });

  duk_push_this(ctx);
  duk_push_c_function(ctx, duv_path_join, DUK_VARARGS);
  duk_get_prop_string(ctx, -2, "id");
  duk_push_string(ctx, "..");
  duk_dup(ctx, 0);
  duk_call(ctx, 3);

  return 1;
}

// Default Duktape.modLoad implementation
// return Duktape.modCompile.call(module, loadFile(module.id));
//     or load shared libraries using Duktape.loadlib.
static duk_ret_t duv_mod_load(duk_context *ctx) {
  const char* id;
  const char* ext;

  dschema_check(ctx, (const duv_schema_entry[]) {
    {NULL}
  });

  duk_get_global_string(ctx, "Duktape");
  duk_push_this(ctx);
  duk_get_prop_string(ctx, -1, "id");
  id = duk_get_string(ctx, -1);
  if (!id) {
    duk_error(ctx, DUK_ERR_ERROR, "Missing id in module");
    return 0;
  }

  // calculate the extension to know which compiler to use.
  ext = id + strlen(id);
  while (ext > id && ext[0] != '.') { --ext; }

  if (strcmp(ext, ".js") == 0) {
    // Stack: [Duktape, this, id]
    duk_push_c_function(ctx, duv_loadfile, 1);
    // Stack: [Duktape, this, id, loadfile]
    duk_insert(ctx, -2);
    // Stack: [Duktape, this, loadfile, id]
    duk_call(ctx, 1);
    // Stack: [Duktape, this, data]
    duk_get_prop_string(ctx, -3, "modCompile");
    // Stack: [Duktape, this, data, modCompile]
    duk_insert(ctx, -3);
    // Stack: [Duktape, modCompile, this, data]
    duk_call_method(ctx, 1);
    // Stack: [Duktape, exports]
    return 1;
  }

  if (strcmp(ext, ".so") == 0 || strcmp(ext, ".dll") == 0) {
    const char* name = ext;
    while (name > id && name[-1] != '/' && name[-1] != '\\') { --name; }
    // Stack: [Duktape, this, id]
    duk_get_prop_string(ctx, -3, "loadlib");
    // Stack: [Duktape, this, id, loadlib]
    duk_insert(ctx, -2);
    // Stack: [Duktape, this, loadlib, id]
    duk_push_sprintf(ctx, "dukopen_%.*s", (int)(ext - name), name);
    // Stack: [Duktape, this, loadlib, id, name]
    duk_call(ctx, 2);
    // Stack: [Duktape, this, fn]
    duk_call(ctx, 0);
    // Stack: [Duktape, this, exports]
    duk_dup(ctx, -1);
    // Stack: [Duktape, this, exports, exports]
    duk_put_prop_string(ctx, -3, "exports");
    // Stack: [Duktape, this, exports]
    return 1;
  }

  duk_error(ctx, DUK_ERR_ERROR,
    "Unsupported extension: '%s', must be '.js', '.so', or '.dll'.", ext);
  return 0;
}

// Load a duktape C function from a shared library by path and name.
static duk_ret_t duv_loadlib(duk_context *ctx) {
  const char *name, *path;
  uv_lib_t lib;
  duk_c_function fn;

  // Check the args
  dschema_check(ctx, (const duv_schema_entry[]) {
    {"path", duk_is_string},
    {"name", duk_is_string},
    {NULL}
  });

  path = duk_get_string(ctx, 0);
  name = duk_get_string(ctx, 1);

  if (uv_dlopen(path, &lib)) {
    duk_error(ctx, DUK_ERR_ERROR, "Cannot load shared library %s", path);
    return 0;
  }
  if (uv_dlsym(&lib, name, (void**)&fn)) {
    duk_error(ctx, DUK_ERR_ERROR, "Unable to find %s in %s", name, path);
    return 0;
  }
  duk_push_c_function(ctx, fn, 0);
  return 1;
}

// Given a module and js code, compile the code and execute as CJS module
// return the result of the compiled code ran as a function.
static duk_ret_t duv_mod_compile(duk_context *ctx) {
  // Check the args
  dschema_check(ctx, (const duv_schema_entry[]) {
    {"code", dschema_is_data},
    {NULL}
  });
  duk_to_string(ctx, 0);

  duk_push_this(ctx);
  duk_get_prop_string(ctx, -1, "id");

  // Wrap the code
  duk_push_string(ctx, "function(){var module=this,exports=this.exports,require=this.require.bind(this);");
  duk_dup(ctx, 0);
  duk_push_string(ctx, "}");
  duk_concat(ctx, 3);
  duk_insert(ctx, -2);

  // Compile to a function
  duk_compile(ctx, DUK_COMPILE_FUNCTION);

  duk_push_this(ctx);
  duk_call_method(ctx, 0);

  return 1;
}

static duk_ret_t duv_main(duk_context *ctx) {

  duk_push_global_object(ctx);
  duk_dup(ctx, -1);
  duk_put_prop_string(ctx, -2, "global");

  duk_push_boolean(ctx, 1);
  duk_put_prop_string(ctx, -2, "dukluv");

  // Load duv module into global uv
  duk_push_c_function(ctx, dukopen_uv, 0);
  duk_call(ctx, 0);
  duk_put_prop_string(ctx, -2, "uv");

  // Replace the module loader with Duktape 2.x polyfill.
  duk_get_prop_string(ctx, -1, "Duktape");
  duk_del_prop_string(ctx, -1, "modSearch");
  duk_push_c_function(ctx, duv_mod_compile, 1);
  duk_put_prop_string(ctx, -2, "modCompile");
  duk_push_c_function(ctx, duv_mod_resolve, 1);
  duk_put_prop_string(ctx, -2, "modResolve");
  duk_push_c_function(ctx, duv_mod_load, 0);
  duk_put_prop_string(ctx, -2, "modLoad");
  duk_push_c_function(ctx, duv_loadlib, 2);
  duk_put_prop_string(ctx, -2, "loadlib");
  duk_pop(ctx);

  // Put in some quick globals to test things.
  duk_push_c_function(ctx, duv_path_join, DUK_VARARGS);
  duk_put_prop_string(ctx, -2, "pathJoin");

  duk_push_c_function(ctx, duv_loadfile, 1);
  duk_put_prop_string(ctx, -2, "loadFile");

  // require.call({id:uv.cwd()+"/main.c"}, path);
  duk_push_c_function(ctx, duv_require, 1);
  {
    // Store this require function in the module prototype
    duk_push_global_stash(ctx);
    duk_push_object(ctx);
    duk_dup(ctx, -3);
    duk_put_prop_string(ctx, -2, "require");
    duk_put_prop_string(ctx, -2, "modulePrototype");
    duk_pop(ctx);
  }
  duk_push_object(ctx);
  duk_push_c_function(ctx, duv_cwd, 0);
  duk_call(ctx, 0);
  duk_push_string(ctx, "/main.c");
  duk_concat(ctx, 2);
  duk_put_prop_string(ctx, -2, "id");
  duk_dup(ctx, 0);
  duk_call_method(ctx, 1);

  uv_run(&loop, UV_RUN_DEFAULT);

  return 0;
}

static void duv_dump_error(duk_context *ctx, duk_idx_t idx) {
  fprintf(stderr, "\nUncaught Exception:\n");
  if (duk_is_object(ctx, idx)) {
    duk_get_prop_string(ctx, -1, "stack");
    fprintf(stderr, "\n%s\n\n", duk_get_string(ctx, -1));
    duk_pop(ctx);
  }
  else {
    fprintf(stderr, "\nThrown Value: %s\n\n", duk_json_encode(ctx, idx));
  }
}