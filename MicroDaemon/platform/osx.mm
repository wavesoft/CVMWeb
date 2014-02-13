/**
 * This file is part of CernVM Web API Plugin.
 *
 * CVMWebAPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CVMWebAPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CVMWebAPI. If not, see <http://www.gnu.org/licenses/>.
 *
 * Developed by Ioannis Charalampidis 2013
 * Contact: <ioannis.charalampidis[at]cern.ch>
 */

#import <Foundation/Foundation.h>
#import <Foundation/NSAppleEventManager.h>

// Webserver
#include <MicroDaemon/web/webserver.h>

// Daemon components
#include <MicroDaemon/daemon_core.h>
#include <MicroDaemon/daemon_factory.h>
#include <MicroDaemon/daemon_session.h>

@interface URLDaemon : NSObject {
  // Daemon core component
  @private DaemonCore *      core;
  // Create a factory that manages the daemon sessions
  @private DaemonFactory *   factory;
  // Create a webserver that serves with the daemon factory
  @private CVMWebserver *    webserver;
}

- (void)run;
- (void)poll;
- (void)getUrl:(NSAppleEventDescriptor *)event 
      withReplyEvent:(NSAppleEventDescriptor *)replyEvent;
@end

@implementation URLDaemon

/**
 * Constructor
 */
-(id)init {

  // Create daemon core
  core = new DaemonCore();

  // Create a factory which is going to create the instances
  factory = new DaemonFactory(*core);

  // Create the webserver instance
  webserver = new CVMWebserver(*factory);

  NSLog(@"Daemon initialized");

  return self;
}

/**
 * Destructor
 */
-(void)dealloc {

  // Destruct dynamic objects
  delete webserver;
  delete factory;
  delete core;

  // Continue dealloc
  [super dealloc];
}

/**
 * Polling timer
 */
- (void)poll
{
  // Poll for 100ms
  webserver->poll(100);
}

/**
 *
 */
- (void)run
{

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

  // Start a polling timer
  NSDate *now = [[NSDate alloc] init];
  NSTimer *timer = [[NSTimer alloc] initWithFireDate:now
    interval:.01
    target:self
    selector:@selector(poll)
    userInfo:nil
    repeats:YES];

  NSLog(@"Registered for URL events");

  // Create a runloop and add the polling timer
  NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
  [runLoop addTimer:timer forMode:NSDefaultRunLoopMode];
  NSLog(@"Starting daemon timer");
  [runLoop run];

}

/**
 *
 */
- (void) getUrl:(NSAppleEventDescriptor *)event 
       withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
  // Get the URL
  NSString *urlStr = [[event paramDescriptorForKeyword:keyDirectObject] 
    stringValue];

  NSLog(@"URL Handled: %s", [urlStr UTF8String]);

}

@end

/**
 * Entry point
 */
int main(int argc, const char *argv[]) {
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

  // Instantiate the daemon that can handle URL-based launches
  URLDaemon *daemon = [URLDaemon new];

  // Run daemon
  [daemon run];

  [pool release];
}