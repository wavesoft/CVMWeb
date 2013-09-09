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

/**********************************************************\

  Auto-generated CVMWebAPISession.cpp

\**********************************************************/

#include "JSObject.h"
#include "variant.h"
#include "variant_list.h"
#include "DOM/Document.h"
#include "global/config.h"

#include "DaemonCtl.h"
#include "Hypervisor.h"
#include "CVMWebAPISession.h"

#include <boost/thread.hpp>

// Helper macro to prohibit the function to be called when the plugin is shutting down
#define  NOT_ON_SHUTDOWN    if (CVMWeb::shuttingDown) return;
#define  WHEN_SAFE          if (!CVMWeb::shuttingDown)

///////////////////////////////////////////////////////////////////////////////
/// @fn CVMWebPtr CVMWebAPISession::getPlugin()
///
/// @brief  Gets a reference to the plugin that was passed in when the object
///         was created.  If the plugin has already been released then this
///         will throw a FB::script_error that will be translated into a
///         javascript exception in the page.
///////////////////////////////////////////////////////////////////////////////
CVMWebPtr CVMWebAPISession::getPlugin()
{
    CRASH_REPORT_BEGIN;
    CVMWebPtr plugin(m_plugin.lock());
    if (!plugin) {
        throw FB::script_error("The plugin is invalid");
    }
    return plugin;
    CRASH_REPORT_END;
}

// Functions
int CVMWebAPISession::pause() {
    CRASH_REPORT_BEGIN;
    
    /* Validate state */
    if (this->session->state != STATE_STARTED) return HVE_INVALID_STATE;

    /* Stop probing timer */
    this->probeTimer->stop();

    /* Make it unavailable */
    if (this->isAlive) {
        this->isAlive = false;
        WHEN_SAFE this->fire_apiUnavailable();
    }
    
    boost::thread t(boost::bind(&CVMWebAPISession::thread_pause, this ));
    return HVE_SCHEDULED;
    CRASH_REPORT_END;
}

void CVMWebAPISession::thread_pause() {
    CRASH_REPORT_BEGIN;
    int ans = this->session->pause();
    if (ans == 0) {
        WHEN_SAFE this->fire_pause();
    } else {
        WHEN_SAFE this->fire_pauseError(hypervisorErrorStr(ans), ans);
        WHEN_SAFE this->fire_error(hypervisorErrorStr(ans), ans, "pause");
    }
    CRASH_REPORT_END;
}

int CVMWebAPISession::close(){
    CRASH_REPORT_BEGIN;
    
    /* Validate state */
    if ((this->session->state != STATE_OPEN) && 
        (this->session->state != STATE_STARTED) && 
        (this->session->state != STATE_PAUSED) &&
        (this->session->state != STATE_ERROR)) return HVE_INVALID_STATE;

    /* Stop probing timer */
    this->probeTimer->stop();

    /* Make it unavailable */
    if (this->isAlive) {
        this->isAlive = false;
        WHEN_SAFE this->fire_apiUnavailable();
    }

    boost::thread t(boost::bind(&CVMWebAPISession::thread_close, this ));
    return HVE_SCHEDULED;
    CRASH_REPORT_END;
}

void CVMWebAPISession::thread_close(){
    CRASH_REPORT_BEGIN;
    int ans = this->session->close();
    if (ans == 0) {
        WHEN_SAFE this->fire_close();
    } else {
        WHEN_SAFE this->fire_closeError(hypervisorErrorStr(ans), ans);
        WHEN_SAFE this->fire_error(hypervisorErrorStr(ans), ans, "close");
    }
    
    /* The needs of having a daemon might have changed */
    CVMWebPtr p = this->getPlugin();
    if (p->hv != NULL) p->hv->checkDaemonNeed();
    
    CRASH_REPORT_END;
}

int CVMWebAPISession::resume(){
    CRASH_REPORT_BEGIN;

    /* Validate state */
    if (this->session->state != STATE_PAUSED) return HVE_INVALID_STATE;

    /* Resume probing timer */
    this->probeTimer->start();
    
    boost::thread t(boost::bind(&CVMWebAPISession::thread_resume, this ));
    return HVE_SCHEDULED;
    CRASH_REPORT_END;
}

void CVMWebAPISession::thread_resume(){
    CRASH_REPORT_BEGIN;
    int ans = this->session->resume();
    if (ans == 0) {
        WHEN_SAFE this->fire_resume();
    } else {
        WHEN_SAFE this->fire_resumeError(hypervisorErrorStr(ans), ans);
        WHEN_SAFE this->fire_error(hypervisorErrorStr(ans), ans, "resume");
    }
    CRASH_REPORT_END;
}

int CVMWebAPISession::reset(){
    CRASH_REPORT_BEGIN;
    
    /* Validate state */
    if (this->session->state != STATE_STARTED) return HVE_INVALID_STATE;
    
    /* Make API unavailable but don't stop the timer */
    if (this->isAlive) {
        this->isAlive = false;
        WHEN_SAFE this->fire_apiUnavailable();
    }
    
    // Start the reset thread
    boost::thread t(boost::bind(&CVMWebAPISession::thread_reset, this ));
    return HVE_SCHEDULED;
    CRASH_REPORT_END;
}

void CVMWebAPISession::thread_reset(){
    CRASH_REPORT_BEGIN;
    int ans = this->session->reset();
    if (ans == 0) {
        WHEN_SAFE this->fire_reset();
    } else {
        WHEN_SAFE this->fire_resetError(hypervisorErrorStr(ans), ans);
        WHEN_SAFE this->fire_error(hypervisorErrorStr(ans), ans, "reset");
    }
    CRASH_REPORT_END;
}

int CVMWebAPISession::stop(){
    CRASH_REPORT_BEGIN;
    
    /* Validate state */
    if (this->session->state != STATE_STARTED) return HVE_INVALID_STATE;

    /* Make it unavailable */
    if (this->isAlive) {
        this->isAlive = false;
        WHEN_SAFE this->fire_apiUnavailable();
    }

    boost::thread t(boost::bind(&CVMWebAPISession::thread_stop, this ));
    return HVE_SCHEDULED;
    CRASH_REPORT_END;
}

void CVMWebAPISession::thread_stop(){
    CRASH_REPORT_BEGIN;
    int ans = this->session->stop();
    if (ans == 0) {
        WHEN_SAFE this->fire_stop();
    } else {
        WHEN_SAFE this->fire_stopError(hypervisorErrorStr(ans), ans);
        WHEN_SAFE this->fire_error(hypervisorErrorStr(ans), ans, "stop");
    }
    CRASH_REPORT_END;
}

int CVMWebAPISession::hibernate(){
    CRASH_REPORT_BEGIN;
    
    /* Validate state */
    if (this->session->state != STATE_STARTED) return HVE_INVALID_STATE;

    /* Make it unavailable */
    if (this->isAlive) {
        this->isAlive = false;
        WHEN_SAFE this->fire_apiUnavailable();
    }

    boost::thread t(boost::bind(&CVMWebAPISession::thread_hibernate, this ));
    return HVE_SCHEDULED;
    CRASH_REPORT_END;
}

void CVMWebAPISession::thread_hibernate(){
    CRASH_REPORT_BEGIN;
    int ans = this->session->hibernate();
    if (ans == 0) {
        WHEN_SAFE this->fire_hibernate();
    } else {
        WHEN_SAFE this->fire_hibernateError(hypervisorErrorStr(ans), ans);
        WHEN_SAFE this->fire_error(hypervisorErrorStr(ans), ans, "hibernate");
    }
    CRASH_REPORT_END;
}

int CVMWebAPISession::open( const FB::variant& o ){
    CRASH_REPORT_BEGIN;

    /* Validate state */
    if ((this->session->state != STATE_CLOSED) && 
        (this->session->state != STATE_ERROR)) return HVE_INVALID_STATE;

    /* Start probing timer */
    this->probeTimer->start();

    boost::thread t(boost::bind(&CVMWebAPISession::thread_open, this, o ));
    return HVE_SCHEDULED;
    CRASH_REPORT_END;
}

/**
 * Helper for thread_open, to check if the user was granted permissions
 * to override particular variables from javascript.
 */
bool __canOverride( const std::string& vname, HVSession * sess ) {
    CRASH_REPORT_BEGIN;
    return (std::find(sess->overridableVars.begin(), sess->overridableVars.end(), vname) != sess->overridableVars.end());
    CRASH_REPORT_END;
}

/**
 * Asynchronous function to open a new session
 */
void CVMWebAPISession::thread_open( const FB::variant& oConfigHash  ){
    CRASH_REPORT_BEGIN;
    int cpus = this->session->cpus;
    int ram = this->session->memory;
    int disk = this->session->disk;
    int flags = this->session->flags;
    std::string ver = this->session->version;
    int ans = 0;
    
    // If the user has provided an object, process overridable parameters
    if (oConfigHash.is_of_type<FB::JSObjectPtr>()) {
        FB::JSObjectPtr o = oConfigHash.cast<FB::JSObjectPtr>();
        
        // Check basic overridable: options
        if (o->HasProperty("cpus")  && __canOverride("cpus", this->session)) cpus = o->GetProperty("cpus").convert_cast<int>();
        if (o->HasProperty("ram")   && __canOverride("ram", this->session))  ram = o->GetProperty("ram").convert_cast<int>();
        if (o->HasProperty("disk")  && __canOverride("disk", this->session)) disk = o->GetProperty("disk").convert_cast<int>();
        
        // Check for overridable: flags
        if (o->HasProperty("flags") && __canOverride("flags", this->session)) {
            try {
                flags = o->GetProperty("flags").convert_cast<int>();
            } catch ( const FB::bad_variant_cast &) {
            }
        }
        
        // Check for overridable: diskURL
        if (o->HasProperty("diskURL") && __canOverride("diskURL", this->session)) {
            ver = o->GetProperty("diskURL").convert_cast<std::string>();
            flags |= HVF_DEPLOYMENT_HDD;
            
            // Check for 64bit image
            bool is64bit = false;
            if (o->HasProperty("cpu64bit")) {
                try {
                    is64bit = o->GetProperty("cpu64bit").convert_cast<bool>();
                } catch ( const FB::bad_variant_cast &) {
                    is64bit = false;
                }
            }
            if (is64bit)
                flags |= HVF_SYSTEM_64BIT;
            
        // Check for overridable: version
        } else if (o->HasProperty("version") && __canOverride("version", this->session)) {
            ver = o->GetProperty("version").convert_cast<std::string>();
            if (!isSanitized(&ver, "01234567890.")) ver=DEFAULT_CERNVM_VERSION;
            flags |= HVF_SYSTEM_64BIT; // Micro is currently only 64-bit

        }
    }
    
    // Open session with the given flags
    ans = this->session->open( cpus, ram, disk, ver, flags );
    if (ans == 0) {
        WHEN_SAFE this->fire_open();
        
    } else {
        
        // Close session in case of a problem
        this->session->close();
        
        // Then fire errors
        WHEN_SAFE this->fire_openError(hypervisorErrorStr(ans), ans);
        WHEN_SAFE this->fire_error(hypervisorErrorStr(ans), ans, "open");
    }
    
    // The requirements of having a daemon might have changed
    CVMWebPtr p = this->getPlugin();
    if (p->hv != NULL) p->hv->checkDaemonNeed();
    
    CRASH_REPORT_END;
}

int CVMWebAPISession::start( const FB::variant& cfg ) {
    CRASH_REPORT_BEGIN;
    
    /* Validate state */
    if (this->session->state != STATE_OPEN) return HVE_INVALID_STATE;
    
    /* Start probing timer */
    this->probeTimer->start();

    boost::thread t(boost::bind(&CVMWebAPISession::thread_start, this, cfg ));
    return HVE_SCHEDULED;
    CRASH_REPORT_END;
}

void CVMWebAPISession::thread_start( const FB::variant& cfg ) {
    CRASH_REPORT_BEGIN;

    int ans;
    std::map<std::string, std::string> vmUserData;
    std::map<std::string, std::string> * dataPtr = NULL;
    
    // Update user data if they are specified
    if ( cfg.is_of_type<FB::JSObjectPtr>() ) {
        
        // Convert object values to key/value pairs
        CVMWA_LOG("Debug", "CFG argument is JSObjectPtr");
        FB::JSObject::GetObjectValues< std::map<std::string, std::string> >(
            cfg.cast<FB::JSObjectPtr>(),
            vmUserData);
        dataPtr = &vmUserData;
        
    }
    
    // Start session
    ans = this->session->start( dataPtr );
    if (ans == 0) {

        // Tell daemon to reload sessions
        if (isDaemonRunning())
            daemonGet( DIPC_RELOAD );

        // Notify browser that the session has started
        WHEN_SAFE this->fire_start();

    } else {
        WHEN_SAFE this->fire_startError(hypervisorErrorStr(ans), ans);
        WHEN_SAFE this->fire_error(hypervisorErrorStr(ans), ans, "start");
    }
    CRASH_REPORT_END;
}

int CVMWebAPISession::setExecutionCap(int cap) {
    CRASH_REPORT_BEGIN;
    return this->session->setExecutionCap( cap );
    CRASH_REPORT_END;
}

int CVMWebAPISession::setProperty( const std::string& name, const std::string& value ) {
    CRASH_REPORT_BEGIN;
    if (name.compare("/CVMWeb/secret") == 0) return HVE_NOT_ALLOWED;
    return this->session->setProperty( name, value );
    CRASH_REPORT_END;
}

std::string CVMWebAPISession::getProperty( const std::string& name ) {
    CRASH_REPORT_BEGIN;
    if (name.compare("/CVMWeb/secret") == 0) return "";
    return this->session->getProperty( name );
    CRASH_REPORT_END;
}

// Read-only parameters
int CVMWebAPISession::get_executionCap() {
    CRASH_REPORT_BEGIN;
    return this->session->executionCap;
    CRASH_REPORT_END;
}

int CVMWebAPISession::get_disk() {
    CRASH_REPORT_BEGIN;
    return this->session->disk;
    CRASH_REPORT_END;
}

int CVMWebAPISession::get_cpus() {
    CRASH_REPORT_BEGIN;
    return this->session->cpus;
    CRASH_REPORT_END;
}

int CVMWebAPISession::get_memory() {
    CRASH_REPORT_BEGIN;
    return this->session->memory;
    CRASH_REPORT_END;
}

int CVMWebAPISession::get_state() {
    CRASH_REPORT_BEGIN;
    return this->session->state;
    CRASH_REPORT_END;
}

int CVMWebAPISession::get_flags() {
    CRASH_REPORT_BEGIN;
    return this->session->flags;
    CRASH_REPORT_END;
}

std::string CVMWebAPISession::get_rdp() {
    CRASH_REPORT_BEGIN;
    return this->session->getRDPHost();
    CRASH_REPORT_END;
}

std::string CVMWebAPISession::get_version() {
    CRASH_REPORT_BEGIN;
    return this->session->version;
    CRASH_REPORT_END;
}

std::string CVMWebAPISession::get_name() {
    CRASH_REPORT_BEGIN;
    return this->session->name;
    CRASH_REPORT_END;
}

std::string CVMWebAPISession::get_ip() {
    CRASH_REPORT_BEGIN;
    return this->session->getAPIHost();
    CRASH_REPORT_END;
}

std::string CVMWebAPISession::get_resolution() {
    CRASH_REPORT_BEGIN;

    // Get screen screen resolution as extra info
    return this->session->getExtraInfo(EXIF_VIDEO_MODE);

    CRASH_REPORT_END;
}

std::string CVMWebAPISession::get_apiEntryPoint() {
    CRASH_REPORT_BEGIN;
    std::string apiURL = this->session->getAPIHost();
    if (apiURL.empty()) return "";
    int apiPort = this->session->getAPIPort();
    if (apiPort == 0) return "";
    apiURL += ":" + ntos<int>( apiPort );
    return apiURL;
    CRASH_REPORT_END;
}

void CVMWebAPISession::onProgress(const size_t v, const size_t tot, const std::string& msg) {
    CRASH_REPORT_BEGIN;
    NOT_ON_SHUTDOWN;
    this->fire_progress(v,tot,msg);
    CRASH_REPORT_END;
}

void CVMWebAPISession::onOpen() {
    CRASH_REPORT_BEGIN;
    NOT_ON_SHUTDOWN;
    this->fire_open();
    CRASH_REPORT_END;
}

void CVMWebAPISession::onError( const std::string& msg, const int code, const std::string& category) {
    CRASH_REPORT_BEGIN;
    NOT_ON_SHUTDOWN;
    this->fire_error(msg, code, category);
    if (category.compare("open") == 0) this->fire_openError(msg, code);
    if (category.compare("start") == 0) this->fire_startError(msg, code);
    if (category.compare("stop") == 0) this->fire_stopError(msg, code);
    if (category.compare("pause") == 0) this->fire_pauseError(msg, code);
    if (category.compare("resume") == 0) this->fire_resumeError(msg, code);
    if (category.compare("reset") == 0) this->fire_resetError(msg, code);
    CRASH_REPORT_END;
}

void CVMWebAPISession::onDebug( const std::string& line ) {
    CRASH_REPORT_BEGIN;
    NOT_ON_SHUTDOWN;
    this->fire_debug( line );
    CRASH_REPORT_END;
}

void CVMWebAPISession::onStart() {
    CRASH_REPORT_BEGIN;
    NOT_ON_SHUTDOWN;
    this->fire_start();
    CRASH_REPORT_END;
}

void CVMWebAPISession::onClose() {
    CRASH_REPORT_BEGIN;
    NOT_ON_SHUTDOWN;
    this->fire_close();
    CRASH_REPORT_END;
}

void CVMWebAPISession::onStop() {
    CRASH_REPORT_BEGIN;
    NOT_ON_SHUTDOWN;
    this->fire_stop();
    CRASH_REPORT_END;
}

std::string CVMWebAPISession::toString() {
    CRASH_REPORT_BEGIN;
    return "[CVMWebAPISession]";
    CRASH_REPORT_END;
}

void CVMWebAPISession::cb_timer() {
    CRASH_REPORT_BEGIN;
    if (!isAlive) {
        if (this->session->state == STATE_STARTED) {
            if (this->session->isAPIAlive( HSK_HTTP )) {
                isAlive = true;
                fire_apiAvailable( this->get_ip(), this->get_apiEntryPoint() );
            }
        }
    } else {
        if ((this->session->state != STATE_STARTED) || (!this->session->isAPIAlive( HSK_HTTP ))) {
            isAlive = false;
            fire_apiUnavailable();
        }
    }
    CRASH_REPORT_END;
}

bool CVMWebAPISession::get_live() {
    CRASH_REPORT_BEGIN;
    return this->isAlive;
    CRASH_REPORT_END;
}

int CVMWebAPISession::get_daemonMinCap() {
    CRASH_REPORT_BEGIN;
    return this->session->daemonMinCap;
    CRASH_REPORT_END;
}

int CVMWebAPISession::get_daemonMaxCap() {
    CRASH_REPORT_BEGIN;
    return this->session->daemonMaxCap;
    CRASH_REPORT_END;
}

bool CVMWebAPISession::get_daemonControlled() {
    CRASH_REPORT_BEGIN;
    return this->session->daemonControlled;
    CRASH_REPORT_END;
}

int CVMWebAPISession::get_daemonFlags() {
    CRASH_REPORT_BEGIN;
    return this->session->daemonFlags;
    CRASH_REPORT_END;
}

void CVMWebAPISession::set_daemonMinCap( int cap ) {
    CRASH_REPORT_BEGIN;
    this->session->daemonMinCap = cap;
    this->setProperty("/CVMWeb/daemon/cap/min", ntos<int>(cap));
    CRASH_REPORT_END;
}

void CVMWebAPISession::set_daemonMaxCap( int cap ) {
    CRASH_REPORT_BEGIN;
    this->session->daemonMaxCap = cap;
    this->setProperty("/CVMWeb/daemon/cap/max", ntos<int>(cap));
    CRASH_REPORT_END;
}

void CVMWebAPISession::set_daemonControlled( bool controled ) {
    CRASH_REPORT_BEGIN;
    this->session->daemonControlled = controled;
    this->setProperty("/CVMWeb/daemon/controlled", (controled ? "1" : "0"));

    /* The needs of having a daemon might have changed */
    CVMWebPtr p = this->getPlugin();
    if (p->hv != NULL) p->hv->checkDaemonNeed();
    CRASH_REPORT_END;
}

void CVMWebAPISession::set_daemonFlags( int flags ) {
    CRASH_REPORT_BEGIN;
    this->session->daemonFlags = flags;
    this->setProperty("/CVMWeb/daemon/flags", ntos<int>(flags));
    CRASH_REPORT_END;
}

/**
 * Update session information from hypervisor session
 */
int CVMWebAPISession::update() {
    CRASH_REPORT_BEGIN;
    if (this->updating) return HVE_STILL_WORKING;
    this->updating = true;
    boost::thread t(boost::bind(&CVMWebAPISession::thread_update, this ));
    return HVE_SCHEDULED;
    CRASH_REPORT_END;
}
void CVMWebAPISession::thread_update() {
    CRASH_REPORT_BEGIN;

    // Don't do anything if we are in the middle of something
    if ((this->session->state == STATE_OPPENING) || (this->session->state == STATE_STARTING)) {
        this->updating = false;
        return;
    }
    
    // Keep old state in order to detect state changes
    int oldState = this->session->state;
    
    // Update session info from driver
    CVMWA_LOG("Debug", "Invoking session update");
    int ans = this->session->update();
    if (ans < 0) {
        CVMWA_LOG("Debug", "Update session returned error = " << ans);
        
        // Check if the session went away
        if (ans == HVE_NOT_FOUND) {
            
            /* Stop probing timer */
            this->probeTimer->stop();

            /* Make it unavailable */
            if (this->isAlive) {
                this->isAlive = false;
                WHEN_SAFE this->fire_apiUnavailable();
            }
            
            /* Session closed */
            WHEN_SAFE this->fire_close();
            
            /* Mark the session closed and exit */
            this->session->state = STATE_CLOSED;
            this->updating = false;
            return;
            
        } else {
            this->updating = false;
            return;
        }
    }
    
    // Check state differences
    CVMWA_LOG("Debug", "Session update completed. Before=" << oldState << ", now=" << this->session->state);
    if (oldState != this->session->state) {
        switch (this->session->state) {
            
            case STATE_OPEN:
                CVMWA_LOG("Debug", "Session switched to OPEN");
            
                /* Start probing timer */
                this->probeTimer->start();
                
                /* Session open */
                //this->fire_open();
                break;
            
            case STATE_STARTED:
                CVMWA_LOG("Debug", "Session switched to STARTED");
            
                /* Start probing timer */
                this->probeTimer->start();
                
                /* Session started */
                WHEN_SAFE this->fire_start();
                break;
            
            case STATE_PAUSED:
                CVMWA_LOG("Debug", "Session switched to PAUSED");

                /* Stop probing timer */
                this->probeTimer->stop();

                /* Make it unavailable */
                if (this->isAlive) {
                    this->isAlive = false;
                    WHEN_SAFE this->fire_apiUnavailable();
                }
    
                /* Session paused */
                WHEN_SAFE this->fire_pause();
                break;
            
            case STATE_ERROR:
                CVMWA_LOG("Debug", "Session switched to ERROR");
                
                // TODO: Am I really using this somewhere? :P :P
                break;
            
        }
    }
    
    CVMWA_LOG("Debug", "Update completed, releasing update flag");
    
    /* Everything was OK */
    this->updating = false;
    
    CRASH_REPORT_END;
}