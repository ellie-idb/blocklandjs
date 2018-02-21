globals
=======

.. js:method:: print(text)
	
	Print out text to the console. Same function as ``console.log``

	.. warning:: Ensure that the string is < 4096 characters, or else it will not print out the intended string.

	:param string text: String to print out to console.

.. js:method:: version()

	Get the current version of V8 running.

	:returns: A string containing the current version of V8 running.

.. js:method:: immediateMode(enable)

	Enable or disable "immediate mode". Used when extreme precision for ``uv.misc.hrtime()`` is needed.

	.. warning:: CPU usage will be extremely high if immediateMode is enabled.

	:param bool enable: Enable/disable immediate mode.

.. js:method:: load(file)

	Load and evaluate a JavaScript file.

	:param string file: Path to the JavaScript file. The path can be relative to the current working directory.


