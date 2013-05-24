
# CernVM Web API - Auto-tuning daemon process

The auto-tuning daemon process is a tiny background daemon process that monitors the system inactivity. When the user is active, it
lowers the VM priority or it pauses the machine. When the user is idle, it starts the VM.

## Supported features

Currently, the daemon supports the following features:

 * Simple idle detection, using the time of the last user input
 * Automatic pausing of the VMs when the user is active
 * Automatic resuming of the VMs when the user is idle

## Planned features

In addition to the above, basic features, the following features are planned:

 * Platform-dependent resource monitoring
 * Changing VM execution cap depending on system load
 * Shutting down VMs when the system resources are exhausted

# Controlling the daemon

The daemon is not started automatically. It must be started using the javascript API:

	// (Assuming variable 'core' contains a reference 
	// to the plugin element)
	
	// Get a reference to the daemon object
	var daemon = core.requestDaemonAccess;
	
	// You can check if the daemon is running
	// using the running property:
	if (!daemon.running) {
		
		// You can start the daemon using the start() function.
		daemon.start();
		
	}
	
	