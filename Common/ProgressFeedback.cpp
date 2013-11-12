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

#include "ProgressFeedback.h"

/** #####################################################################################
 *  # ProgressTask Implementation - Base Task
 ** ##################################################################################### */

/**
 * Register a callback that handles the 'started' event
 */
void ProgressTask::onStarted ( const cbStarted & cb ) {
	startedCallbacks.push_back(cb);
}

/**
 * Register a callback that handles the 'completed' event
 */
void ProgressTask::onCompleted ( const cbCompleted & cb ) {
	completedCallbacks.push_back(cb);
}

/**
 * Register a callback that handles the 'failed' event
 */
void ProgressTask::onError	( const cbError & cb ) {
	errorCallbacks.push_back(cb);
}

/**
 * Register a callback that handles the 'progress' event
 */
void ProgressTask::onProgress ( const cbProgress & cb ) {
	progressCallbacks.push_back(cb);
}

/**
 * Mark the task as completed
 */
void ProgressTask::complete ( const std::string& message ) {
    std::string msg = message;

	// Build empty message
	if (message.empty()) {
		if (!lastMessage.empty()) {
			msg = "Completed " + lastMessage;
        }
	}

	// Notify update
	_notifyCompleted(message);

}

/**
 * Forward a message without any progress update
 */
void ProgressTask::doing ( const std::string& message ) {

	// Notify update
	_notifyUpdate(message);

}

/**
 * Let listeners know that we are completed
 */
void ProgressTask::_notifyCompleted ( const std::string& message ) {

	// If we are alrady completed, do nothing
	if (completed) return;

	// Mark as completed
	completed = true;

	// Store the last message
	lastMessage = message;

	// Fire events
	for (std::vector< cbCompleted >::iterator it = completedCallbacks.begin(); it != completedCallbacks.end(); ++it) {
		cbCompleted cb = *it;
		if (cb) cb( message );
	}

	// Propagate event
	if (parent) {
		parent->_notifyUpdate( message );
	}
}

/**
 * Check for state of this progress event
 */
void ProgressTask::_notifyUpdate ( const std::string& message ) {

	// Store the last message
	lastMessage = message;

	// Check if we are completed
	if (isCompleted()) {

		// Send a completion notification
		_notifyCompleted( message );

	} else {

		// Call all the progress callbacks
		for (std::vector< cbProgress >::iterator it = progressCallbacks.begin(); it != progressCallbacks.end(); ++it) {
			cbProgress cb = *it;
			if (cb) cb( message, getProgress() );
		}

		// Propagate event
		if (parent) {
			parent->_notifyUpdate( message );
		}

	}
}

/**
 * Let everybody know that we started the event
 */
void ProgressTask::_notifyStarted ( const std::string& message ) {

	// Don't do such thing if we are already started
	if (started) return;

	// Mark us as started
	started = true;

	// Store the last message
	lastMessage = message;

	// Fire callbacks
	for (std::vector< cbStarted >::iterator it = startedCallbacks.begin(); it != startedCallbacks.end(); ++it) {
		cbStarted cb = *it;
		if (cb) cb( message );
	}

	// Propagate event
	if (parent) {
		parent->_notifyStarted( message );
	}

}

/**
 * Forward progress event
 * TODO: Optimizations are needed (too many nested loops on calculating getProgress)
 */
void ProgressTask::_forwardProgress( const std::string& message ) {

	// Store the last message
	lastMessage = message;

	// Call all the progress callbacks
	for (std::vector< cbProgress >::iterator it = progressCallbacks.begin(); it != progressCallbacks.end(); ++it) {
		cbProgress cb = *it;
		cb( message, getProgress() );
	}

	// Forward event to the parent elements
	if (parent)
		parent->_forwardProgress( message );

}

/**
 * Forward error event
 */
void ProgressTask::_forwardError( const std::string& message, const int errorCode ) {

	// Store the last message
	lastMessage = message;

	// Call all the progress callbacks
	for (std::vector< cbError >::iterator it = errorCallbacks.begin(); it != errorCallbacks.end(); ++it) {
		cbError cb = *it;
		cb( message, errorCode );
	}

	// Forward event to the parent elements
	if (parent)
		parent->_forwardError( message, errorCode );

}

/** #####################################################################################
 *  # FiniteTask Implementation - Finite Task Set
 ** ##################################################################################### */

/**
 * Check if the task is completed
 */
bool FiniteTask::isCompleted ( ) {

	// If I am already completed, return true
	if (completed) return true;

	// If I haven't started, return false
	if (!started) return false;

	// Get the number of tasks
	size_t len = tasks.size();

	// Loop over tasks
	for (int i=0; i<len; i++) {

		if (tasks[i] == 0) {
			// If task is not completed, just return false
			return false;

		} else if (tasks[i] == 2) {
			// If we have a sub-task, and it's not finished, return false
			if (!taskObjects[i]->isCompleted())
				return false;

		}

	}

	// Everything looks good
	return true;

}

/**
 * Return progress event
 */
double FiniteTask::getProgress ( ) {

	// If I am already completed, return 1.0
	if (completed) return 1.0;

	// If I haven't started, return 0.0
	if (!started) return 0.0;

	// Get the number of tasks
	size_t len = tasks.size();

	// Calculate the step size
	double stepSize = 1.0 / (double)len;
	double value = 0.0;

	// Loop over tasks
	for (int i=0; i<len; i++) {

		if (tasks[i] == 1) {
			// If task is completed, add a step
			value += stepSize;

		} else if (tasks[i] == 2) {
			// If we have a sub-task, calculate the sub-progress
			value += stepSize * taskObjects[i]->getProgress();

		}

	}

	// Return value
	return value;

}

/**
 * Set maximum number of tasks
 */
void FiniteTask::setMax ( size_t maxTasks ) {

	// Get the size of the array
	size_t len = tasks.size();

	if (maxTasks < len) {
		// Delete elements if smaller
		tasks.resize( maxTasks );
		taskObjects.resize( maxTasks );

	} else if (maxTasks > len) {
		// Allocate more elements in 'unfinished' state
		tasks.resize( maxTasks, 0 );
		taskObjects.resize( maxTasks );

	}

	// Notify update events if we are in the middle of something
	if (started) _notifyUpdate( lastMessage );

}

/**
 * Mark the task as completed
 */
void FiniteTask::done( const std::string& message ) {

	// Notify started
	_notifyStarted( message );

	// Get the size of the array
	size_t len = tasks.size();

	// Allocate task only if we have enough slots
	if (taskIndex < len) {

		// Mark the given task as completed
		tasks[taskIndex] = 1;

		// Go to next step
		taskIndex += 1;

	}

	// Forward update
	_notifyUpdate( message );


}

/**
 * Allocate a new child task
 */
template <typename T> boost::shared_ptr<T> 
FiniteTask::begin( const std::string& message ) {

	// Notify started
	_notifyStarted( message );

	// Get the size of the array
	size_t len = tasks.size();

    // Create a new object instance
    boost::shared_ptr<T> newPtr = boost::make_shared<T>();
    newPtr->parent = shared_from_this();

	// Allocate task only if we have enough slots
	if (taskIndex < len) {

		// Mark the given task as object-based
		tasks[taskIndex] = 2;

		// Make a new shared object of the given kind
		taskObjects[taskIndex] = newPtr;

		// Go to next step
		taskIndex += 1;

	}

	// Forward update
	_notifyUpdate( message );

    // Return object
    return newPtr;

}

/** #####################################################################################
 *  # VariableTask Implementation - Variable Task Set
 ** ##################################################################################### */

/**
 * Check if the task is completed
 */
bool VariableTask::isCompleted ( ) {

	// If I haven't started, return false
	if (!started) return false;

	// If I am already completed, return true
	if (completed) return true;

	// If we have current == max return true
	if (current == max) return true;

	// Otherwise, false
	return false;

}

/**
 * Return progress event
 */
double VariableTask::getProgress ( ) {

	// If I am already completed, return 1.0
	if (completed) return 1.0;

	// If I haven't started, return 0.0
	if (!started) return 0.0;

	// Return ammount of tasks completed
	return (double)current / (double)max;

}

/**
 * Set max value
 */
void VariableTask::setMax ( size_t maxValue ) {

	// Update max value
	max = maxValue;

	// Trigger update if we were in the middle of something
	if (started) _notifyUpdate( lastMessage );

}

/**
 * Update value
 */
void VariableTask::update ( size_t value ) {

	// Notify started
	_notifyStarted( lastMessage );

	// Update value
	current = value;

	// Trigger update
	_notifyUpdate( lastMessage );

}

/**
 * Update the default value
 */
void VariableTask::setMessage ( const std::string& message ) {
	lastMessage = message;
}


/** #####################################################################################
 *  # BooleanTask Implementation - Variable Task Set
 ** ##################################################################################### */

/**
 * Check if the task is completed
 */
bool BooleanTask::isCompleted ( ) {

	// If I am already completed, return 1.0
	if (completed) return true;

	// If I haven't started, return 0.0
	if (!started) return false;

	// Otherwise I am not completed
	return true;

}

/**
 * Return progress event
 */
double BooleanTask::getProgress ( ) {

	// If I am already completed, return 1.0
	if (completed) return 1.0;

	// If I haven't started, return 0.0
	if (!started) return 0.0;

	// Otherwise I am not completed
	return 0.0;
}

/**
 * Reset the state to non-completed
 */
void BooleanTask::reset ( ) {
	completed = false;
}

/**
 * Implement some known templates
 */
template FiniteTaskPtr      FiniteTask::begin<FiniteTask>( const std::string& message );
template VariableTaskPtr    FiniteTask::begin<VariableTask>( const std::string& message );
template BooleanTaskPtr     FiniteTask::begin<BooleanTask>( const std::string& message );
