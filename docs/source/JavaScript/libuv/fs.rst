fs
==========

.. js:module:: uv.fs

.. js:method:: close(fd)
	
	See the manpages for `close(2) <http://linux.die.net/man/2/close>`_

	:param int fd: A file descriptor that is open.

	:throws: If unable to close the file descriptor/the file descriptor does not exist.

.. js:method:: open(path, flags, callback)

	See the manpages for `open(2) <http://linux.die.net/man/2/open>`_

	:param string path: The path to the file. May be relative to the current working directory.

	:param string flags: Flags to open the file with. 

	:param function(fd) callback: The callback that will recieve the file descriptor of the newly opened file.

.. js:method:: read(fd, size, offset, callback)

	See the manpages for `preadv(2) <http://linux.die.net/man/2/preadv>`_

	:param int fd: The opened file-descriptor.

	:param int size: The amount of bytes to read from the file.

	:param int offset: The offset to read from.

	:param function(ArrayBuffer) callback: The callback function which will take an ``ArrayBuffer`` as it's first argument.

.. js:method:: unlink(fd, callback)

	See the manpages for `unlink(2) <http://linux.die.net/man/2/unlink>`_

	:param int fd: The opened file-descriptor

	:param function(bool) callback: A callback that will accept a boolean as it's first argument, indicating the status of the request.

.. js:method:: write(fd, array, offset, callback)

	See the manpages for `pwritev(2) <http://linux.die.net/man/2/pwritev>`_

	:param int fd: The opened file-descriptor.

	:param Uint8Array array: The bytes to write out to the file.

	:param int offset: The offset for writing to the file.

	:param function(bool) callback: A callback that will accept a boolean as it's first argument, indicating the status of the request.

.. js:method:: mkdir(path, callback)

	See the manpages for `mkdir(2) <http://linux.die.net/man/2/mkdir>`_

	:param string path: The path to create.

	:param function(bool) callback: The callback which will recieve the status of the request.

.. js:method:: mkdtemp(path, callback)

	See the manpages for `mkdtemp(3) <http://linux.die.net/man/3/mkdtemp>`_

.. js:method:: rmdir(path, callback)

	See the manpages for `rmdir(2) <http://linux.die.net/man/2/rmdir>`_

.. js:method:: scandir(path, callback)

	See the manpages for `scandir(3) <http://linux.die.net/man/3/scandir>`_

	After this function finishes execution, the callback will recieve a function which you can repeatedly call to get the next file.

.. js:method:: stat(path, callback)

	See the manpages for `stat(2) <http://linux.die.net/man/2/stat>`_

.. js:method:: fstat(fd, callback)

	See the manpages for `fstat(2)  <http://linux.die.net/man/2/fstat>`_

.. js:method:: lstat(path, callback)
	
	See the manpages for `lstat(2) <http://linux.die.net/man/2/lstat>`_

.. js:method:: rename(path, new_path, callback)

	See the manpages for `rename(2) <http://linux.die.net/man/2/rename>`_

.. js:method:: fsync(fd, callback)

	See the manpages for `fsync(2) <http://linux.die.net/man/2/fsync>`_

.. js:method:: fdatasync(fd, callback)

	See the manpages for `fdatasync(2) <http://linux.die.net/man/2/fdatasync>`_

.. js:method:: ftruncate(fd, offset, callback)

	See the manpages for `ftruncate(2) <http://linux.die.net/man/2/fsync>`_

.. js:method:: sendfile(fd, out_fd, offset, length, callback)
	
	See the manpages for `sendfile(2) <http://linux.die.net/man/2/sendfile>`_

.. js:method:: access(path, flags, callback)

	See the manpages for `access(2) <http://linux.die.net/man/2/access>`_

.. js:method:: chmod(path, flags, callback)

	See the manpages for `chmod(2) <http://linux.die.net/man/2/chmod>`_

.. js:method:: fchmod(fd, flags, callback)

	See the manpages for `fchmod(2) <http://linux.die.net/man/2/fchmod>`_

.. js:method:: utime(path, atime, mtime, callback)

	See the manpages for `utime(2) <http://linux.die.net/man/2/utime>`_

.. js:method:: futime(fd, atime, mtime, callback)

	See the manpages for `futime(2) <http://linux.die.net/man/2/futime>`_

.. js:method:: link(path, new_path, callback)

	See the manpages for `link(2) <http://linux.die.net/man/2/link>`_

.. js:method:: symlink(path, new_path, flags, callback)

	See the manpages for `symlink(2) <http://linux.die.net/man/2/symlink>`_

.. js:method:: readlink(path, callback)

	See the manpages for `readlink(2) <http://linux.die.net/man/2/readlink>`_

.. js:method:: chown(path, uid, gid, callback)

	See the manpages for `chown(2) <http://linux.die.net/man/2/chown>`_

.. js:method:: fchown(fd, uid, gid, callback)

	See the manpages for `fchown(2) <http://linux.die.net/man/2/fchown>`_


