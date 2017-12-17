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
`ts_eval("4+5")` - evals in torquescript, returns 9

* `ts_call(namespace, function [, arg1, arg2, arg3, ...])` - calls a torquescript function<br>
`ts_call("ts", "echo", "hi")` - calls echo("hi");

* `ts_func(namespace, function)` - declares a torquescript function in javascript<br>
`var a = ts_func("SimSet", "getID"); a(ts_obj("ClientGroup"));` - returns the id of the object identified by 'ClientGroup'

* `ts_setVariable(name, value)` - set the value of a global variable<br>
`ts_setVariable("Pref::Server::Password", "asdf")` - set $Pref::Server::Password to be "asdf"

* `ts_getVariable(name)` - gets the value of a global variable<br>
`ts_getVariable("Pref::Server::Password")` - returns "asdf"

* `ts_newObj(classname)` - returns a pointer to the object created<br>
`ts_newObj("fxDTSBrick")` - creates a new fxDTSBrick- however, it does not register it

* `ts_registerObject(pointer to obj)` - registers the object giving it a unique id<br>
`ts_registerObject(ts_newObj("fxDTSBrick"))` - will register the fxDTSBrick object we just created

* `ts_setObjectField(obj, name, value)` - set field on obj with the name (name) to val<br>
`ts_setObjectField(playerObj, 'position', '0 0 0'));`

* `ts_getObjectField(obj, name)` - returns the value contained in the field identified by name in obj<br>
`ts_getObjectField(playerObj, 'position')` - returns '12 0 0'

* `ts_obj(id/name)` - returns a pointer to the object identified by the name or id you pass to it<br>
`ts_obj(1234)` - returns a pointer to the object with id 1234

* `ts_linkClass(classname)` - returns constructor for class<br>
`ts_linkClass("fxDTSBrick")` - returns function

* `version()` - returns the version of duktape used<br>
`version()` - returns v1.8.0



