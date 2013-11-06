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

/* Forward-declaration of ProgressFeedback class */
class ProgressFeedback;
typedef boost::shared_ptr< ProgressFeedback >       ProgressFeedbackPtr;

/* Callback functions reference */
typedef boost::function<void ( const std::string& message )>    										cbStarted;
typedef boost::function<void ( const std::string& message )>    										cbCompleted;
typedef boost::function<void ( const std::string& message, const int errorCode )> 						cbFailed;
typedef boost::function<void ( const size_t current, const size_t max, const std::string& message )>    cbProgress;

/**
 * General purpose function to report progress events to the user.
 */
class ProgressFeedback {
public:

	/**
	 * Progress feedback constructor
	 */
	ProgressFeedback 	() : startedCallbacks(), completedCallbacks(), failedCallbacks(), progressCallbacks(), subtasks(), max(0), current(0) { };

	/**
	 * Register callbacks on progress events
	 */
	void onStarted		( cbStarted & cb );
	void onCompleted	( cbCompleted & cb );
	void onFailed		( cbFailed & cb );
	void onProgress		( cbProgress & cb );

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
	 * Update the maximum number of steps in this task 
	 */
	void 				setMax 		( const size_t max = 0);

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
	void 				completed 	( );

	/**
	 * Fail the current task 
	 */
	void 				failed 		( const std::string& message, const int errorCode = 0 );

private:

	// Metrics
	size_t 								max;
	size_t 								current;

	// Callback list
	std::vector< cbStarted >			startedCallbacks;
	std::vector< cbCompleted >			completedCallbacks;
	std::vector< cbFailed >				failedCallbacks;
	std::vector< cbProgress >			progressCallbacks;

	std::list< ProgressFeedbackPtr > 	subtasks;

	// Internal callbacks

};

#endif /* end of include guard: PROGRESSFEEDBACK_H */
