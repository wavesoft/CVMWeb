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
#ifndef CALLBACKS_PROGRESS_H
#define CALLBACKS_PROGRESS_H

#include "Callbacks.h"

class CallbacksProgress: public Callbacks {
public:

    /**
     * Initialize parent
     */
    CallbacksProgress(): Callbacks() { };

	/**
	 * Fire 'started' event
	 */
	void fireStarted( const std::string & msg )
		{ fire("started", ArgumentList(msg) ); };

	/**
	 * Fire 'completed' event
	 */
	void fireCompleted( const std::string & msg )
		{ fire("completed", ArgumentList(msg) ); };

	/**
	 * Fire 'failed' event
	 */
	void fireFailed( const std::string & msg, const int errorCode )
		{ fire("failed", ArgumentList(msg)(errorCode) ); };

	/**
	 * Fire 'progress' event
	 */
	void fireProgress( const std::string& msg, const double progress )
		{ fire("progress", ArgumentList(msg)(progress) ); };


};

#endif /* end of include guard: CALLBACKS_PROGRESS_H */