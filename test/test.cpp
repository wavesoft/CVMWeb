
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

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

    //SessionState    state;
    LocalConfigPtr      config = LocalConfig::forRuntime("session-test");
    VBoxSession         fsm(config);

    // Run
    cout << "* START!" << endl;
    fsm.FSMGoto(7);

    fsm.FSMThreadStart();

    Sleep(4000);

    // Destroy
    cout << "* DESTROY!" << endl;
    fsm.FSMGoto(3);
    Sleep(7000);

    fsm.FSMThreadStop();

    return 0;
}
