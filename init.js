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

var handler = {
	get: function(tar, name) {
		return ts_getObjectField(tar._obj, name);
	}
}

// Link a namespace to a function that will create an object
function ts_linkClass(type) {
	// Create a new method
	function _createMethod(that, obj, name, func) {
		that[name] = function() {
			var args = [obj];
			args.push.apply(args, arguments);
			return func.apply(null, args);
		};
	}
	var kk = function(args) {
		var _type = type;
		var _obj = ts_newObj(_type);
		// Apply all methods
		var _functions = ts_getMethods(_type);
		for (var i = 0; i < _functions.length; ++i) {
			var _func_name = _functions[i];
			var _func = ts_func(_type, _func_name);
			// Create method
			_createMethod(this, _obj, _func_name, _func);
		}
		// Setter
		//this.set = function(name, value) {
		//	ts_setObjectField(_obj, name, value);
		//};
		//this.prototype.__defineGetter__(function(name) {
		//		return ts_getObjectField(_obj, name);
		//	});
		// Getter
		this.get = function(name) {
			return ts_getObjectField(_obj, name);
		};
		// Add arguments
		for (var i in args) {
			// Workaround
			if (i === 'datablock' && this.setDatablock)
				this.setDatablock(args[i]);
			this.set(i, args[i]);
		}
		// Make it visible from TS
		ts_registerObject(_obj);
	};
	return new Proxy(kk, handler);
}
