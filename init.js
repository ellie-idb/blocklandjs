// Call a function
function ts_func(namespace, name) {
	return function() {
		// Merge all arguments
		var args = [namespace, name];
		args.push.apply(arguments);
		// Call the function
		return ts_call.apply(null, args);
	};
}
