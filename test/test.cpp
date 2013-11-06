
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

#include "Utilities.h"
#include "Virtualbox.h"
#include "Hypervisor.h"
#include "ThinIPC.h"
#include "DaemonCtl.h"
#include "DownloadProvider.h"
#include "SimpleFSM.h"

#include <openssl/rand.h>

using namespace std;

class SessionState {
public:

};

class VBoxSession : public HVSession {
public:

    virtual int             pause               ();
    virtual int             close               ( bool unmonitored = false );
    virtual int             resume              ();
    virtual int             reset               ();
    virtual int             stop                ();
    virtual int             hibernate           ();
    virtual int             open                ( int cpus, int memory, int disk, std::string cvmVersion, int flags );
    virtual int             start               ( std::map<std::string,std::string> *userData );
    virtual int             setExecutionCap     ( int cap);
    virtual int             setProperty         ( std::string name, std::string key );
    virtual std::string     getProperty         ( std::string name, bool forceUpdate = false );
    virtual std::string     getRDPHost          ();
    virtual std::string     getExtraInfo        ( int extraInfo );
    virtual std::string     getAPIHost          ();
    virtual int             getAPIPort          ();

    virtual int             update              ();
    virtual int             updateFast          ();

};

class VBoxFSM : public SimpleFSM, public HVSession {
public:

    VBoxFSM() {

        FSM_REGISTRY(1,             // Entry point is on '1'
        {

            // Target states
            FSM_STATE(1, 100);      // Entry point
            FSM_STATE(2, 102);      // Error
            FSM_STATE(3, 104);      // Destroyed
            FSM_STATE(4, 105,108);  // Power off
            FSM_STATE(5, 107,108);  // Saved
            FSM_STATE(6, 109,111);  // Paused
            FSM_STATE(7, 110,106);  // Running

            // 100: INITIALIZE HYPERVISOR
            FSM_HANDLER(100, &VBoxFSM::Initialize,              101);

            // 101: UPDATE SESSION STATE FROM THE HYPERVISOR
            FSM_HANDLER(101, &VBoxFSM::UpdateSession,           2,3,4,5,6,7);

            // 102: HANDLE ERROR SEQUENCE
            FSM_HANDLER(102, &VBoxFSM::HandleError,             103);
                FSM_HANDLER(103, &VBoxFSM::CureError,           101);       // Try to recover error and recheck state

            // 104: CREATE SEQUENCE
            FSM_HANDLER(104, &VBoxFSM::CreateVM,                201);       // Allocate VM
                FSM_HANDLER(201, &VBoxFSM::ConfigureVM,         202);       // Configure VM
                FSM_HANDLER(202, &VBoxFSM::DownloadMedia,       203);       // Download required media files
                FSM_HANDLER(203, &VBoxFSM::ConfigureVMBoot,     204);       // Configure Boot media
                FSM_HANDLER(204, &VBoxFSM::ConfigureVMScratch,  4);         // Configure Scratch storage

            // 105: DESTROY SEQUENCE
            FSM_HANDLER(105, &VBoxFSM::ReleaseVMScratch,        207);       // Release Scratch storage
                FSM_HANDLER(207, &VBoxFSM::ReleaseVMBoot,       208);       // Release Boot Media
                FSM_HANDLER(208, &VBoxFSM::DestroyVM,           3);         // Configure API Disks

            // 106: POWEROFF SEQUENCE
            FSM_HANDLER(106, &VBoxFSM::PoweroffVM,              209);       // Power off the VM
                FSM_HANDLER(209, &VBoxFSM::ReleaseVMAPI,        4);         // Release the VM API media

            // 107: DISCARD STATE SEQUENCE
            FSM_HANDLER(107, &VBoxFSM::DiscardVMState,          209);       // Discard saved state of the VM
                FSM_HANDLER(209, &VBoxFSM::ReleaseVMAPI,        4);         // Release the VM API media

            // 108: START SEQUENCE
            FSM_HANDLER(108, &VBoxFSM::PrepareVMBoot,           205);       // Prepare start parameters
                FSM_HANDLER(205, &VBoxFSM::ConfigureVMAPI,      206);       // Configure API Disks
                FSM_HANDLER(206, &VBoxFSM::StartVM,             7);         // Launch the VM

            // 109: SAVE STATE SEQUENCE
            FSM_HANDLER(109, &VBoxFSM::SaveVMState,             5);         // Save VM state

            // 110: PAUSE SEQUENCE
            FSM_HANDLER(110, &VBoxFSM::PauseVM,                 6);         // Pause VM

            // 111: PAUSE SEQUENCE
            FSM_HANDLER(111, &VBoxFSM::ResumeVM,                7);         // Resume VM

        });

    }

    /**
     * Initialize connection with VirtualBox
     */
    void Initialize() {
        cout << "- Initialize" << endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    }

    /**
     * Load VirtualBox session 
     */
    void UpdateSession() {
        cout << "- UpdateSession" << endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(200));
        FSMSkew(3);
    }

    /**
     * Handle errors
     */
    void HandleError() {
        cout << "- HandleError" << endl;
    }

    /**
     * Cure errors
     */
    void CureError() {
        cout << "- CureError" << endl;
    }

    /**
     * Create new VM
     */
    void CreateVM() {
        cout << "- CreateVM" << endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    }

    void ConfigureVM() {
        cout << "- ConfigureVM" << endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    }

    void DownloadMedia() {
        cout << "- DownloadMedia" << endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
    }

    void ConfigureVMBoot() {
        cout << "- ConfigureVMBoot" << endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    }

    void ReleaseVMBoot() {
        cout << "- ReleaseVMBoot" << endl;
    }

    void ConfigureVMScratch() {
        cout << "- ConfigureVMScratch" << endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    }

    void ReleaseVMScratch() {
        cout << "- ReleaseVMScratch" << endl;
    }

    void ConfigureVMAPI() {
        cout << "- ConfigureVMAPI" << endl;
    }

    void ReleaseVMAPI() {
        cout << "- ReleaseVMAPI" << endl;
    }

    void PrepareVMBoot() {
        cout << "- PrepareVMBoot" << endl;
    }

    void DestroyVM() {
        cout << "- DestroyVM" << endl;
    }

    void PoweroffVM() {
        cout << "- PoweroffVM" << endl;
    }

    void DiscardVMState() {
        cout << "- DiscardVMState" << endl;
    }

    void StartVM() {
        cout << "- StartVM" << endl;
    }

    void SaveVMState() {
        cout << "- SaveVMState" << endl;
    }

    void PauseVM() {
        cout << "- PauseVM" << endl;
    }

    void ResumeVM() {
        cout << "- ResumeVM" << endl;
    }

};

int main( int argc, char ** argv ) {

    //SessionState    state;
    VBoxFSM         fsm;

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
