tcp
===

.. js:class:: uv.tcp()

	Construct a new Tcp object, inheriting from the ``Stream`` object.

	.. NOTE::
		Stream object methods apply here.

	.. js:method:: open(fd)

		Open an existing file descriptor/SOCKET as a TCP handle.

		:param number fd: existing file descriptor or socket.
		:throws: If unable to open

	.. js:method:: nodelay(enable)

		Enable TCP_NODELAY, which disables Nagle’s algorithm.

		:param bool enable: enable or disable nodelay
		:throws: If unable to set nodelay

	.. js:method:: simultaneous_accepts(enable)

		Enable / disable simultaneous asynchronous accept requests

		:param bool enable: enable or disable simultaneous accepts for object.
		:throws: If unable to set simultaneous_accepts

	.. js:method:: keepalive(enable, delay)

		Enable / disable TCP keepalive. 

		:param bool enable: enable or disable keepalive
		:param int delay: initial delay for keepalive in seconds.
		:throws: If unable to set keepalive

	.. js:method:: bind(host, port)

		Bind to a specified host/port.

		:param string host: IP address to bind to.
		:param int port: Port to bind to.
		:throws: If unable to bind to host/port

	.. js:method:: getpeername()

		Get the address of the client/peer connected to the handle.

		:returns: An object containing the family, port, and IP of the peer.
		:throws: If unable to get peername

	.. js:method:: getsockname()

		Get the address of the socket (us)

		:returns: An object containing the family, port, and IP of the socket.
		:throws: If unable to get socket name.

	.. js:method:: connect(host, port)

		Connect a socket to a host, and port.

		:param string host: IP address to connect to.
		:param int port: Port to connect to.
		:throws: If unable to connect to host/port.
 




