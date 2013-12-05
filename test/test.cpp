
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <string>
#include <math.h>

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
#include "Hypervisor/Virtualbox/VBoxInstance.h"

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

void show_pbar( double progress, int charCount ) {
    char *buff = new char[charCount+1];
    memset( buff, '.', charCount );

    int v = (int)( progress * (double)charCount );
    memset( buff, '=', v );
    buff[charCount] = '\0';
    cout << " |" << buff << "| ";
    delete[] buff;
    cout << setiosflags(ios::fixed) << setprecision(2) << (progress * 100.0) << " %" << std::endl;
}

void cb_progress( const std::string& message, const double progress ) {
    cout << "[-----] : " << message << " (" << (progress * 100) << " %)" << endl;
    show_pbar(progress, 80);
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
    pTasks->onFailed(boost::bind(&cb_error, _1, _2));
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

    FiniteTaskPtr pTasks = boost::make_shared<FiniteTask>();

    // Setup callbacks
    pTasks->onStarted( boost::bind(&cb_started, _1) );
    pTasks->onCompleted(boost::bind(&cb_completed, _1));
    pTasks->onFailed(boost::bind(&cb_error, _1, _2));
    pTasks->onProgress(boost::bind(&cb_progress, _1, _2));

    // Install Hypervisor
    //installHypervisor( DownloadProvider::Default(), UserInteraction::Default(), pTasks );

    // Detect and instantiate hypervisor
    HVInstancePtr hv = detectHypervisor();
    if (!hv) {
        pTasks->restart("Installing hypervisor");
        installHypervisor( DownloadProvider::Default(), UserInteraction::Default(), pTasks);
        hv = detectHypervisor();
    }

    pTasks->restart("Creating the VM");
    pTasks->setMax(2);

    if (hv) {
        cout << "HV Binary: " << hv->hvBinary << endl;
        cout << "HV Version: " << hv->version.verString << endl;

        // Wait until hypervisor is ready
        //FiniteTaskPtr pTasksA = pTasks->begin<FiniteTask>("Initializing Hypervisor");
        pTasks->restart("Waiting till VM is ready");
        hv->waitTillReady( pTasks ); // pTasksA );
        
        ParameterMapPtr parm = boost::make_shared<ParameterMap>();
        parm->set("name", "LHC@Home 3.0");
        parm->set("key", "awesome13");
        parm->set("flags", "5");

        //FiniteTaskPtr pTasksB = pTasks->begin<FiniteTask>("Setting-up Session");
        pTasks->restart("Oppening VM Session");
        HVSessionPtr sess = hv->sessionOpen( parm, pTasks );
        if (!sess) {
            cerr << "Session could not be oppened!" << endl;
            return 1;
        }

        // Goto powered off state
        std::map< std::string, std::string > args;
        pTasks->restart("Starting");
        sess->start( &args );
        sleepMs(10000);
        pTasks->restart("Pausing");
        sess->pause();
        sleepMs(10000);
        pTasks->restart("Closing");
        sess->close();
        sleepMs(10000);
        pTasks->restart("Hibernating");
        sess->hibernate();
        sleepMs(20000);
        pTasks->restart("Closing");
        sess->close();

        //cout << boost::static_pointer_cast<VBoxSession>(sess)->getUserData() << endl;

        //sleepMs(1000);
        //hv->loadSessions();

        //boost::static_pointer_cast<VBoxInstance>(hv)->installExtPack( DownloadProvider::Default(),  );
        sleepMs(10000);
        hv->abort();

        
    } else {
        cout << "No hypervisor found!" << endl; 
    }

    return 0;
}
