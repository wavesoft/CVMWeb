#include <string>
#include <boost/thread.hpp>
#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>
#include "logging.h"
 
#include "DialogManagerMac.h"
#include "BrowserHost.h"
 
DialogManager* DialogManager::get()
{
    static DialogManagerMac inst;
    return &inst;
}

void DialogManagerMac::OpenFolderDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, const PathCallback& cb)
{
    host->ScheduleOnMainThread(boost::shared_ptr<DialogManagerMac>(), boost::bind(&DialogManagerMac::_showFolderDialog, this, win, cb));
}

bool DialogManagerMac::ConfirmDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, std::string message)
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