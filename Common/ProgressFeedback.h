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

#ifndef PROGRESSFEEDBACK_H
#define PROGRESSFEEDBACK_H

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
typedef boost::shared_ptr< BaseProgressTask >       ProgressTaskPtr;
typedef boost::shared_ptr< FiniteTask >       		FiniteTaskPtr;
typedef boost::shared_ptr< VariableTask >       	VariableTaskPtr;
typedef boost::shared_ptr< BooleanTask >       		BooleanTaskPtr;


/* Callback functions reference */
typedef boost::function<void ( const std::string& message )>    						cbStarted;
typedef boost::function<void ( const std::string& message )>    						cbCompleted;
typedef boost::function<void ( const std::string& message, const int errorCode )> 		cbError;
typedef boost::function<void ( const double progress, const std::string& message )> 	cbProgress;

/**
 * Base class to monitor progress events
 */
class ProgressTask: public boost::enable_shared_from_this<BaseProgressTask> {
public:

	//////////////////////
	// Constructor
	//////////////////////
	ProgressTask() 
		: started(false), completed(false), startedCallbacks(), parent(), lastMessage(""),
		  completedCallbacks(), errorCallbacks(), progressCallbacks() { };


	//////////////////////
	// Event handlers
	//////////////////////

	/**
	 * Register a callback that will be fired when the very first task has
	 * started progressing.
	 */
	void 				onStarted		( cbStarted & cb );

	/**
	 * Register a callback that will be fired when the last task has completed
	 * progress.
	 */
	void 				onCompleted		( cbCompleted & cb );

	/**
	 * Register a callback that will be fired when an error has occured.
	 */
	void 				onError			( cbError & cb );

	/**
	 * Register a callback event that will be fired when a progress
	 * event is updated.
	 */
	void 				onProgress		( cbProgress & cb );

protected:

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

private:

	//////////////////////
	// State variables
	//////////////////////
	bool				 				started;
	bool 								completed;
	ProgressTaskPtr 					parent;
	std::message 						lastMessage;

	//////////////////////
	// Event registry
	//////////////////////

	std::vector< cbStarted >			startedCallbacks;
	std::vector< cbCompleted >			completedCallbacks;
	std::vector< cbError >				errorCallbacks;
	std::vector< cbProgress >			progressCallbacks;

	//////////////////////
	// Internal callbacks
	//////////////////////

	/**
	 * [Internal] Callback to let listeners know that we started progress
	 */
	void				_notifyStarted();

	/**
	 * [Internal] Callback to let listeners know that a progress change has occured.
	 */
	void 				_notifyProgressUpdate();

	/**
	 * [Internal] Callback to let listeners know that we are done
	 */
	void 				_notifyCompleted();


};


/**
 * Base class to monitor progress events
 */
class FiniteTask: public ProgressTask {
public:

	//////////////////////
	// Constructor
	//////////////////////
	ProgressTask() : ProgressTask(), tasks(), taskObjects(), taskIndex(0) { };

	/**
	 * Define the maximum number of tasks
	 */
	void 						setMax ( size_t maxTasks );

	/**
	 * Mark one of the sub-tasks as completed
	 */
	void 						done( const std::message& message );

	/**
	 * Create a sub-task
	 */
	template <typename T>
	 	boost::shared_ptr<T> 	begin( const std::message& message );

	/**
	 * Mark the entire stack of objects as completed
	 */
	void 						complete( const std::message& message );


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
	std::vector< ProgressTask >		taskObjects;

	/**
	 * Position index
	 */
	size_t 							taskIndex;

};



#endif /* end of include guard: PROGRESSFEEDBACK_H */
