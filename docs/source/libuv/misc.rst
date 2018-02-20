misc
====
 
Misc functions for a tinkering user.

.. js:module:: uv.misc

.. js:method:: version()

	Get the current version of LibUV (as a integer)

	:returns: A integer representing the current libuv version.

.. js:method:: version_string()

	Get the current version of LibUV (as a string)

	:returns: A string representing the current libuv version.

.. js:method:: get_process_title()

	Get the current process title.

	:returns: A string representing the current process title.

.. js:method:: set_process_title(name)

	Set the process title of the executable.

	:param string name: The new process title.

.. js:method:: resident_set_memory()

	Get the resident set size for the current process.

	:returns: A integer representing the resident set size

.. js:method:: uptime()
	
	Get the current system uptime

	:returns: A integer representing the current system uptime (in milliseconds)

.. js:method:: getrusage()
	
	Get the current resource usage of the system.

	:returns: An object containing various fields relating to resource usage.

.. js:method:: cpu_info()

	Get info on the processors on the host system.

	.. js:attribute:: object.model

		Model of CPU.

	.. js:attribute:: object.speed

		Speed of CPU.

	.. js:attribute:: object.times

		   Times for the CPU.

		.. js:attribute:: times.user

			"User" times for the CPU.

		.. js:attribute:: object.times.nice

			"Nice" times for the CPU.

		.. js:attribute:: object.times.sys

			"Sys" times for the CPU.

		.. js:attribute:: object.times.idle

			"Idle" times for the CPU.

		.. js:attribute:: object.times.irq

			"IRQ" times for the CPU.


	:returns: An array containing objects containing information about the processors in the host system.

.. js:method:: interface_addresses()

	Get the address, family, and port of the network interfaces on the host system.

	.. js:attribute:: object.name

		Name of the network interface.

	.. js:attribute:: object.internal

		Boolean representing if the object is an internal interface or not.

	.. js:attribute:: object.address

		String representing the IPv4/IPv6 address of the interface.

	.. js:attribute:: object.protocol

		String representing the protocol of the interface.

	:returns: An array containing objects containing information about the network interfaces in the host.

.. js:method:: loadavg()
	
	Get the load average of the system.

	:returns: An array containing three numbers representing the load averages of the system.

.. js:method:: exepath()

	Get the path to the running executable.

	:returns: A string corresponding to the path of the current running executable.

.. js:method:: cwd()

	Get the current working directory.

	:returns: A string corresponding to the current working directory.

.. js:method:: os_homedir() 

	Get the home directory of the current logged in user.

	:returns: A string corresponding to the home directory of the current user.

.. js:method:: chdir(path)
	
	Change the current working directory to the path.

	.. warning:: Be sure to change the current working path to the Blockland/ directory, or else Torque will be fucked.

	:param string path: The path to change the current working directory to.

.. js:method:: get_total_memory()

	Get the current memory of the system.

	:returns: A number representing the total memory, in kilobytes.

.. js:method:: hrtime()

	Get the current high-resolution timestamp of the system.

	:returns: A number representing the current high-resolution timestamp, in nanoseconds.

.. js:method:: now()

	Get a value corresponding to some value stored in the timer loop. This is relative to some random point in time in the past.

	:returns: A number representing a timestamp.


