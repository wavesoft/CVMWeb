
# What is this

This is an NPAPI plugin for web browsers that enables real-time interaction with the CernVM guest OS. 
This plugin identifies the hypervisors installed in the system and picks the most appropriate one to
host the CernVM Guest OS. 

It then provides a simple interface to start/stop the VM and access it's data sets. 

# How to install the plugin

The plugin has no installation system yet, but you can install it manually using one of the following techniques.
You can always find the latest binary builds for your operating system in the *builds* directory.

## Installing on Mac OSX

Simply copy the plugin to your **Library/Internet Plug-Ins** directory. The plugin will be
immediately available to all of your browsers.

## Installation on Windows

*(Not yet available)*

## Installation on Linux

*(Not yet available)*

# How to use the plugin

To use this plugin you must first create an embed element in your website. You must give it a unique ID:

	<embed type="application/x-cvmweb" id="cvmweb">

You can then get a reference to the object and access it's properties:

	var o = document.getElementById('cvmweb');
	alert('We are running with ' + o.hypervisorName + "/" + e.hypervisorVersion );

# Creating sessions

Every computation session is a CernVM instance and is accessed individually. You can request a session that fits your needs
using the requestSession function:

	var sess = o.requestSession( "session name", "secret key" );

The secret key prohibits unauthorized users to access your session. The first user that requests a session is automatically
the owner and only his secret key can be used to request access to the same session again.

This function will return either a CVMWebAPISession object or *false* if an error occurred.

Upon receiving the session object you can bind your event listeners using the *addEventListener(event, callback)* function.

## Opening session

The session is unusable until the *open* function is called and the session requirements are specified:

	sess.open({ requirements })

The requirements object contains the minimum resource requirements you are asking for. If it's not specified, the default
values will be used. The supported parameters and their default values are the following:

	{
		cpus: 		1,
		memory: 	256, 		// Mb
		disk:		1024,		// Mb
		version:	'1.3'		// The CernVM Micro Release
	}

This function will return immediately. Upon a successful completion, the *onOpen* event will be fired. If an error 
occurs the *onOpenError* event will be fired. You can also monitor the progress with the *onProgress* event.

# Starting a CernVM instance

To start a CernVM instance you can use the *open* function. You must also specify the contextuaization string that will
setup the environment for your instance:

	var str = 
		"[cernvm]" +
		"users=user:users:s3cr3t" +
		"shell=/bin/bash";
	sess.start(str);
	
The function will return 1 if the action was scheduled successfully, or an error code if an error occurred. 	
Upon a successful completion, the *onStart* event will be fired. If an error occurs the *onStartError* event will be fired. 

The plugin will probe the CernVM communication channel periodically. When the channel is established the *onLive* event will
be fired. When a live connection is interrupted, the *onDead* event will be fired.

# Controlling a running instance

You can change various parameters while the VM is still running. A brief list is shown below:

## Execution cap

You can change the execution cap of your application using the *setExecutionCap* function:

	sess.setExecutionCap( 50 );	// Change Execution cap to 50%
	alert( sess.executionCap ); // Will say '50'

## Guest properties

You can set or get guest properties on hypervisors that support it using the *setProperty* and *getProperty* function:

	sess.setProperty("host", "10.110.17.67");
	var host = sess.getProperty("host");

## Controlling execution

You can pause, resume and stop the VM using the *pause*, *resume* and *stop* functions:

	sess.pause();	// Pause session
	sess.resume();	// Resume session
	sess.stop();	// Shut down VM 
	sess.reset();	// Hard-reset the VM

All these functions will also fire the *onPause*, *onResume*, *onStop* and *onReset* events, respectively.
If an error occurs, the functions will return a non-zero value. An *HVE_INVALID_STATE (-8)* will be returned
if the VM is not in the appropriate state for the operation.

## Closing session

When you are done using the session you must close it using the *close* function:

	sess.close();	// Poweroff VM, remove it from the hypervisor and close session

When the VM is shut the *onClose* event will be fired. You can monitor the cleanup process by listening
for *onProgress* events.

The *close* function shutdown, destroy and unregister the VM. In order to use this session again you must 
repeat the *open* procedure.

# Monitoring the progress

The *open*, *start* and *close* actions are time-consuming and are executed asynchronously. You can monitor
the progress of their execution by listening for *onProgress* events.

Take for example the following handler:

	sess.addEventListener('progress', function( current, total, msg ) {
		window.console.log( "Performing: " + msg + ", " + current + " out of " + total + " tasks completed" );
	});

# Appendix A - Error codes

Most of the functions return a number. If the value is less than zero, an error has occurred. Here is a short list of 
the errors that can occur.

	 1   Successfuly scheduled (status not known yet)
	 0   No error
	-1   Creation Error
	-2	 Modification Error
	-3   Control Error
	-4   Deletion Error
	-5   Query Error
	-6   I/O Error
	-7   External library / server error
	-8   Not a valid operation for the current state
	-9   Not found
	-10	 Access denied
	-100 Function is not implemented

# Appendix B - Events

The following list of events is fired by the session object. You can listen for them using the 
*session.addEventListener* function.

	Event name			Callback signature
	---------------     -------------------------
	open				( )
	start				( )
	close				( )
	pause				( )
	resume				( )
	stop				( )
	openError			( errorMessage, errorCode )
	startError			( errorMessage, errorCode )
	closeError			( errorMessage, errorCode )
	pauseError			( errorMessage, errorCode )
	resumeError			( errorMessage, errorCode )
	stopError			( errorMessage, errorCode )
	progress			( tasksCompleted, tasksPending, statusMessage )
	errror				( errorMessage, errorCode, errorCategory )
    debug				( debugMessage )

# Appendix C - Functions

The following list of functions is supported by the session object:

	Function name		Call signature					Return value
	---------------     -------------------------       ------------------------
	start				( userData )					1 or < 0 on error
	open				( {								1 or < 0 on error
							cpus 	: <int>,
							ram  	: <int>,
							disk 	: <int>,
							version : <string>
						} )
	pause				( )								1 or < 0 on error
	resume				( )								1 or < 0 on error
	reset				( )								1 or < 0 on error
	close				( )								1 or < 0 on error
	setProperty			( propertyName, propertyValue ) 0 or < 0 on error
	getProperty			( propertyName )				string
	setExecutionCap		( cap )							0 or < 0 on error

# Appendix C - Properties

The following list of properties exist in a session object:

	Property name		Description
	---------------     -------------------
	ip					The IP address of the VM (not yet supported)
	cpus				The number of cpus
	state				The state of the VM

# Appendix D - State constants

The *sess.state* property has one of the following values:

	State			Description
	------------    -----------------
	0	Closed		The session is closed. You must call close()
	1	Oppening	The open() function was called, but the session is not yet ready
	2	Open		The session is open, you can call start() to start it
	3	Starting	The start() function was called, but the session is not yet started
	4	Started		The VM is running, you can now call pause(), resume(), reset() or stop()
	5	Error		There was an error starting the VM
	6	Paused		The session is open but the VM is paused

# License

CVMWebAPI is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CVMWebAPI is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CVMWebAPI. If not, see <http://www.gnu.org/licenses/>.

Developed by Ioannis Charalampidis, 2013
