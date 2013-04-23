#include <string>
#include <boost/thread.hpp>
#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>
#include "logging.h"
 
#include "../Dialogs.h"
#include "BrowserHost.h"

bool CVMConfirmDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, std::string message)
{
	NSString *confirmMessage = [NSString stringWithUTF8String:message.c_str()];

	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert addButtonWithTitle:@"Yes"];
	[alert addButtonWithTitle:@"No"];
    [alert setAlertStyle:NSInformationalAlertStyle];
    [alert setMessageText:@"CernVM Web API"];
    [alert setInformativeText: confirmMessage ];

	int ans = [alert runModal];
	return (ans == NSAlertFirstButtonReturn);
}