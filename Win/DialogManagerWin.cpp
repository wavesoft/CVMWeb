
#include "win_common.h"
#include <commdlg.h>
#include <string>
#include <boost/thread.hpp>
#include "utf8_tools.h"
#include "Win/PluginWindowlessWin.h"
#include "Win/PluginWindowWin.h"
 
#include "DialogManagerWin.h"
#include <shlobj.h>
 
DialogManager* DialogManager::get()
{
    static DialogManagerWin inst;
    return &inst;
}

bool ConfirmDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, std::string message) {
    FB::PluginWindowWin* wndWin = dynamic_cast<FB::PluginWindowWin*>(win);
    FB::PluginWindowlessWin* wndlessWin = dynamic_cast<FB::PluginWindowlessWin*>(win);
 
    HWND browserWindow = wndWin ? wndWin->getBrowserHWND() : wndlessWin->getHWND();
}
