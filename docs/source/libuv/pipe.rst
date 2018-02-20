pipe
====

.. js:class:: uv.pipe(ipc)
	
	Construct a new named pipe.

	:param bool ipc: A toggle for if this pipe is going to be used for IPC.

	.. note:: The ``Stream`` object methods also apply here.

	.. js:method:: bind(path)

		Bind the pipe to a named pipe/path.

		:param string path: Path, or the named pipe.

	.. js:method:: open(fd)

		Open the pipe on an existing file descriptor or HANDLE.

		:param int fd: The file descriptor to open the ``Pipe`` on.

		:throws: If unable to open the pipe on said file descriptor.

	.. js:method:: connect(pipe)

		Connect to the named pipe/socket.

		:param string pipe: The named pipe to connect to.

	.. js:method:: getsockname()

		Get the name of the socket.

		:returns: A string representing the name of the socket/pipe.

	.. js:method:: getpeername()

		Get the name of the named pipe that the pipe is connected to.

		:returns: A string representing the name of the peer.

	.. js:method:: pending_instances(count)

		Set the number of pending pipe instance handles when the pipe server is waiting for connections.

		:param int count: Number of pending pipe instance handles.

	.. js:method:: pending_count()

		Get pending count of pipe. Used to transmit types.

		:returns: An integer representing the pending count of handles.

	.. js:method:: pending_type()

		Get pending type of handle being transmitted.

		:returns: An integer representing the type to be transmitted.

	.. js:method:: chmod(flags) 

		Alter pipe permissions. 

		:param int flags: The flags that should now be set to the pipe.
