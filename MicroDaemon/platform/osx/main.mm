//
//  URLDaemonDelegate.m
//  WebAPI Daemon
//
//  Created by Ioannis Charalampidis on 14/02/14.
//  Copyright (c) 2014 CernVM. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "URLDaemonDelegate.h"

int main(int argc, const char * argv[])
{
	URLDaemonDelegate * delegate = [[URLDaemonDelegate alloc] init];
	[[NSApplication sharedApplication] setDelegate:delegate];
	[NSApp run];
}
