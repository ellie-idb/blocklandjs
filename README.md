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
* `print(string)` - print a string to console<br>
`print("hello world")` - prints hello world to console

* `ts_eval(string)` - evaluate a string as torquescript<br>
`ts_eval("4+5")` - evals and returns 9

* `ts_call(namespace, function [, arg1, arg2, arg3, ...])` - calls a torquescript function<br>
`ts_call("ts", "echo", "hi")` - calls echo("hi");

* `ts_func(namespace, function)` - declares a torquescript function in javascript<br>
`ts_func("ScriptObject", "getID")` - defines ScriptObject__getID

* `ts_setVariable(name, value)` - set the value of a global variable
`ts_setVariable("Pref::Server::Password", "asdf")` - set $Pref::Server::Password to be "asdf"

* `ts_newObj(classname)` - returns a pointer to the object created
`ts_newObj("fxDTSBrick")` - creates a new fxDTSBrick- however, it does not register it

* `ts_registerObject(pointer to obj)` - registers the object giving it a unique id
`ts_registerObject(ts_newObj("fxDTSBrick"))` - will register the fxDTSBrick object we just created

* `ts_setObjectField(obj, name, value)` - set field on obj with the name (name) to val
`ts_setObjectField(playerObj, 'position', '0 0 0'));

