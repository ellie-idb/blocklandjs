// Call a function
function ts_func(namespace, name) {
	return function() {
		// Merge all arguments
		var args = [namespace, name];
		args.push.apply(args, arguments);
		// Call the function
		return ts_call.apply(null, args);
	};
}

// Link a namespace to a function that will create an object
function ts_linkClass(type) {
	return function() {
		var _type = type;
		var _obj = ts_newObj(_type);
		// Apply all methods
		var _functions = ts_getMethods(_type);
		for (var i in _functions) {
			var _func_name = _functions[i];
			var _func = ts_func(_type, _func_name);
			// Create method
			this[_func_name] = function() {
				var args = [_obj];
				args.push.apply(args, arguments);
				return _func.apply(null, args);
			};
		}
		// Setter
		this.set = function(name, value) {
			ts_setObjectField(_obj, name, value);
		};
		// Getter
		this.get = function(name) {
			return ts_getObjectField(_obj, name);
		};
	};
}
