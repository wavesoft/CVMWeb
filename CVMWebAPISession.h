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

/**
 * Javascript JSAPI adapter to the HVSession object
 */
 
#include <string>
#include <sstream>
#include <boost/weak_ptr.hpp>
#include "JSAPIAuto.h"
#include "Timer.h"
#include "BrowserHost.h"
#include "CVMWeb.h"
#include "Hypervisor.h"

#ifndef H_CVMWebAPISession
#define H_CVMWebAPISession

#define TIMER_PROBE_INTERVAL    5000

class CVMWebAPISession : public FB::JSAPIAuto
{
public:

    CVMWebAPISession(const CVMWebPtr& plugin, const FB::BrowserHostPtr& host, HVSession * session) :
        m_plugin(plugin), m_host(host)
    {

        registerMethod("start",                 make_method(this, &CVMWebAPISession::start));
        registerMethod("open",                  make_method(this, &CVMWebAPISession::open));
        registerMethod("pause",                 make_method(this, &CVMWebAPISession::pause));
        registerMethod("resume",                make_method(this, &CVMWebAPISession::resume));
        registerMethod("reset",                 make_method(this, &CVMWebAPISession::reset));
        registerMethod("close",                 make_method(this, &CVMWebAPISession::close));
        registerMethod("stop",                  make_method(this, &CVMWebAPISession::stop));
        registerMethod("hibernate",             make_method(this, &CVMWebAPISession::hibernate));
        registerMethod("setProperty",           make_method(this, &CVMWebAPISession::setProperty));
        registerMethod("getProperty",           make_method(this, &CVMWebAPISession::getProperty));
        registerMethod("setExecutionCap",       make_method(this, &CVMWebAPISession::setExecutionCap));
        
        registerProperty("ip",                  make_property(this, &CVMWebAPISession::get_ip));
        registerProperty("cpus",                make_property(this, &CVMWebAPISession::get_cpus));
        registerProperty("state",               make_property(this, &CVMWebAPISession::get_state));
        registerProperty("ram",                 make_property(this, &CVMWebAPISession::get_memory));
        registerProperty("disk",                make_property(this, &CVMWebAPISession::get_disk));
        registerProperty("version",             make_property(this, &CVMWebAPISession::get_version));
        registerProperty("live",                make_property(this, &CVMWebAPISession::get_live));
        registerProperty("executionCap",        make_property(this, &CVMWebAPISession::get_executionCap));
        registerProperty("apiURL",              make_property(this, &CVMWebAPISession::get_apiEntryPoint));
        registerProperty("rdpURL",              make_property(this, &CVMWebAPISession::get_rdp));

        registerProperty("daemonControlled",    make_property(this, &CVMWebAPISession::get_daemonControlled,
                                                                    &CVMWebAPISession::set_daemonControlled));
        registerProperty("daemonMinCap",        make_property(this, &CVMWebAPISession::get_daemonMinCap,
                                                                    &CVMWebAPISession::set_daemonMinCap));
        registerProperty("daemonMaxCap",        make_property(this, &CVMWebAPISession::get_daemonMaxCap,
                                                                    &CVMWebAPISession::set_daemonMaxCap));
        registerProperty("daemonFlags",         make_property(this, &CVMWebAPISession::get_daemonFlags,
                                                                    &CVMWebAPISession::set_daemonFlags));

        // Beautification
        registerMethod("toString",              make_method(this, &CVMWebAPISession::toString));
        
        /* Import session */
        this->session = session;
        this->sessionID = session->internalID;

        /* Setup timer */
        this->probeTimer = FB::Timer::getTimer( TIMER_PROBE_INTERVAL, true, boost::bind(&CVMWebAPISession::cb_timer, this ) );
        this->probeTimer->start();
        
        /* Setup session connections */
        if (this->session != NULL) {

            /* Callback bindings */
            this->session->cbObject = (void *) this;
            this->session->onProgress = &CVMWebAPISession::onProgress;
            this->session->onDebug = &CVMWebAPISession::onDebug;
            this->session->onError = &CVMWebAPISession::onError;
            this->session->onOpen = &CVMWebAPISession::onOpen;
            this->session->onStart = &CVMWebAPISession::onStart;
            this->session->onClose = &CVMWebAPISession::onClose;
            
        }
        
    }

    virtual ~CVMWebAPISession() {};
    CVMWebPtr getPlugin();
    
    // Events
    FB_JSAPI_EVENT(open,        0, ());
    FB_JSAPI_EVENT(openError,   2, ( const std::string&, int ));
    FB_JSAPI_EVENT(start,       0, ());
    FB_JSAPI_EVENT(startError,  2, ( const std::string&, int ));
    FB_JSAPI_EVENT(close,       0, ());
    FB_JSAPI_EVENT(closeError,  2, ( const std::string&, int ));
    FB_JSAPI_EVENT(progress,    3, ( int, int, const std::string& ));
    FB_JSAPI_EVENT(pause,       0, ());
    FB_JSAPI_EVENT(pauseError,  2, ( const std::string&, int ));
    FB_JSAPI_EVENT(resume,      0, ());
    FB_JSAPI_EVENT(resumeError, 2, ( const std::string&, int ));
    FB_JSAPI_EVENT(reset,       0, ());
    FB_JSAPI_EVENT(resetError,  2, ( const std::string&, int ));
    FB_JSAPI_EVENT(stop,        0, ());
    FB_JSAPI_EVENT(stopError,   2, ( const std::string&, int ));
    FB_JSAPI_EVENT(hibernate,   0, ());
    FB_JSAPI_EVENT(hibernateError,2, ( const std::string&, int ));
    FB_JSAPI_EVENT(error,       3, ( const std::string&, int, const std::string& ));
    FB_JSAPI_EVENT(apiAvailable,2, ( const std::string&, const std::string&));
    FB_JSAPI_EVENT(apiUnavailable,0, ());
    FB_JSAPI_EVENT(debug,       1, ( const std::string& ));
    
    // Threads
    void thread_close( );
    void thread_pause( );
    void thread_resume( );
    void thread_stop( );
    void thread_reset( );
    void thread_hibernate( );
    void thread_open( const FB::JSObjectPtr &o );
    void thread_start( const FB::variant& cfg );
    
    // Functions
    int pause();
    int close();
    int resume();
    int reset();
    int stop();
    int hibernate();
    int open( const FB::JSObjectPtr &o );
    int start( const FB::variant& cfg );
    int setExecutionCap(int cap);
    int setProperty( const std::string& name, const std::string& value );
    std::string getProperty( const std::string& name );
    
    // Property getters
    int get_executionCap();
    int get_cpus();
    int get_memory();
    int get_disk();
    int get_state();
    int get_daemonMinCap();
    int get_daemonMaxCap();
    int get_daemonFlags();
    bool get_daemonControlled();
    bool get_live();
    std::string get_ip();
    std::string get_rdp();
    std::string get_apiEntryPoint();
    std::string get_version();
    std::string toString();
    
    // Property setters
    void set_daemonMinCap( int Cap );
    void set_daemonMaxCap( int Cap );
    void set_daemonFlags( int Cap );
    void set_daemonControlled( bool controled );
    
    // Callback forwards
    static void onProgress(int, int, std::string, void *);
    static void onError(std::string, int, std::string, void *);
    static void onDebug(std::string, void * );
    static void onOpen( void * );
    static void onStart( void * );
    static void onClose( void * );
    static void onStop( void * );
    
    // Timer implementation
    FB::TimerPtr    probeTimer;
    void            cb_timer();

    // Local logic
    bool            isAlive;
    
private:
    CVMWebWeakPtr           m_plugin;
    FB::BrowserHostPtr      m_host;
    int                     sessionID;
    HVSession *             session;
};

#endif // H_CVMWebAPISession

