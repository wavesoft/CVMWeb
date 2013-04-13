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
#include "BrowserHost.h"
#include "CVMWeb.h"
#include "Hypervisor.h"

#ifndef H_CVMWebAPISession
#define H_CVMWebAPISession

class CVMWebAPISession : public FB::JSAPIAuto
{
public:

    CVMWebAPISession(const CVMWebPtr& plugin, const FB::BrowserHostPtr& host, HVSession * session) :
        m_plugin(plugin), m_host(host)
    {

        registerMethod("start",             make_method(this, &CVMWebAPISession::start));
        registerMethod("open",              make_method(this, &CVMWebAPISession::open));
        registerMethod("pause",             make_method(this, &CVMWebAPISession::pause));
        registerMethod("resume",            make_method(this, &CVMWebAPISession::resume));
        registerMethod("reset",             make_method(this, &CVMWebAPISession::reset));
        registerMethod("close",             make_method(this, &CVMWebAPISession::close));
        registerMethod("stop",              make_method(this, &CVMWebAPISession::stop));
        registerMethod("setProperty",       make_method(this, &CVMWebAPISession::setProperty));
        registerMethod("getProperty",       make_method(this, &CVMWebAPISession::getProperty));
        registerMethod("setExecutionCap",   make_method(this, &CVMWebAPISession::setExecutionCap));
        
        registerProperty("ip",              make_property(this, &CVMWebAPISession::get_ip));
        registerProperty("cpus",            make_property(this, &CVMWebAPISession::get_cpus));
        registerProperty("state",           make_property(this, &CVMWebAPISession::get_state));
        registerProperty("ram",             make_property(this, &CVMWebAPISession::get_memory));
        registerProperty("disk",            make_property(this, &CVMWebAPISession::get_disk));
        registerProperty("version",         make_property(this, &CVMWebAPISession::get_version));
        registerProperty("apiURL",          make_property(this, &CVMWebAPISession::get_apiEntryPoint));
        registerProperty("executionCap",    make_property(this, &CVMWebAPISession::get_executionCap));
        
        /* Import session */
        this->session = session;
        this->sessionID = session->internalID;
        
        /* Callback bindings */
        if (this->session != NULL) {
            this->session->cbObject = (void *) this;
            this->session->onProgress = &CVMWebAPISession::onProgress;
            this->session->onOpen = &CVMWebAPISession::onOpen;
            this->session->onOpenError = &CVMWebAPISession::onOpenError;
            this->session->onStart = &CVMWebAPISession::onStart;
            this->session->onStartError = &CVMWebAPISession::onStartError;
            this->session->onClose = &CVMWebAPISession::onClose;
            this->session->onLive = &CVMWebAPISession::onLive;
            this->session->onDead = &CVMWebAPISession::onDead;
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
    FB_JSAPI_EVENT(error,       3, ( const std::string&, int, const std::string& ));
    FB_JSAPI_EVENT(live,        0, ());
    FB_JSAPI_EVENT(dead,        0, ());
    
    // Threads
    void thread_close( );
    void thread_pause( );
    void thread_resume( );
    void thread_stop( );
    void thread_reset( );
    void thread_open( const FB::JSObjectPtr &o );
    void thread_start( const FB::variant& cfg );
    
    // Functions
    int pause();
    int close();
    int resume();
    int reset();
    int stop();
    int open( const FB::JSObjectPtr &o );
    int start( const FB::variant& cfg );
    void setExecutionCap(int cap);
    void setProperty( const std::string& name, const std::string& value );
    std::string getProperty( const std::string& name );
    
    // Read-only parameters
    int get_executionCap();
    int get_cpus();
    int get_memory();
    int get_disk();
    int get_state();
    std::string get_ip();
    std::string get_apiEntryPoint();
    std::string get_version();
    
    // Callback forwards
    static void onProgress(int, int, std::string, void *);
    static void onOpen( void * );
    static void onOpenError(std::string, int, void *);
    static void onStart( void * );
    static void onStartError(std::string, int, void *);
    static void onClose( void * );
    static void onLive( void * );
    static void onDead( void * );
    static void onStop( void * );
    
    
private:
    CVMWebWeakPtr           m_plugin;
    FB::BrowserHostPtr      m_host;
    int                     sessionID;
    HVSession *             session;
};

#endif // H_CVMWebAPISession

