timer
=====

.. js:class:: uv.timer()
	
	A timer class, providing similar functionality of the default ``setTimeout``, and ``setRepeat``

	.. js:method:: start(timeout, repeat, callback, arguments)

		Start the timer.

		:param int timeout: The time that the timer should wait before first calling the callback (in milliseconds).
		:param int repeat: The time between repeat calls of the timer (in milliseconds).
		:param function callback: The callback that should be called every time that this timer ticks.
		:param Array arguments: arguments to pass to the callback function

	.. js:method:: stop()

		Stop the timer.

		:throws: If the timer is not running.

	.. js:method:: again()

		Start the timer with the parameters specified before, after a ``stop()`` call has been made.

		:throws: If this is the timer's first go.

	.. js:method:: set_repeat(repeat)

		Set the time between repeat calls of this timer.

		:param int repeat: The time between repeat calls of the timer (in milliseconds).

		:throws: If the timer has not been started.

	.. js:method:: get_repeat()

		Get the time between repeat calls of this timer.

		:returns: An integer representing the time between repeat calls of this timer (in milliseconds).
