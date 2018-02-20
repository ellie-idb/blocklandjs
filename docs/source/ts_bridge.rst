TorqueScript bridge
===================

The TorqueScript bridge is a way to communicate between JavaScript and TorqueScript. It is purely meant as a compatibility layer.

.. js:module:: ts

.. js:method:: setVariable(name, value)
	
	This will set a variable within the TorqueScript global scope to value.

	.. warning:: Ensure that the value is coercible to a string/a string!

	:param string name: The name of the variable to set value to.
	:param string value: The value of the variable.

.. js:method:: getVariable(name) 

	This will return a variable corresponding to a TorqueScript global variable.

	.. note:: This will ALWAYS return a string, due to how TorqueScript handles internal types.

	:param string name: The name of the variable to get the value of.

	:returns: A string corresponding to the value held by the variable.

.. js:method:: linkClass(name)

	Returns a constructor for a TorqueScript class.

	.. note:: Be sure to register any object created by this constructor. If you fail to register an object, no method call will work, as the object has no ID.

	:param string name: The name of the class corresponding to a TorqueScript class.
	:returns: A function that works as a constructor for the TorqueScript type specified.

.. js:method:: registerObject(object)

	Register an object with Torque, allowing for method functions to be called on it, and giving it an object id.

	:param object object: The object to be registered with Torque.

.. js:method:: func(name)

	Get a JavaScript function corresponding to the TorqueScript function.

	:param string name: The name of the function in the global TorqueScript namespace.

	:returns: A JavaScript function that corresponds to the TorqueScript function

.. js:method:: obj(name)
	obj(id)

	Get an object referring to the TorqueScript object given by the name/id

	:param int id: The ID corresponding to the TorqueScript object.

	:param string name: The name corresponding to the TorqueScript object.

	:returns: An object referring to the TorqueScript object given by the id/name.

.. js:method:: switchToTS()

	Change your in-game console to use TorqueScript instead of JavaScript.

.. js:method:: expose(info, function)

	.. js:attribute:: info.class

		The class that the function should be registered to. Optional.

	.. js:attribute:: info.name

		The name that the function should be registered as.

	.. js:attribute:: info.description

		The description that the function should have. Optional.

	:param object info: An object containing all of the attributes listed above.
	:param function function: A function that should be called every time the TorqueScript callback is called.

	Exposes a JavaScript function to TorqueScript.

.. js:method:: ts.SimSet.getObject(SimSet, id)
	
	Get an object inside of a SimSet.

	.. warning:: This DOES NOT DO ANY BOUND CHECKING. Assume this to be a very insecure function.

	:param object SimSet: An object referring to a TorqueScript SimSet.
	:param int id: The integer referring to the object's position within the SimSet.

	:returns: An object that is found at the index given, inside of the SimSet.