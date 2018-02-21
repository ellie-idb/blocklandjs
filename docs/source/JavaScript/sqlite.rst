sqlite
======

.. js:class:: sqlite()

	Construct a new SQLite database worker.

	.. js:method:: open(database)

		Open a new database.

		:throws: If unable to open the database.

	.. js:method:: exec(query[, callback])

		Execute a SQL query. The callback will be called with the first argument as an array, holding the results.

		:throws: If a SQLite error is occured, or the database is not opened.

	.. js:method:: close()

		Close an opened database.

		:throws: If unable to close the database, or there is not a database currently opened.