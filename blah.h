extern "C" {
#include "duv.h"
#include "misc.h"
}
#include "duktape.h"

struct duv_list {
  const char* part;
  int offset;
  int length;
  struct duv_list* next;
};

static duk_ret_t duv_loadfile(duk_context *ctx);
static duv_list_t* duv_list_node(const char* part, int start, int end, duv_list_t* next);
static duk_ret_t duv_path_join(duk_context *ctx);
static duk_ret_t duv_require(duk_context *ctx);
static duk_ret_t duv_mod_resolve(duk_context *ctx);
static duk_ret_t duv_mod_load(duk_context *ctx);
static duk_ret_t duv_loadlib(duk_context *ctx);
static duk_ret_t duv_mod_compile(duk_context *ctx);
static duk_ret_t duv_main(duk_context *ctx);
