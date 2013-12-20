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
#ifndef PROGRESSFEEDBACK_H
#define PROGRESSFEEDBACK_H

#include "CallbacksProgress.h"

#include <vector>
#include <list>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>

/* Forward-declaration of ProgressFeedback class */
class ProgressTask;
class FiniteTask;
class VariableTask;
class BooleanTask;
typedef boost::shared_ptr< ProgressTask >           ProgressTaskPtr;
typedef boost::shared_ptr< FiniteTask >       		FiniteTaskPtr;
typedef boost::shared_ptr< VariableTask >       	VariableTaskPtr;
typedef boost::shared_ptr< BooleanTask >       		BooleanTaskPtr;

/**
 * Base class to monitor progress events
 */
class ProgressTask: public boost::enable_shared_from_this<ProgressTask>, public CallbacksProgress {
public:

	//////////////////////
	// Constructor
	//////////////////////
	ProgressTask() 
		: CallbacksProgress(), started(false), completed(false), parent(), lastMessage(""), __lastEventTime(0) { };


	//////////////////////
	// Event handlers
	//////////////////////

	/**
	 * Mark the task as completed, overriding any active progress
	 */
	void 				complete 		( const std::string& message = "" );

	/**
	 * Mark the task as failed, overriding any active progress
	 */
	void 				fail 			( const std::string& message = "", const int errorCode = -1 );

	/**
	 * Fire a progress update without any change, just change message
	 */
	void				doing			( const std::string& message );

    /**
     * Used for throttling
     */
    unsigned long                       __lastEventTime;

	//////////////////////////
	// Overridable callbacks
	//////////////////////////
    
	/**
	 * Get the percentage of the completed tasks
	 */
	virtual double 		getProgress		( ) = 0;

	/**
	 * Return a flag that denotes if the task is completed
	 */
	virtual bool 		isCompleted		( ) = 0;

    /**
     * Reset state and restart
     */
    virtual void        restart         ( const std::string& message, bool triggerUpdate = true ) = 0;


    //////////////////////
	// State variables
	//////////////////////

    /**
     * The parent object
     */
    ProgressTaskPtr 					parent;

    /**
     * The default progress message to use when the user has not
     * specified a string in the progress update functions.
     */
    std::string 						lastMessage;

protected:

	//////////////////////
	// State variables
	//////////////////////
	bool				 				started;
	bool 								completed;

	//////////////////////
	// Internal callbacks
	//////////////////////

	/**
	 * [Internal] Callback to let listeners know that we started progress
	 */
	void				_notifyStarted( const std::string& message );

	/**
	 * [Internal] Callback to let listeners know that a progress change has occured.
	 */
	void 				_notifyUpdate( const std::string& message );

	/**
	 * [Internal] Callback to let listeners know that we are done
	 */
	void 				_notifyCompleted( const std::string& message );

	/**
	 * [Internal] Callback to let listeners know that we have failed
	 */
	void 				_notifyFailed( const std::string& message, const int errorCode );

    /**
     * [Internal] Propagate a progress event to the callbacks and parent
     */
    void                _forwardProgress( const std::string& message );

};


/**
 * Base class to monitor progress events
 */
class FiniteTask: public ProgressTask {
public:

	//////////////////////
	// Constructor
	//////////////////////

	FiniteTask() : ProgressTask(), tasks(), taskObjects(), taskIndex(0) { };

	/**
	 * Define the maximum number of tasks
	 */
	void 						setMax ( size_t maxTasks, bool triggerUpdate = true );

	/**
	 * Mark one of the sub-tasks as completed
	 */
	void 						done( const std::string& message );

    /**
     * Reset state and restart
     */
    virtual void                restart( const std::string& message, bool triggerUpdate = true );

	/**
	 * Rollback an action that was previsusly 'done'
	 */
	void 						rollback( const std::string& reason );

	/**
	 * Create a sub-task
	 */
	template <typename T>
	 	boost::shared_ptr<T> 	begin( const std::string& message );


protected:

	//////////////////////
	// Implementation
	//////////////////////

	/**
	 * Check if the tasks are completed
	 */
	virtual bool 		isCompleted		( );


	/**
	 * Return the progress value of this task
	 */
	virtual double 		getProgress		( );


private:

	//////////////////////
	// State variables
	//////////////////////

	/**
	 * The status of the tasks that were flagged as done
	 */
	std::vector< unsigned char > 	tasks;

	/**
	 * The sub-tasks that can 
	 */
	std::vector< ProgressTaskPtr >  taskObjects;

	/**
	 * Position index
	 */
	size_t 							taskIndex;

};


/**
 * A variable task is like the download progress. It has multiple steps
 */
class VariableTask: public ProgressTask {
public:

	VariableTask() : ProgressTask(), max(0), current(0) { };

	/**
	 * Update the default message for all the tasks 
	 */
	void 				setMessage 		( const std::string& message );

	/**
	 * Set maximum value
	 */
	void 				setMax			( size_t maxValue, bool triggerUpdate = false );

	/**
	 * Update value
	 */
	void 				update 			( size_t value );

    /**
     * Reset state and restart
     */
    virtual void        restart         ( const std::string& message, bool triggerUpdate = true );

protected:

	//////////////////////
	// Implementation
	//////////////////////

	/**
	 * Check if the tasks are completed
	 */
	virtual bool 		isCompleted		( );


	/**
	 * Return the progress value of this task
	 */
	virtual double 		getProgress		( );


private:

	// Variable size
	size_t 							max;
	size_t							current;


};


/**
 * Base class to monitor progress events
 */
class BooleanTask: public ProgressTask {
public:

	BooleanTask() : ProgressTask() { };

    /**
     * Reset state and restart
     */
    virtual void        restart         ( const std::string& message, bool triggerUpdate = true );

protected:

	//////////////////////
	// Implementation
	//////////////////////

	/**
	 * Check if the tasks are completed
	 */
	virtual bool 		isCompleted		( );


	/**
	 * Return the progress value of this task
	 */
	virtual double 		getProgress		( );

};


#endif /* end of include guard: PROGRESSFEEDBACK_H */
