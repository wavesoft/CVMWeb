
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <string>

#include "Common/Utilities.h"
#include "Common/Hypervisor.h"
#include "Common/ThinIPC.h"
#include "Common/DaemonCtl.h"
#include "Common/DownloadProvider.h"
#include "Common/SimpleFSM.h"
#include "Common/LocalConfig.h"

#include "Hypervisor/Virtualbox/VBoxSession.h"

#include <openssl/rand.h>

using namespace std;


int main( int argc, char ** argv ) {

    std::vector< std::string > keys = LocalConfig::runtime()->enumFiles( "session-" );

    //SessionState    state;
    LocalConfigPtr      config = LocalConfig::forRuntime("session-509a8252-add2-4c2e-a4d1-a00fbbd8e5e5");
    VBoxSession         fsm(config);

    vectorDump(config->enumKeys());
    cout << "----" << endl;
    vectorDump(fsm.userData->enumKeys());
    fsm.userData->clear();
    cout << "Username: " << fsm.userData->get("Name", "(Undefined)") << endl;
    fsm.userData->set("Name", "john");
    config->save();

    // Run
    cout << "* START!" << endl;
    fsm.FSMGoto(7);

    fsm.FSMThreadStart();

    boost::this_thread::sleep(boost::posix_time::milliseconds(4000));

    // Destroy
    cout << "* DESTROY!" << endl;
    fsm.FSMGoto(3);
    boost::this_thread::sleep(boost::posix_time::milliseconds(7000));

    fsm.FSMThreadStop();

    return 0;
}
