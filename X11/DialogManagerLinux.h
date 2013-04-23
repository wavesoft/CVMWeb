#ifndef DialogManagerLinux_h__
#define DialogManagerLinux_h__
 
#include <boost/noncopyable.hpp>
#include <string>
 
#include "../DialogManager.h"
 
class DialogManagerLinux : public DialogManager
{
public:
    void OpenFolderDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, const PathCallback& cb);
    bool ConfirmDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, std::string message);
 
protected:
    DialogManagerWin() {};
    ~DialogManagerWin() {};
    friend class DialogManager;
};
#endif // DialogManagerLinux_h__