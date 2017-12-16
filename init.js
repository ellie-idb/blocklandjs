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

var blocklandJS_objhandler = {
	construct: function(target, argumentsList, newT) {
		newT.apply(target, argumentsList);
		return new Proxy(target, blocklandJS_funcHandler);
	}
};

var blocklandJS_funcHandler = {
	get: function(tar, name) {
		if(ts_getMethods(tar.type).some(function(e, i) {
			if(name.toLowerCase() === e.toLowerCase()){
				return true;
			}
		}))
		{
			return function() {
				var args = [tar.type, name, tar.obj];
				args.push.apply(args, arguments);
				return ts_call.apply(null, args);
			}
		} //Handle function calls.
		return ts_getObjectField(tar.obj, name);
	},
	set: function(tar, name, val) {
		ts_setObjectField(tar.obj, name, val);
	}
};

// Link a namespace to a function that will create an object
function ts_linkClass(type) {
	// Create a new method
	/*
	function _createMethod(that, obj, name, func) {
		that[name] = function() {
			var args = [obj];
			args.push.apply(args, arguments);
			return func.apply(null, args);
		};
	}
	*/
	var kk = function(args) {
		var _type = type;
		var _obj = ts_newObj(_type);
		this.obj = _obj;
		this.type = _type;

		// Add arguments
		for (var i in args) {
			// Workaround
			if (i === 'datablock' && ts_func(_type, "setDatablock"))
				ts_func(_type, "setDatablock")(args[i]);
			ts_setObjectField(_obj, i, args[i]);
		}
		print("Registered " + _type);
		// Make it visible from TS
		ts_registerObject(_obj);
	};
	return new Proxy(kk, blocklandJS_objhandler);
}

function ts_globalVariables() {
	var h = {
		get: function(tar, name) {
			return ts_getVariable(name);
		},
		set: function(tar, name, val) {
			ts_setVariable(name, val);
		}
	}
	return new Proxy({}, h);
}
