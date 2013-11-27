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

#pragma once
#ifndef VBOXSESSION_H
#define VBOXSESSION_H

#include "VBoxCommon.h"

#include <string>
#include <map>

#include "Common/SimpleFSM.h"
#include "Common/Hypervisor.h"
#include "Common/CrashReport.h"

#include <boost/regex.hpp>

/**
 * Virtualbox Session, built around a Finite-State-Machine model
 */
class VBoxSession : public SimpleFSM, public HVSession {
public:

    VBoxSession( ParameterMapPtr param, HVInstancePtr hv ) : SimpleFSM(), HVSession(param, hv) {

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
            FSM_HANDLER(100, &VBoxSession::Initialize,              101);

            // 101: UPDATE SESSION STATE FROM THE HYPERVISOR
            FSM_HANDLER(101, &VBoxSession::UpdateSession,           2,3,4,5,6,7);

            // 102: HANDLE ERROR SEQUENCE
            FSM_HANDLER(102, &VBoxSession::HandleError,             103);
                FSM_HANDLER(103, &VBoxSession::CureError,           101);       // Try to recover error and recheck state

            // 104: CREATE SEQUENCE
            FSM_HANDLER(104, &VBoxSession::CreateVM,                201);       // Allocate VM
                FSM_HANDLER(201, &VBoxSession::ConfigureVM,         202);       // Configure VM
                FSM_HANDLER(202, &VBoxSession::DownloadMedia,       203);       // Download required media files
                FSM_HANDLER(203, &VBoxSession::ConfigureVMBoot,     204);       // Configure Boot media
                FSM_HANDLER(204, &VBoxSession::ConfigureVMScratch,  4);         // Configure Scratch storage

            // 105: DESTROY SEQUENCE
            FSM_HANDLER(105, &VBoxSession::ReleaseVMScratch,        207);       // Release Scratch storage
                FSM_HANDLER(207, &VBoxSession::ReleaseVMBoot,       208);       // Release Boot Media
                FSM_HANDLER(208, &VBoxSession::DestroyVM,           3);         // Configure API Disks

            // 106: POWEROFF SEQUENCE
            FSM_HANDLER(106, &VBoxSession::PoweroffVM,              209);       // Power off the VM
                FSM_HANDLER(209, &VBoxSession::ReleaseVMAPI,        4);         // Release the VM API media

            // 107: DISCARD STATE SEQUENCE
            FSM_HANDLER(107, &VBoxSession::DiscardVMState,          209);       // Discard saved state of the VM
                FSM_HANDLER(209, &VBoxSession::ReleaseVMAPI,        4);         // Release the VM API media

            // 108: START SEQUENCE
            FSM_HANDLER(108, &VBoxSession::PrepareVMBoot,           205);       // Prepare start parameters
                FSM_HANDLER(205, &VBoxSession::ConfigureVMAPI,      206);       // Configure API Disks
                FSM_HANDLER(206, &VBoxSession::StartVM,             7);         // Launch the VM

            // 109: SAVE STATE SEQUENCE
            FSM_HANDLER(109, &VBoxSession::SaveVMState,             5);         // Save VM state

            // 110: PAUSE SEQUENCE
            FSM_HANDLER(110, &VBoxSession::PauseVM,                 6);         // Pause VM

            // 111: PAUSE SEQUENCE
            FSM_HANDLER(111, &VBoxSession::ResumeVM,                7);         // Resume VM

        });

    }

    /////////////////////////////////////
    // FSM implementation functions 
    /////////////////////////////////////

    void Initialize();
    void UpdateSession();
    void HandleError();
    void CureError();
    void CreateVM();
    void ConfigureVM();
    void DownloadMedia();
    void ConfigureVMBoot();
    void ReleaseVMBoot();
    void ConfigureVMScratch();
    void ReleaseVMScratch();
    void ConfigureVMAPI();
    void ReleaseVMAPI();
    void PrepareVMBoot();
    void DestroyVM();
    void PoweroffVM();
    void DiscardVMState();
    void StartVM();
    void SaveVMState();
    void PauseVM();
    void ResumeVM();

    /////////////////////////////////////
    // HVSession Implementation
    /////////////////////////////////////

    virtual int             pause               ();
    virtual int             close               ( bool unmonitored = false );
    virtual int             resume              ();
    virtual int             reset               ();
    virtual int             stop                ();
    virtual int             hibernate           ();
    virtual int             open                ();
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

    /////////////////////////////////////
    // External updates feedback
    /////////////////////////////////////

    /**
     * Notification from the VBoxInstance that the session
     * has been destroyed from an external source.
     */
    void                    hvNotifyDestroyed   ();

    /**
     * Notification from the VBoxInstance that we are going
     * for a forceful shutdown. We should cleanup everything
     * without raising any alert during the handling.
     */
    void                    hvStop              ();

    /**
     *  Compile the user data and return it's string representation
     */
    std::string             getUserData         ();

protected:

    /////////////////////////////////////
    // Tool functions
    /////////////////////////////////////

    /**
     * Execute the specified command using the hypervisor binary as base
     */
    int                     wrapExec            ( std::string cmd, 
                                                  std::vector<std::string> * stdoutList, 
                                                  std::string * stderrMsg = NULL, 
                                                  int retries = 4, 
                                                  int timeout = SYSEXEC_TIMEOUT );


    int                     getMachineUUID      ( std::string mname, std::string * ans_uuid,  int flags );
    std::string             getDataFolder       ();
    std::string             getHostOnlyAdapter  ();
    std::map<std::string, 
        std::string>        getMachineInfo      ( int timeout = SYSEXEC_TIMEOUT );
    int                     startVM             ();
    int                     controlVM           ( std::string how, int timeout = SYSEXEC_TIMEOUT );

    ////////////////////////////////////
    // Local variables
    ////////////////////////////////////
    
    std::string             dataPath;
    bool                    updateLock;

    ////////////////////////////////////
    // State variables
    // ----------------
    // All the following variables must
    // be saved/restored on context 
    // switches.
    ////////////////////////////////////

    /* RDP and API Ports */
    int                     rdpPort;
    int                     localApiPort;
    
    /* Offline properties map (for optimizing performance) */
    std::map<
        std::string,
        std::string >       properties;
    std::map<
        std::string,
        std::string >       unsyncedProperties;

};


#endif /* end of include guard: VBOXSESSION_H */
