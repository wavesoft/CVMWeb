#ifndef DialogManagerWin_h__
#define DialogManagerWin_h__
 
#include <boost/noncopyable.hpp>
#include <string>
 
#include "../DialogManager.h"
 
class DialogManagerWin : public DialogManager
{
public:
    void OpenFolderDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, const PathCallback& cb);
    bool ConfirmDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, std::string message);
 
protected:
    DialogManagerWin() {};
    ~DialogManagerWin() {};
    friend class DialogManager;
};
#endif // DialogManagerWin_h__