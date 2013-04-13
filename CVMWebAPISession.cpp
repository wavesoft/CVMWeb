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
#include "variant_list.h"
#include "DOM/Document.h"
#include "global/config.h"

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
    int ans = this->session->pause();
    if (ans == 0) this->fire_pause();
    return ans;
}

int CVMWebAPISession::close(){
    boost::thread t(boost::bind(&CVMWebAPISession::thread_close, this ));
    return HVE_SHEDULED;
}

void CVMWebAPISession::thread_close(){
    int ans = this->session->close();
    if (ans == 0) {
        this->fire_close();
    } else {
        this->fire_closeError(hypervisorErrorStr(ans), ans);
    }
}

int CVMWebAPISession::resume(){
    int ans = this->session->resume();
    if (ans == 0) this->fire_resume();
    return ans;
}

int CVMWebAPISession::reset(){
    int ans = this->session->reset();
    if (ans == 0) this->fire_reset();
    return ans;
}

int CVMWebAPISession::stop(){
    int ans = this->session->stop();
    if (ans == 0) this->fire_stop();
    return ans;
}

int CVMWebAPISession::open( const FB::JSObjectPtr& o ){
    boost::thread t(boost::bind(&CVMWebAPISession::thread_open, this, o ));
    return HVE_SHEDULED;
}

void CVMWebAPISession::thread_open( const FB::JSObjectPtr& o ){
    int cpus = 1;
    int ram = 256;
    int disk = 1024;
    int ans = 0;
    std::string ver = DEFAULT_CERNVM_VERSION;
    if (o->HasProperty("cpus")) cpus = o->GetProperty("cpus").convert_cast<int>();
    if (o->HasProperty("ram")) ram = o->GetProperty("ram").convert_cast<int>();
    if (o->HasProperty("disk")) disk = o->GetProperty("disk").convert_cast<int>();
    if (o->HasProperty("version")) ver = o->GetProperty("version").cast<std::string>();
    ans = this->session->open( cpus, ram, disk, ver);
    if (ans == 0) {
        this->fire_open();
    } else {
        this->fire_openError(hypervisorErrorStr(ans), ans);
    }
}

int CVMWebAPISession::start( const FB::variant& cfg ) {
    boost::thread t(boost::bind(&CVMWebAPISession::thread_start, this, cfg ));
    return HVE_SHEDULED;
}

void CVMWebAPISession::thread_start( const FB::variant& cfg ) {
    int ans = 0;
    std::string vmUserData = cfg.cast<std::string>();
    ans = this->session->start(vmUserData);
    if (ans == 0) {
        this->fire_start();
    } else {
        this->fire_startError(hypervisorErrorStr(ans), ans);
    }
}

void CVMWebAPISession::setExecutionCap(int cap) {
    this->session->setExecutionCap( cap );
}

void CVMWebAPISession::setProperty( const std::string& name, const std::string& value ) {
    if (name.compare("web-secret") == 0) return;
    this->session->setProperty( name, value );
}

std::string CVMWebAPISession::getProperty( const std::string& name ) {
    if (name.compare("web-secret") == 0) return "";
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

std::string CVMWebAPISession::get_ip() {
    return this->session->ip;
}

std::string CVMWebAPISession::get_version() {
    return this->session->version;
}

std::string CVMWebAPISession::get_apiEntryPoint() {
    return "http://" + this->session->ip + ":8080/api";
}

void CVMWebAPISession::onProgress(int v, int tot, std::string msg, void * p) {
    CVMWebAPISession * self = (CVMWebAPISession*)p;
    self->fire_progress(v,tot,msg);
}

void CVMWebAPISession::onOpen( void * p) {
    CVMWebAPISession * self = (CVMWebAPISession*)p;
    self->fire_open();
}

void CVMWebAPISession::onOpenError(std::string msg, int code, void * p) {
    CVMWebAPISession * self = (CVMWebAPISession*)p;
    self->fire_openError(msg, code);
}

void CVMWebAPISession::onStart( void * p ) {
    CVMWebAPISession * self = (CVMWebAPISession*)p;
    self->fire_start();
}

void CVMWebAPISession::onStartError(std::string msg, int code, void * p) {
    CVMWebAPISession * self = (CVMWebAPISession*)p;
    self->fire_startError(msg, code);
}

void CVMWebAPISession::onClose( void * p ) {
    CVMWebAPISession * self = (CVMWebAPISession*)p;
    self->fire_close();
}

void CVMWebAPISession::onLive( void * p ) {
    CVMWebAPISession * self = (CVMWebAPISession*)p;
    self->fire_live();
}

void CVMWebAPISession::onDead( void * p ) {
    CVMWebAPISession * self = (CVMWebAPISession*)p;
    self->fire_dead();
}

void CVMWebAPISession::onStop( void * p ) {
    CVMWebAPISession * self = (CVMWebAPISession*)p;
    self->fire_stop();
}