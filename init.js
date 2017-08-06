function ts_func(namespace, name)
{
	return function(){
	arg = arguments;
	//print(argc + ' arguments');
	//yeah i could optimize this more but idk.. i could have a bunch of args but w/e
	//idk it works ill leave it and it's fast enough
	switch(arguments.length)
	{
		case 0:
			return ts_call(namespace, name);

		case 1:
			return ts_call(namespace, name, arg[0]);

		case 2:
			return ts_call(namespace, name, arg[0], arg[1]);

		case 3:
			return ts_call(namespace, name, arg[0], arg[1], arg[2]);

		case 4:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3]);

		case 5:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4]);

		case 6:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);

		case 7:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6]);

		case 8:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7]);
		
		case 9:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8]);

		case 10:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9]);

		case 11:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10]);

		case 12:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10], arg[11]);

		case 13:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10], arg[11], arg[12]);

		case 14:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10], arg[11], arg[12], arg[13]);

		case 15:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10], arg[11], arg[12], arg[13], arg[14]);

		case 16:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10], arg[11], arg[12], arg[13], arg[14], arg[15]);

		case 17:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10], arg[11], arg[12], arg[13], arg[14], arg[15], arg[16]);

		case 18:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10], arg[11], arg[12], arg[13], arg[14], arg[15], arg[16], arg[17]);

		case 19:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10], arg[11], arg[12], arg[13], arg[14], arg[15], arg[16], arg[17], arg[18]);

		case 20:
			return ts_call(namespace, name, arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9], arg[10], arg[11], arg[12], arg[13], arg[14], arg[15], arg[16], arg[17], arg[18], arg[19]);

	}
}
}
