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
    CVMWebPtr plugin(m_plugin.lock());
    if (!plugin) {
        throw FB::script_error("The plugin is invalid");
    }
    return plugin;
}

// Functions
int CVMWebAPISession::pause() {
    
    /* Validate state */
    if (this->session->state != STATE_STARTED) return HVE_INVALID_STATE;

    /* Stop probing timer */
    this->probeTimer->stop();

    /* Make it unavailable */
    if (this->isAlive) {
        this->isAlive = false;
        this->fire_apiUnavailable();
    }
    
    boost::thread t(boost::bind(&CVMWebAPISession::thread_pause, this ));
    return HVE_SCHEDULED;
}

void CVMWebAPISession::thread_pause() {
    int ans = this->session->pause();
    if (ans == 0) {
        this->fire_pause();
    } else {
        this->fire_pauseError(hypervisorErrorStr(ans), ans);
        this->fire_error(hypervisorErrorStr(ans), ans, "pause");
    }
}

int CVMWebAPISession::close(){
    
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
        this->fire_apiUnavailable();
    }

    boost::thread t(boost::bind(&CVMWebAPISession::thread_close, this ));
    return HVE_SCHEDULED;
}

void CVMWebAPISession::thread_close(){
    int ans = this->session->close();
    if (ans == 0) {
        this->fire_close();
    } else {
        this->fire_closeError(hypervisorErrorStr(ans), ans);
        this->fire_error(hypervisorErrorStr(ans), ans, "close");
    }
    
    /* The needs of having a daemon might have changed */
    CVMWebPtr p = this->getPlugin();
    if (p->hv != NULL) p->hv->checkDaemonNeed();
    
}

int CVMWebAPISession::resume(){

    /* Validate state */
    if (this->session->state != STATE_PAUSED) return HVE_INVALID_STATE;

    /* Resume probing timer */
    this->probeTimer->start();
    
    boost::thread t(boost::bind(&CVMWebAPISession::thread_resume, this ));
    return HVE_SCHEDULED;
}

void CVMWebAPISession::thread_resume(){
    int ans = this->session->resume();
    if (ans == 0) {
        this->fire_resume();
    } else {
        this->fire_resumeError(hypervisorErrorStr(ans), ans);
        this->fire_error(hypervisorErrorStr(ans), ans, "resume");
    }
}

int CVMWebAPISession::reset(){
    
    /* Validate state */
    if (this->session->state != STATE_STARTED) return HVE_INVALID_STATE;
    
    /* Make API unavailable but don't stop the timer */
    if (this->isAlive) {
        this->isAlive = false;
        this->fire_apiUnavailable();
    }
    
    // Start the reset thread
    boost::thread t(boost::bind(&CVMWebAPISession::thread_reset, this ));
    return HVE_SCHEDULED;
}

void CVMWebAPISession::thread_reset(){
    int ans = this->session->reset();
    if (ans == 0) {
        this->fire_reset();
    } else {
        this->fire_resetError(hypervisorErrorStr(ans), ans);
        this->fire_error(hypervisorErrorStr(ans), ans, "reset");
    }
}

int CVMWebAPISession::stop(){
    
    /* Validate state */
    if (this->session->state != STATE_STARTED) return HVE_INVALID_STATE;

    /* Make it unavailable */
    if (this->isAlive) {
        this->isAlive = false;
        this->fire_apiUnavailable();
    }

    boost::thread t(boost::bind(&CVMWebAPISession::thread_stop, this ));
    return HVE_SCHEDULED;
}

void CVMWebAPISession::thread_stop(){
    int ans = this->session->stop();
    if (ans == 0) {
        this->fire_stop();
    } else {
        this->fire_stopError(hypervisorErrorStr(ans), ans);
        this->fire_error(hypervisorErrorStr(ans), ans, "stop");
    }
}

int CVMWebAPISession::hibernate(){
    
    /* Validate state */
    if (this->session->state != STATE_STARTED) return HVE_INVALID_STATE;

    /* Make it unavailable */
    if (this->isAlive) {
        this->isAlive = false;
        this->fire_apiUnavailable();
    }

    boost::thread t(boost::bind(&CVMWebAPISession::thread_hibernate, this ));
    return HVE_SCHEDULED;
}

void CVMWebAPISession::thread_hibernate(){
    int ans = this->session->hibernate();
    if (ans == 0) {
        this->fire_hibernate();
    } else {
        this->fire_hibernateError(hypervisorErrorStr(ans), ans);
        this->fire_error(hypervisorErrorStr(ans), ans, "hibernate");
    }
}

int CVMWebAPISession::open( const FB::variant& o ){

    /* Validate state */
    if ((this->session->state != STATE_CLOSED) && 
        (this->session->state != STATE_ERROR)) return HVE_INVALID_STATE;

    /* Start probing timer */
    this->probeTimer->start();

    boost::thread t(boost::bind(&CVMWebAPISession::thread_open, this, o ));
    return HVE_SCHEDULED;
}

/**
 * Helper for thread_open, to check if the user was granted permissions
 * to override particular variables from javascript.
 */
bool __canOverride( const std::string& vname, HVSession * sess ) {
    return (std::find(sess->overridableVars.begin(), sess->overridableVars.end(), vname) != sess->overridableVars.end());
}

/**
 * Asynchronous function to open a new session
 */
void CVMWebAPISession::thread_open( const FB::variant& oConfigHash  ){
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
        this->fire_open();
        
    } else {
        
        // Close session in case of a problem
        this->session->close();
        
        // Then fire errors
        this->fire_openError(hypervisorErrorStr(ans), ans);
        this->fire_error(hypervisorErrorStr(ans), ans, "open");
    }
    
    // The requirements of having a daemon might have changed
    CVMWebPtr p = this->getPlugin();
    if (p->hv != NULL) p->hv->checkDaemonNeed();
    
}

int CVMWebAPISession::start( const FB::variant& cfg ) {
    
    /* Validate state */
    if (this->session->state != STATE_OPEN) return HVE_INVALID_STATE;
    
    /* Start probing timer */
    this->probeTimer->start();

    boost::thread t(boost::bind(&CVMWebAPISession::thread_start, this, cfg ));
    return HVE_SCHEDULED;
}

void CVMWebAPISession::thread_start( const FB::variant& cfg ) {
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
        this->fire_start();

    } else {
        this->fire_startError(hypervisorErrorStr(ans), ans);
        this->fire_error(hypervisorErrorStr(ans), ans, "start");
    }
}

int CVMWebAPISession::setExecutionCap(int cap) {
    return this->session->setExecutionCap( cap );
}

int CVMWebAPISession::setProperty( const std::string& name, const std::string& value ) {
    if (name.compare("/CVMWeb/secret") == 0) return HVE_NOT_ALLOWED;
    return this->session->setProperty( name, value );
}

std::string CVMWebAPISession::getProperty( const std::string& name ) {
    if (name.compare("/CVMWeb/secret") == 0) return "";
    return this->session->getProperty( name );
}

// Read-only parameters
int CVMWebAPISession::get_executionCap() {
    return this->session->executionCap;
}

int CVMWebAPISession::get_disk() {
    return this->session->disk;
}

int CVMWebAPISession::get_cpus() {
    return this->session->cpus;
}

int CVMWebAPISession::get_memory() {
    return this->session->memory;
}

int CVMWebAPISession::get_state() {
    return this->session->state;
}

int CVMWebAPISession::get_flags() {
    return this->session->flags;
}

std::string CVMWebAPISession::get_ip() {
    return this->session->getIP();
}

std::string CVMWebAPISession::get_rdp() {
    return this->session->getRDPHost();
}

std::string CVMWebAPISession::get_version() {
    return this->session->version;
}

std::string CVMWebAPISession::get_name() {
    return this->session->name;
}

std::string CVMWebAPISession::get_resolution() {

    // Get screen screen resolution as extra info
    return this->session->getExtraInfo(EXIF_VIDEO_MODE);

}

std::string CVMWebAPISession::get_apiEntryPoint() {
    std::string ip = this->session->getIP();
    if (ip.empty()) {
        return "";
    } else {
        char numstr[21]; // enough to hold all numbers up to 64-bits
        sprintf(numstr, "%d", (unsigned int) this->session->apiPort);
        return "http://" + ip + ":" + numstr;
    }
}

void CVMWebAPISession::onProgress(const size_t v, const size_t tot, const std::string& msg) {
    this->fire_progress(v,tot,msg);
}

void CVMWebAPISession::onOpen() {
    this->fire_open();
}

void CVMWebAPISession::onError( const std::string& msg, const int code, const std::string& category) {
    this->fire_error(msg, code, category);
    if (category.compare("open") == 0) this->fire_openError(msg, code);
    if (category.compare("start") == 0) this->fire_startError(msg, code);
    if (category.compare("stop") == 0) this->fire_stopError(msg, code);
    if (category.compare("pause") == 0) this->fire_pauseError(msg, code);
    if (category.compare("resume") == 0) this->fire_resumeError(msg, code);
    if (category.compare("reset") == 0) this->fire_resetError(msg, code);
}

void CVMWebAPISession::onDebug( const std::string& line ) {
    this->fire_debug( line );
}

void CVMWebAPISession::onStart() {
    this->fire_start();
}

void CVMWebAPISession::onClose() {
    this->fire_close();
}

void CVMWebAPISession::onStop() {
    this->fire_stop();
}

std::string CVMWebAPISession::toString() {
    return "[CVMWebAPISession]";
}

void CVMWebAPISession::cb_timer() {
    if (!isAlive) {
        if (this->session->state == STATE_STARTED) {
            if (this->session->isAPIAlive()) {
                isAlive = true;
                fire_apiAvailable( this->get_ip(), this->get_apiEntryPoint() );
            }
        }
    } else {
        if ((this->session->state != STATE_STARTED) || (!this->session->isAPIAlive())) {
            isAlive = false;
            fire_apiUnavailable();
        }
    }
}

bool CVMWebAPISession::get_live() {
    return this->isAlive;
}

int CVMWebAPISession::get_daemonMinCap() {
    return this->session->daemonMinCap;
}

int CVMWebAPISession::get_daemonMaxCap() {
    return this->session->daemonMaxCap;
}

bool CVMWebAPISession::get_daemonControlled() {
    return this->session->daemonControlled;
}

int CVMWebAPISession::get_daemonFlags() {
    return this->session->daemonFlags;
}

void CVMWebAPISession::set_daemonMinCap( int cap ) {
    this->session->daemonMinCap = cap;
    this->setProperty("/CVMWeb/daemon/cap/min", ntos<int>(cap));
}

void CVMWebAPISession::set_daemonMaxCap( int cap ) {
    this->session->daemonMaxCap = cap;
    this->setProperty("/CVMWeb/daemon/cap/max", ntos<int>(cap));
}

void CVMWebAPISession::set_daemonControlled( bool controled ) {
    this->session->daemonControlled = controled;
    this->setProperty("/CVMWeb/daemon/controlled", (controled ? "1" : "0"));

    /* The needs of having a daemon might have changed */
    CVMWebPtr p = this->getPlugin();
    if (p->hv != NULL) p->hv->checkDaemonNeed();
}

void CVMWebAPISession::set_daemonFlags( int flags ) {
    this->session->daemonFlags = flags;
    this->setProperty("/CVMWeb/daemon/flags", ntos<int>(flags));
}

/**
 * Update session information from hypervisor session
 */
int CVMWebAPISession::update() {
    if (this->updating) return HVE_STILL_WORKING;
    this->updating = true;
    boost::thread t(boost::bind(&CVMWebAPISession::thread_update, this ));
    return HVE_SCHEDULED;
}
void CVMWebAPISession::thread_update() {

    // Don't do anything if we are in the middle of something
    if ((this->session->state == STATE_OPPENING) || (this->session->state == STATE_STARTING)) {
        this->updating = false;
        return;
    }
    
    // Keep old state in order to detect state changes
    int oldState = this->session->state;
    
    // Update session info from driver
    int ans = this->session->update();
    if (ans < 0) {
        
        // Check if the session went away
        if (ans == HVE_NOT_FOUND) {
            
            /* Stop probing timer */
            this->probeTimer->stop();

            /* Make it unavailable */
            if (this->isAlive) {
                this->isAlive = false;
                this->fire_apiUnavailable();
            }
            
            /* Session closed */
            this->fire_close();
            
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
    if (oldState != this->session->state) {
        switch (this->session->state) {
            
            case STATE_OPEN:
            
                /* Start probing timer */
                this->probeTimer->start();
                
                /* Session open */
                //this->fire_open();
                break;
            
            case STATE_STARTED:
            
                /* Start probing timer */
                this->probeTimer->start();
                
                /* Session started */
                this->fire_start();
                break;
            
            case STATE_PAUSED:

                /* Stop probing timer */
                this->probeTimer->stop();

                /* Make it unavailable */
                if (this->isAlive) {
                    this->isAlive = false;
                    this->fire_apiUnavailable();
                }
    
                /* Session paused */
                this->fire_pause();
                break;
            
            case STATE_ERROR:
                
                // TODO: Am I really using this somewhere? :P :P
                break;
            
        }
    }
    
    /* Everything was OK */
    this->updating = false;
    
}