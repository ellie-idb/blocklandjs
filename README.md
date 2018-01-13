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

* `ts_eval(string)` - evaluate a string as torquescript<br>
`ts_eval("4+5")` - evals in torquescript, returns 9

* `ts_func(function)` - declares a global torquescript function in javascript<br>
`var a = ts_func("error"); a('aaaaaa');` - would error out "aaaaaa"

* `ts_setVariable(name, value)` - set the value of a global variable<br>
`ts_setVariable("Pref::Server::Password", "asdf")` - set $Pref::Server::Password to be "asdf"

* `ts_getVariable(name)` - gets the value of a global variable<br>
`ts_getVariable("Pref::Server::Password")` - returns "asdf"

* `ts_registerObject(pointer to obj)` - registers the object giving it a unique id<br>
`ts_registerObject(ts_newObj("fxDTSBrick"))` - will register the fxDTSBrick object we just created

* `ts_obj(id/name)` - returns a pointer to the object identified by the name or id you pass to it<br>
`ts_obj(1234)` - returns a pointer to the object with id 1234

* `ts_linkClass(classname)` - returns constructor for class<br>
`ts_linkClass("fxDTSBrick")` - returns function

### Compilation

* Link against the mozjs library, compile dllmain.cpp, Torque.cpp. Everything else is up to you.

