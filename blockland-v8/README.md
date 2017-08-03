# BlocklandJS

# what is this
complete rewrite of blocklandjs from the ground up using duktape instead of v8
slower, but easier to maintain :)

# commands
### TorqueScript Functions
* `js_eval(string)` - evaluate javascript code in the global context<br>
`js_eval("2+2")`

* `js_load(string)` - execute .js file in the global context<br>
`js_load("config/test.js")`

### JavaScript Functions
* `ts_eval(string)` - evaluate a string as torquescript<br>
`ts_eval("4+5")`

* `ts_call(namespace, function [, arg1, arg2, arg3, ...])` - calls a torquescript function<br>
`ts_call("", "echo", "hi")`

* `ts_func(namespace, function)` - declares a torquescript function in javascript<br>
`ts_func("ScriptObject", "getID")` - defines ScriptObject__getID

