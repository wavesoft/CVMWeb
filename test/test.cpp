
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <string>

#include <boost/bind.hpp>

#include "Common/Utilities.h"
#include "Common/Hypervisor.h"
#include "Common/ThinIPC.h"
#include "Common/DaemonCtl.h"
#include "Common/DownloadProvider.h"
#include "Common/SimpleFSM.h"
#include "Common/LocalConfig.h"
#include "Common/ProgressFeedback.h"

#include "Hypervisor/Virtualbox/VBoxSession.h"

#include <openssl/rand.h>

using namespace std;

#define _wait(x)    boost::this_thread::sleep(boost::posix_time::milliseconds(x));

void cb_started( const std::string& message ) {
    cout << "[START] : " << message << endl;
}

void cb_completed( const std::string& message ) {
    cout << "[ END ] : " << message << endl;
}

void cb_error( const std::string& message, const int errorCode ) {
    cout << "[ERROR] : " << message << " (#" << errorCode << ")" << endl;
}

void cb_progress( const std::string& message, const double progress ) {
    cout << "[-----] : " << message << " (" << (progress * 100) << " %)" << endl;
}

void doProgress( const ProgressTaskPtr & pTaskPtr = ProgressTaskPtr() );
void doProgress( const ProgressTaskPtr & pTaskPtr) {
    if (!pTaskPtr) {
        cout << "-- NULL PASSED --" << endl;
        return;
    }
    FiniteTaskPtr pTasks = boost::static_pointer_cast<FiniteTask>(pTaskPtr);

    // Setup callbacks
    pTasks->onStarted( boost::bind(&cb_started, _1) );
    pTasks->onCompleted(boost::bind(&cb_completed, _1));
    pTasks->onFailure(boost::bind(&cb_error, _1, _2));
    pTasks->onProgress(boost::bind(&cb_progress, _1, _2));

    // Set max
    pTasks->setMax(5);
    VariableTaskPtr pProgressTask = pTasks->begin<VariableTask>( "Downloading stuff" );
    _wait(100);

    // Do "download"
    pProgressTask->setMessage("Downloading CernVM");
    pProgressTask->setMax(10);
    srand (time(NULL));
    for (int i=0; i<10; i++) {
        pProgressTask->update(i);
        _wait(rand() % 100);
    }
    pProgressTask->complete("Done downloading");

    // Do subtasks
    FiniteTaskPtr pSubTasks = pTasks->begin<FiniteTask>( "Doing many thins" );
    pSubTasks->setMax(3);

    // Do "upload"
    pProgressTask = pSubTasks->begin<VariableTask>( "Uploading stuff" );
    pProgressTask->setMessage("Uploading CernVM");
    pProgressTask->setMax(10);
    srand (time(NULL));
    for (int i=0; i<10; i++) {
        pProgressTask->update(i);
        _wait(rand() % 100);
    }
    pProgressTask->complete("Done uploading");

    // Do "diff"
    pProgressTask = pSubTasks->begin<VariableTask>( "Diffing stuff" );
    pProgressTask->setMessage("Diffing CernVM");
    pProgressTask->setMax(11);
    srand (time(NULL));
    for (int i=0; i<10; i++) {
        pProgressTask->update(i);
        _wait(rand() % 100);
    }
    pProgressTask->fail("Done diffing");

    // Do last task
    pSubTasks->done("Doing my job");
    pSubTasks->complete("Done doing sub-tasks");

    // Do other tasks
    pTasks->done("Setting-up environment");
    _wait(500);
    pTasks->done("Creating VM");
    _wait(400);
    pTasks->doing("Finalizing");
    _wait(1000);
    pTasks->done("Completed finalization");
    pTasks->complete("Done");

}

int main( int argc, char ** argv ) {

    /*
    FiniteTaskPtr pTasks = boost::make_shared<FiniteTask>( );    

    doProgress();
    doProgress(pTasks);
    */

    /*
    HypervisorVersion hvv("1.3.4r10031");
    cout << "Major=" << hvv.major << endl;
    cout << "Minor=" << hvv.minor << endl;
    cout << "Build=" << hvv.build << endl;
    cout << "Revision=" << hvv.revision << endl;
    cout << "Misc=" << hvv.misc << endl;
    cout << "All=" << hvv.verString << endl;

    cout << endl;

    cout << "Comparison=" << hvv.compareStr("1.3.4r10030") << endl;
    */

    /*
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
    */

    HVInstancePTr hv = detectHypervisor();

    return 0;
}
