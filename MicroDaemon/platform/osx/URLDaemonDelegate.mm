//
//  URLDaemonDelegate.m
//  WebAPI Daemon
//
//  Created by Ioannis Charalampidis on 14/02/14.
//  Copyright (c) 2014 CernVM. All rights reserved.
//

#import "URLDaemonDelegate.h"

@implementation URLDaemonDelegate

/**
 * State to register event manager
 */
- (void)applicationWillFinishLaunching:(NSNotification *)aNotification
{
	// Create the C++ daemon core
	core = new DaemonCore();
	// Create a factory which is going to create the instances
	factory = new DaemonFactory(*core);
	// Create the webserver instance
	webserver = new CVMWebserver(*factory);

	// Handle URL
	NSAppleEventManager *em = [NSAppleEventManager sharedAppleEventManager];
	[em 
		setEventHandler:self 
		andSelector:@selector(getUrl:withReplyEvent:) 
		forEventClass:kInternetEventClass 
		andEventID:kAEGetURL];  
	[em 
		setEventHandler:self 
		andSelector:@selector(getUrl:withReplyEvent:) 
		forEventClass:'WWW!' 
		andEventID:'OURL'];

	// Daemon components are ready
	NSLog(@"Daemon initialized");

}

/**
 * Application launch phase completed
 */
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{

	// Start a polling timer
	timer = [NSTimer 
		scheduledTimerWithTimeInterval:.01 
		target:self
		selector:@selector(serverStep)
		userInfo:nil
		repeats:YES];

	// Reap timer
	reapTimer = [NSTimer 
		scheduledTimerWithTimeInterval:1
		target:self
		selector:@selector(serverReap)
		userInfo:nil
		repeats:YES];

	NSLog(@"Timer started");

}

/**
 * Polling timer
 */
- (void)serverStep
{
	// Poll for 100ms
	webserver->poll(100);
}

/**
 * Reap server if there are no active connections
 */
- (void)serverReap
{
	if (!webserver->hasLiveConnections()) {
		NSLog(@"Reaping server");
		[NSApp terminate: nil];
	}
}

/**
 * Handle URL requests
 */
- (void) getUrl:(NSAppleEventDescriptor *)event 
	   withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
	// Get the URL
	NSString *urlStr = [[event paramDescriptorForKeyword:keyDirectObject] 
		stringValue];

	NSLog(@"URL Handled: %s", [urlStr UTF8String]);
}

/**
 * Cleanup objects at shutdown
 */
- (void)applicationWillTerminate:(NSNotification *)aNotification
{

	NSLog(@"Cleaning-up daemon");

	// Stop timer
	[timer invalidate];
	[reapTimer invalidate];

	// Destruct webserver components
	delete webserver;
	delete factory;
	delete core;

}

@end
