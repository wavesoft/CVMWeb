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

#include "../Dialogs.h"
#include <Windows.h>
#include <Winuser.h>

bool CVMConfirmDialog(const FB::BrowserHostPtr& host, FB::PluginWindow* win, std::string message) {

    /* Convert to WString */
    std::wstring ws;
    ws.assign(message.begin(), message.end());
    
    /* Display message box */
    int msgboxID = MessageBox(
        NULL,
        (LPCWSTR)ws.c_str(),
        (LPCWSTR)L"CernVM Web API",
        MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2 | MB_APPLMODAL | MB_TOPMOST
    );
    
    /* Return value */
    return ( msgboxID == IDYES );
}
