stream
=======

.. js:module:: uv.stream

.. js:method:: shutdown()

	Shutdown a stream that is connected, disabling the write side. The shutdown callback will be called directly after.
	
	.. warning:: Ensure that you do not close the stream at the same time that you shutdown. Close after the shutdown callback has been called.

.. js:method:: listen(backlog)

	Set a ``Stream`` object to listen. 

	:param int backlog: The number of connections the kernel may queue.

.. js:method:: accept()
	
	Accept a new connection on a ``Stream``.

	:returns: An object corresponding to the original type of the ``Stream`` that this function was called on.

.. js:method:: read_start()

	Start reading from the ``Stream``.

	:throws: If unable to start reading from the Stream (disconnected, etc.)

.. js:method:: read_stop()

	Stop reading data from the ``Stream``.

	.. note:: The 'data' callback will not be called beyond this point, if new data is recieved.

.. js:method:: write(buffer)

	Write data to the ``Stream``.

	:param Uint8Array buffer: A Uint8Array containing the raw data you want to write to the ``Stream``.

	:throws: If unable to write to the ``Stream``.

.. js:method:: on(event, callback)

	Set a callback for an event occuring. Possible events are 

	========== =====================
	Event      Callback schema
	========== =====================
	data       function(len, buffer)
	connection function(status)
	write      function(status)
	connect    function(status)
	close      function()
	shutdown   function(status)
	========== =====================

	:param string event: A string indicating the event that you want to listen for.
	:param function callback: A function that should be called upon these events occuring.

.. js:method:: is_writable()
	
	Returns a value indicating if the ``Stream`` can be written to.

	:returns: A boolean indicating if you can write to the ``Stream``.

.. js:method:: is_readable()
	
	Returns a value indicating if the ``Stream`` can be read from.

	:returns: A boolean indicating if you can read from the ``Stream``

.. js:method:: set_blocking(enable)

	Set if the ``Stream`` should block.

	:param bool enable: Whether the ``Stream`` should block or not.

.. js:method:: close()

	Close the ``Stream``.

	.. note:: If you call this while there are pending events, there is a very high likelyhood that the game will crash. Ensure that you shutdown the ``Stream``, before closing it.

	:throws: If unable to close the ``Stream``.
