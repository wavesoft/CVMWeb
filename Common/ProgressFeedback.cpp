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


/**
 * Register a callback that handles the 'started' event
 */
void BaseProgressTask::onStarted ( cbStarted & cb ) {
	startedCallbacks.push_back(cb);
}

/**
 * Register a callback that handles the 'completed' event
 */
void BaseProgressTask::onCompleted ( cbCompleted & cb ) {
	completedCallbacks.push_back(cb);
}

/**
 * Register a callback that handles the 'failed' event
 */
void BaseProgressTask::onError	( cbError & cb ) {
	errorCallbacks.push_back(cb);
}

/**
 * Register a callback that handles the 'progress' event
 */
void BaseProgressTask::onProgress ( cbProgress & cb ) {
	progressCallbacks.push_back(cb);
}

/**
 * Forward progress event
 * TODO: Optimizations are needed (too many nested loops on calculating getProgress)
 */
void _forwardProgress( const std::string& message ) {

	// Call all the progress callbacks
	for (std::vector< cbProgress >::iterator it = progressCallbacks.begin(); it != progressCallbacks.end(); ++it) {
		cbProgress cb = *it;
		cb( getProgress(), message );
	}

	// Forward event to the parent elements
	if (parent)
		parent._forwardProgress( message );

}

/**
 * Forward error event
 */
void _forwardError( const std::string& message, const int errorCode ) {

	// Call all the progress callbacks
	for (std::vector< cbProgress >::iterator it = errorCallbacks.begin(); it != errorCallbacks.end(); ++it) {
		cbFailed cb = *it;
		cb( message, errorCode );
	}

	// Forward event to the parent elements
	if (parent)
		parent._forwardError( message, errorCode );

}


/**
 * Create a new subtask with finite number of tasks
 */
SlottedTaskPtr BaseProgressTask::beginTasks ( const std::string& taskName, const size_t max ) {

	// Create new ProgressFeedback sub-object
	SlottedTaskPtr newTask = make_shared<SlottedTask>(taskName, max);
	newTask.parent = shared_from_this();

	// Push on sub-tasks
	subtasks.push_back( newTask );

	// Return new task
	return newTask;

}

/**
 * Create a new subtask for a single task that takes long time to complete
 */
ProgressTaskPtr BaseProgressTask::beginProgress ( const std::string& taskName, const size_t max ) {

	// Create new ProgressFeedback sub-object
	ProgressTaskPtr newTask = make_shared<ProgressTask>(taskName, max);
	newTask.parent = shared_from_this();

	// Push on sub-tasks
	subtasks.push_back( newTask );

	// Return new task
	return newTask;

}

/**
 * Create a new subtask
 */
UnknownTaskPtr BaseProgressTask::beginUnknown ( const std::string& taskName ) {

	// Create new ProgressFeedback sub-object
	UnknownTaskPtr newTask = make_shared<UnknownTask>(taskName);
	newTask.parent = shared_from_this();

	// Push on sub-tasks
	subtasks.push_back( newTask );

	// Return new task
	return newTask;

}

/**
 * By default return the summarized progress stack
 */
double BaseProgressTask::getProgress() {

	// Calculate the percentage per slot
	double v = 1.0 / subtasks.size();
	double value = 0.0;

	// Summarize according to percentage per child
	for (std::list< ProgressFeedbackPtr >::iterator it = subtasks.begin(); it != subtasks.end(); ++it) {
		ProgressFeedbackPtr pf = *it;
		value += v * pf.getProgress();
	}

	// Return summarized value
	return v;

}

