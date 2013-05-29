
# What is this?

This is an NPAPI plugin for web browsers that enables real-time interaction with the CernVM guest OS. 
This plugin identifies the hypervisors installed in the system and picks the most appropriate one to
host the CernVM Guest OS. 

It then provides a simple interface to start/stop the VM and access it's data sets. 

# Testing the plugin

You can test the installation and the status of the plugin using the *test.html* file that comes with the source.

# Building the plugin

CVMWebAPI Plugin is built using the [FireBrath](http://www.firebreath.org/) framework. So, your very first step is to
download the latest stable release of FireBreath : http://www.firebreath.org/display/documentation/Download

Then create a 'projects' folder inside the extracted files:

	wget https://github.com/firebreath/FireBreath/tarball/firebreath-1.7
	tar -zxf firebreath-FireBreath-firebreath-1.7.0-8-gb73d799.tar.gz
	cd firebreath-FireBreath-df8659e
	mkdir projects

Then clone the plugin GIT repository inside the projects folder:

	git clone https://github.com/wavesoft/CVMWeb.git CVMWeb

You can then follow the appropriate instructions for:

 * [How to build on OSX](http://www.firebreath.org/display/documentation/Building+on+Mac+OS+X)
 * [How to build on Linux](http://www.firebreath.org/display/documentation/Building+on+Linux)
 * [How to build on Windows](http://www.firebreath.org/display/documentation/Building+on+Windows)

# Installing the Plugin

The plugin has no installation system yet, but you can install it manually using one of the following techniques.
You can always find the latest binary builds for your operating system in the *builds* directory.

## Installing on Mac OSX

Copy the plugin from the **build/projects/CVMWeb/Debug** or **build/projects/CVMWeb/Release** directory to your 
**Library/Internet Plug-Ins** directory. The plugin will be immediately available to all of your browsers.

## Installation on Windows

Copy the .dll to a directory that it's going to stay for the rest of it's time (ex. C:\Program Files\Common Files\CernVM) and
then run:

	regsvr32 C:\Program Files\Common Files\CernVM\npCVMWeb.dll

## Installation on Linux

Copy the plugin from the **build/bin/CVMWeb** directory to your **~/.mozilla/plugins** directory. The plugin will be immediately 
available to Firefox.

## Installation on Firefox

**NEW** Now the plugin is available as an .xpi plugin! (Check the builds folder). To install it, simply drag it onto a firefox
instance...

The .xpi plugin contains precompiled binaries for the following platforms:

 * Darwin/GCC3 x86 ( Mac OSX 10.2+ )
 * Darwin/GCC3 x86_64 ( Mac OSX 10.6+ )
 * Linux/GCC3 x86 (Also works on x86_64)
 * Windows XP/7 32bit (Also works on 64-bit)

The only linux platform currently tested was Ubuntu 12.04.2 LTS (32 bit) but in principle it should work on many more...

# How to use the plugin

To use this plugin you must first create an embed element in your website. You must give it a unique ID:

	<embed type="application/x-cvmweb" id="cvmweb">

You can then get a reference to the object and access it's properties:

	var o = document.getElementById('cvmweb');
	alert('We are running with ' + o.hypervisorName + "/" + e.hypervisorVersion );

# Creating sessions

Every computation session is a CernVM instance and is accessed individually. You can request a session that fits your needs
using the requestSession function:

	var res = o.requestSession( "session name", "secret key", success_callback, failure_callback );

The secret key prohibits unauthorized users to access your session. The first user that requests a session is automatically
the owner and only his secret key can be used to request access to the same session again.

This function will return 1 (HVE_SCHEDULED) if succeeded or an error code if failed. When the session is ready, the function
provided by the *success_callback* parameter will be called. The callback signature is the following:

	// Called when session is ready
	success_callback( session ) {
		...
	}
	
	// Called if an error occured
	failure_callback( error_code ) {
		...
	}

Upon receiving the session object you can bind your event listeners using the *addEventListener(event, callback)* function. 
Check appendix B for the events that you can handle.

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

The plugin will probe the CernVM communication channel periodically. When the channel is established the *onApiAvailable* event will
be fired. When a live connection is interrupted, the *onApiUnavailable* event will be fired.

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

	sess.pause();		// Pause session
	sess.resume();		// Resume session
	sess.stop();		// Power of the VM 
	sess.hibernate();	// Save state and shut down the VM
	sess.reset();		// Hard-reset the VM

All these functions will also fire the *onPause*, *onResume*, *onStop*, *onHibernate* and *onReset* events, respectively.
If an error occurs the *onPauseError*, *onResumeError*, *onStopError*, *onHibernateError* and *onResetError* events will be fired.
The function will always return 1 unless the VM is not in the appropriate state for the operation, where an 
*HVE_INVALID_STATE (-8)* will be returned.

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


# Assisting daemon process

Since the running VM needs to be calibrated at real-time, depending on the CPU load or other factors, a tiny daemon process
is started along with every VM. 

You can learn more about the daemon if you check the README.md in the daemon/ folder.

## Checking the daemon from javascript

If you want to monitor the status of the daemon you have to request a daemon access. To do so, use the *requestDaemonAccess()* function
on the core object:

	var o = document.getElementById('cvmweb'),
		daemon = o.requestDaemonAccess();

The function will either return a *CVMWebAPIDaemon* instance or an numeric error code if something went wrong.

If the website is not in a privileged domain you can only access the following properties:

 * **.isRunning** (Read-only) : Returns true if the daemon is running.
 * **.isSystemIdle** (Read-only) : Return true if the user's system is idle.
 * **.idleTime** (Read-only) : Returns the amount of time the daemon should wait before entering the idle state.

If you are in a privileged domain you get additionally the following:

 * **.idleTime** (Read-Write) : You can change for how long the daemon should wait
 * **.path** (Read-only) : The location of the daemon, as detected by the plugin
 * **.start()** : Manually start the daemon
 * **.stop()** : Manually stop the daemon

## Opting-in for daemon management

In order to control a session with the daemon you must set it's *.daemonControlled* property to *true*. You can then use the *.daemonMinCap*, *.daemonMaxCap*
and *.daemonFlags* in order to define how the daemon will control the session. For example:

	session.daemonControlled = true;	// Enable daemon on this session
	session.daemonMinCap = 25;			// When the system is active, the execution cap will be at 25%
	session.daemonMaxCap = 100;			// When the system is idle, the executionc ap will be at 100%

You can use the following combinations in order to achieve different control options:

<table>
  <tr>
  	<th>.daemonMinCap</th>
	<th>.daemonMaxCap</th>
	<th>.daemonFlags</th>
	<th>What happens</th>
  </tr>
  <tr><td>&gt; 0</td><td>1 - 100</td><td>0</td><td>The VM is always running and it's execution cap is changed based on system's state.</td></tr>
  <tr><td>0</td><td>(ignored)</td><td>0</td><td>The VM is paused when the system is active and resumed when idle.</td></tr>
  <tr><td>0</td><td>(ignored)</td><td>1</td><td>The VM is hibernated when the system is active and started when idle.</td></tr>
</table>

The *.daemonFlags* is a bitmask integer that defines how the daemon will control the VM. Currently the following bits are used:

<table>
  <tr>
  	<th>Bit</th>
  	<th>Hex value</th>
  	<th>Name</th>
  	<th>Description</th>
  </tr>
  <tr><td>0</td><td>0x0001</td><td>DF_SUSPEND</td><td>Suspend the VM instead of pausing it when .daemonMinCap is 0.</td></tr>
  <tr><td>1</td><td>0x0002</td><td>DF_AUTOSTART</td><td>Automatically start the VM if it's shut down.</td></tr>
</table>

# Security considerations

It is very important to note that this is an NPAPI plugin, and therefore not protected by any browser sandbox! It can directly access your entire system!

Extra care was taken to minimize the parameters that the user can specify and that can reach a system exec() command. 
Only the **openSession()** function is currently passing (sanitized) integer arguments to the VBoxManage command. 

In addition and in order to protect the user from unauthorized VM initiations a system-modal dialog will be appeared every time a website calls the **openSession()** function.
If the user rejects the session initiation twice within 5 seconds, no further dialogs will be displayed and all the requests will be rejected.

There is no other (known) vulnerabilities that an attacker can exploit, but be aware!
**Accept VM initiations only from domains that you trust and only after they told you that they are going to do so!**

---------------------------------------

# Appendix A - Error codes

Most of the functions return a number. If the value is less than zero, an error has occurred. Here is a short list of 
the errors that can occur.

<table>
  <tr>
  	<th>Return Value</th>
	<th>Description</th>
  </tr>
  <tr><td>1</td><td>Successfuly scheduled (status not known yet)</td></tr>
  <tr><td>0</td><td>No error</td></tr>
  <tr><td>-1</td><td>Creation Error</td></tr>
  <tr><td>-2</td><td>Modification Error</td></tr>
  <tr><td>-3</td><td>Control Error</td></tr>
  <tr><td>-4</td><td>Deletion Error</td></tr>
  <tr><td>-5</td><td>Query Error</td></tr>
  <tr><td>-6</td><td>I/O Error</td></tr>
  <tr><td>-7</td><td>External library / server error</td></tr>
  <tr><td>-8</td><td>Not a valid operation for the current state</td></tr>
  <tr><td>-9</td><td>Not found</td></tr>
  <tr><td>-10</td><td>Not allowed</td></tr>
  <tr><td>-11</td><td>Not supported</td></tr>
  <tr><td>-12</td><td>Not validated</td></tr>
  <tr><td>-13</td><td>Not trusted</td></tr>
  <tr><td>-20</td><td>Password denied</td></tr>
  <tr><td>-99</td><td>Invalid usage</td></tr>
  <tr><td>-100</td><td>Function is not implemented</td></tr>
</table>

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
	hibernate			( )
	openError			( errorMessage, errorCode )
	startError			( errorMessage, errorCode )
	closeError			( errorMessage, errorCode )
	pauseError			( errorMessage, errorCode )
	resumeError			( errorMessage, errorCode )
	stopError			( errorMessage, errorCode )
	hibernateError		( errorMessage, errorCode )
	progress			( tasksCompleted, tasksPending, statusMessage )
	errror				( errorMessage, errorCode, errorCategory )
	apiAvailable		( machineIP, apiURL )
	apiUnavailable		( )
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
	hibernate			( )								1 or < 0 on error
	setProperty			( propertyName, propertyValue ) 0 or < 0 on error
	getProperty			( propertyName )				string
	setExecutionCap		( cap )							0 or < 0 on error

# Appendix C - Properties

The following list of properties exist in the session objects:

	Property name		Description
	---------------     -------------------
	ip					The IP address of the VM (not yet supported)
	cpus				The number of cpus
	state				The state of the VM
	ram					The ammount of memory (in MB) used
	disk 				The ammount of memory (in MB) allocated for the disk
	version				The version of CernVM-Micro used
	executionCap		The current execution cap
	apiURL				The URL inside the VM to contact for API operations
	rdpURL				The hostname and the port for the display
	daemonControlled	TRUE if the session is managed by the daemon
	daemonMinCap		The min (active) cap of the VM
	daemonMaxCap		The max (idle) cap of the VM
	daemonFlags			Additional flags to define how the daemon will control the VM

# Appendix D - State constants

The *sess.state* property has one of the following values:

	State			Description
	------------    -----------------
	0	Closed		The session is closed. You must call open()
	1	Oppening	The open() function was called, but the session is not yet ready
	2	Open		The session is open, you can call start() to start it
	3	Starting	The start() function was called, but the session is not yet started
	4	Started		The VM is running, you can now call pause(), resume(), reset() or stop()
	5	Error		There was an error starting the VM
	6	Paused		The session is open but the VM is paused

---------------------------------------

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
