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
class BaseProgressTask;
class ProgressTask;
class SlottedTask;
class UnknownTask;
typedef boost::shared_ptr< BaseProgressTask >       BaseProgressTaskPtr;
typedef boost::shared_ptr< ProgressTask >       	ProgressTaskPtr;
typedef boost::shared_ptr< SlottedTask >       		SlottedTaskPtr;
typedef boost::shared_ptr< UnknownTask >       		UnknownTaskPtr;


/* Callback functions reference */
typedef boost::function<void ( const std::string& message )>    										cbStarted;
typedef boost::function<void ( const std::string& message )>    										cbCompleted;
typedef boost::function<void ( const std::string& message, const int errorCode )> 						cbError;
typedef boost::function<void ( const double progress, const std::string& message )>    cbProgress;

/**
 * Base class to monitor progress events
 */
class BaseProgressTask: public boost::enable_shared_from_this<BaseProgressTask> {
public:

	/**
	 * Progress feedback constructor
	 */
	BaseProgressTask 	( const std::string& taskName = "", const size_t maxTasks = 0 
		) : startedCallbacks(), completedCallbacks(), errorCallbacks(), progressCallbacks(), subtasks(), max(maxTasks), current(0), name(taskName), completed(false) { };

	/**
	 * Return the completeness of this task
	 */
	virtual double		getProgress		() = 0;

	/**
	 * Return a slotted task instance
	 */
	 SlottedTaskPtr		beginTasks		( const std::string& message, const size_t tasks = 0 );
	 ProgressTaskPtr	beginProgress	( const std::string& message, const size_t tasks = 0 );
	 UnknownTaskPtr		beginUnknown	( const std::string& message );

	/**
	 * Register callbacks on progress events
	 */
	void 				onStarted		( cbStarted & cb );
	void 				onCompleted		( cbCompleted & cb );
	void 				onError			( cbError & cb );
	void 				onProgress		( cbProgress & cb );

protected:

	// Forward events
	void 								_forwardProgress( const std::string& message );
	void 								_forwardError( const std::string& message, const int errorCode );

	// Metrics
	std::string 						name;
	BaseProgressTaskPtr					parent;

	// Callback list
	std::vector< cbStarted >			startedCallbacks;
	std::vector< cbCompleted >			completedCallbacks;
	std::vector< cbError >				errorCallbacks;
	std::vector< cbProgress >			progressCallbacks;

	// List of sub-tasks
	std::list< BaseProgressTaskPtr > 	subtasks;

};

/**
 * Slotted tasks class
 */
class SlottedTask : public BaseProgressTask {
public:

	/**
	 * Update the maximum number of steps in this task 
	 */
	void 				setMax 		( const size_t max = 0);

	/**
	 * Increase the step counter by one and display the specified message 
	 */
	void 				step 		( const std::string& message );

	/**
	 * Override the default progress function
	 */
	virtual double		getProgress		();

private:

	size_t 								max;
	size_t 								current;

};

/**
 * Progress Tasks
 */
class ProgressTask : public BaseProgressTask {
public:

	/**
	 * Update the maximum number of steps in this task 
	 */
	void 				setMax 		( const size_t max = 0);

	/**
	 * Update to a variable progress 
	 */
	void 				update 		( const size_t value );

	/**
	 * Override the default progress function
	 */
	virtual double		getProgress		();

private:

	size_t 								max;
	size_t 								current;

};

/**
 * Unknw
 */
class UnknownTask : public BaseProgressTask {
public:

	/**
	 * Mark the task as completed
	 */
	void 				complete 		( const std::string& message );

	/**
	 * Override the default progress function
	 */
	virtual double		getProgress		();

private:

	bool 								completed;

};











/**
 * General purpose function to report progress events to the user.
 */
class ProgressFeedback {
public:



	/**
	 * Start a new sub-task on this progress  
	 */
	ProgressFeedbackPtr	newTask 	( const std::string& taskName, const size_t max = 0 );

	/**
	 * Enter a state where we can't report anything about the progress.
	 * Just wait for the completed() function to be called
	 */
	void 				unknown 	( const std::string& message );


	/**
	 * Start the task and optionally change the maximum number of tasks in advance
	 */
	void 				start 		( const std::string& name, const size_t max = 0 );

	/**
	 * Increase the step counter by one and display the specified message 
	 */
	void 				step 		( const std::string& message );

	/**
	 * Update to a variable progress 
	 */
	void 				set 		( const size_t value );

	/**
	 * Complete the current task 
	 */
	void 				done 		( );

	/**
	 * Fail the current task 
	 */
	void 				failed 		( const std::string& message, const int errorCode = 0 );

private:



	// Internal callbacks

};

#endif /* end of include guard: PROGRESSFEEDBACK_H */
