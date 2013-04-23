#ifndef DialogManager_h__
#define DialogManager_h__
 
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include "BrowserHost.h"
 
namespace FB { class PluginWindow; }

bool CVMConfirmDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, std::string message);

#endif // DialogManager_h__