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
#ifndef DAEMON_COMPONENT_WEBAPISESSION_H
#define DAEMON_COMPONENT_WEBAPISESSION_H

#include <Common/Hypervisor.h>

class DaemonCore;
class DaemonSession;

#include "../daemon_core.h"
#include "../daemon_session.h"

class CVMWebAPISession {
public:

	/**
	 * Constructor for the CernVM WebAPI Session 
	 */
	CVMWebAPISession( DaemonCore& core, DaemonSession& session, HVSessionPtr hvSession  )
		: core(core), session(session), hvSession(hvSession)
	{ 

		// Handle state changes
        hvSession->on( "stateChanged", boost::bind( &CVMWebAPISession::__cbStateChanged, this, _1 ) );

	};

	/**
	 * Handling of commands directed for this session
	 */
	void handleAction( const std::string& id, const std::string& action, ParameterMapPtr parameters );

private:

	/**
	 * Event callbacks from the hypervisor session
	 */
	void __cbStateChanged( VariantArgList& args );

	/**
	 * The daemon's core state manager
	 */
	DaemonCore&			core;

	/**
	 * The websocket session used for I/O
	 */
	DaemonSession& 		session;

	/**
	 * The encapsulated hypervisor session pointer
	 */
	HVSessionPtr		hvSession;

};

#endif /* end of include guard: DAEMON_COMPONENT_WEBAPISESSION_H */
