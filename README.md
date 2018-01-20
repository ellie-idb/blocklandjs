# BlocklandJS

# what is this

this is a little side-project i started with a friend
to build this, open the sln and compile under 32bit and debug
the libraries can be rebuilt under your own time if you don't trust them lol (compile those under x86 debug as well)


#commands
###TorqueScript Functions
* `js_eval(string)` - evaluate javascript code in the global context<br>
`js_eval("2+2")`

* `js_exec(string)` - execute .js file in the global context<br>
`js_exec("config/test.js")`

###JavaScript Functions
* `ts_eval(string)` - evaluate a string as torquescript<br>
`ts_eval("4+5")`

* `ts_call(namespace, function [, arg1, arg2, arg3, ...])` - calls a torquescript function<br>
`ts_call("", "echo", "hi")`

* `ts_setVariable(var, value)` - sets a torquescript global variable to a given value

* `ts_getVariable(var)` - returns the value of a torquescript global variable

* `ts_func(namespace, function)` - declares a torquescript function in javascript<br>
`ts_func("ScriptObject", "getID")` - defines ScriptObject__getID

* `ts_newObj(class, fieldObj)` - returns a new torquescript object<br>
`ts_newObj("ScriptObject", {someField: "someValue"})`

* `ts_setObjectField(obj, field, value) - sets a field's value on an object`

* `ts_getObjectField(obj, field) - returns the value of an object's field`
