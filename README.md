# BlocklandJS

# what is this
complete rewrite of blocklandjs from the ground up using duktape instead of v8
slower, but easier to maintain :)

# commands
### TorqueScript Functions
* `js_eval(string)` - evaluate javascript code in the global context<br>
`js_eval("2+2")`

### JavaScript Functions
* `print(string)` - print a string to console<br>
`print("hello world")` - prints hello world to console

### Compilation

* Link against the mozjs library, compile dllmain.cpp, Torque.cpp. Everything else is up to you.

