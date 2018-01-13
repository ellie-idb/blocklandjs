# BlocklandJS

# what is this
complete rewrite of blocklandjs from the ground up using spidermonkey

# commands
### TorqueScript Functions
* `js_eval(string)` - evaluate javascript code in the global context<br>
`js_eval("2+2")`

### JavaScript Functions
* `print(string)` - print a string to console<br>
`print("hello world")` - prints hello world to console

* `ts_func(function)` - declares a global torquescript function in javascript<br>
`var a = ts_func("error"); a('aaaaaa');` - would error out "aaaaaa"

* `setGlobalVariable(name, value)` - set the value of a global variable<br>
`setGlobalVariable("Pref::Server::Password", "asdf")` - set $Pref::Server::Password to be "asdf"

* `getGlobalVariable(name)` - gets the value of a global variable<br>
`getGlobalVariable("Pref::Server::Password")` - returns "asdf"

* `ts_registerObject(pointer to obj)` - registers the object giving it a unique id<br>
`ts_registerObject(ts_newObj("fxDTSBrick"))` - will register the fxDTSBrick object we just created

* `ts_obj(id/name)` - returns a pointer to the object identified by the name or id you pass to it<br>
`ts_obj(1234)` - returns a pointer to the object with id 1234

* `ts_linkClass(classname)` - returns constructor for class<br>
`ts_linkClass("fxDTSBrick")` - returns function

## Object manipulation
* `(object).(any)` - would return the field as it is in Torque
`var a = ts_obj(1234); a.hello = 'world'; print(a.hello);` - would get the object with the id 1234, set the object's variable "hello" to world, then echo out "world".
* `(object).(any)()` - call a function on an object this way...
`var a = ts_obj(1234); a.dump();` - would call dump() on the object with an id of 1234.

### Compilation

* Link against the mozjs library, compile dllmain.cpp, Torque.cpp. Everything else is up to you.

