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

#include "SimpleFSM.h"
#include <cstdarg>
#include <stdexcept>
#include <iostream>

/**
 * Reset FSM registry variables
 */
void SimpleFSM::FSMRegistryBegin() {
    // Reset
    fsmNodes.clear();
    fsmTmpRouteLinks.clear();
    fsmCurrentPath.clear();
    fsmRootNode = NULL;
    fsmCurrentNode = NULL;
}

/**
 * Add entry to the FSM registry
 */
void SimpleFSM::FSMRegistryAdd( int id, fsmHandler handler, ... ) {
    std::vector<int> v;
    va_list pl;
    int l;

    // Initialize node
    fsmNodes[id].id = id;
    fsmNodes[id].handler = handler;
    fsmNodes[id].children.clear();
    
    // Store route mapping to temp routes vector
    // (Will be synced by FSMRegistryEnd)
    va_start(pl, handler);
    while ((l = va_arg(pl,int)) != 0) {
        v.push_back(l);
    }
    va_end(pl);
    fsmTmpRouteLinks[id] = v;

}

/**
 * Complete FSM registry decleration and build FSM tree
 */
void SimpleFSM::FSMRegistryEnd( int rootID ) {
	std::map<int,FSMNode>::iterator pt;
	std::vector<FSMNode*> 			nodePtr;
	std::vector<int> 				links;

	// Build FSM linked list
	for (std::map<int,FSMNode>::iterator it = fsmNodes.begin(); it != fsmNodes.end(); ++it) {
		int id = (*it).first;
		FSMNode * node = &((*it).second);

		// Create links
		nodePtr.clear();
		links = fsmTmpRouteLinks[id];
		for (std::vector<int>::iterator jt = links.begin(); jt != links.end(); ++jt) {

			// Get pointer to node element in map
			pt = fsmNodes.find( *jt );
			FSMNode * refNode = &((*pt).second);

			// Update node
			node->children.push_back( refNode );

		}

	}

	// Fetch root node
	fsmTargetState = rootID;
	pt = fsmNodes.find( rootID );
	fsmRootNode = &((*pt).second);

	// Reset current node
	fsmCurrentNode = fsmRootNode;

	// Flush temp arrays
	fsmTmpRouteLinks.clear();

}

/**
 * Helper function to call a handler
 */
bool SimpleFSM::_callHandler( FSMNode * node, bool inThread ) {

	// Use guarded execution
	try {

		// Run the new state
		if (node->handler)
			node->handler();

	} catch (boost::thread_interrupted &e) {
		CVMWA_LOG("Debuf", "FSM Handler interrupted");

		// Cleanup
		fsmInsideHandler = false;

		if (inThread) {
			// If we are in-thread, re-throw so it's
			// catched with the external wrap
			throw;
		} else {
			// Otherwise, return false to indicate that
			// something failed.
			return false;
		}

	} catch ( std::exception &e ) {
		CVMWA_LOG("Exception", e.what() );
		return false;

	} catch ( ... ) {
		CVMWA_LOG("Exception", "Unknown exception" );
		return false;

	}

	// Executed successfully
	return true;

}

/**
 * Run next action in the FSM
 */
bool SimpleFSM::FSMContinue( bool inThread ) {
	if (fsmInsideHandler) return false;
	if (fsmCurrentPath.empty() && (fsmCurrentNode != NULL)) return false;
	fsmInsideHandler = true;

	// Get next action in the path
	FSMNode * next = fsmCurrentPath.front();
    fsmCurrentPath.pop_front();

	// Skip state nodes
	while ((!next->handler) && !fsmCurrentPath.empty()) {
		next = fsmCurrentPath.front();
        fsmCurrentPath.pop_front();
	}

	// Change current node
	fsmCurrentNode = next;

	// Call handler
	if (!_callHandler(next, inThread)) 
		return false;

	// We are now outside the handler
	fsmInsideHandler = false;
    return true;
}

/**
 * Helper function to traverse the FSM graph, trying to find the
 * shortest path that leads to the given action.
 */
void findShortestPath( std::vector< FSMNode * > path, FSMNode * node, int state,	// Arguments
					  size_t * clipLength, std::vector< FSMNode * > ** bestPath ) {	// State

	// Update path
	path.push_back( node );

	// Clip protection
	if (path.size() >= *clipLength)
		return;

	// Iterate through the children
	for (std::vector<FSMNode*>::iterator it = node->children.begin(); it != node->children.end(); ++it) {
		FSMNode * n = *it;

		// Ignore if it's already visited
		if (std::find(path.begin(), path.end(), n) != path.end())
			continue;

		// Check if we found the node
		if (n->id == state) {

			// Set new clip path
			*clipLength = path.size();
			path.push_back( n );

			// Replace best path
			if (*bestPath != NULL) delete *bestPath;
			*bestPath = new std::vector<FSMNode*>(path);

#ifdef LOGGING
			// Present best path
			std::ostringstream oss;
			for (std::vector<FSMNode*>::iterator j= (*bestPath)->begin(); j!=(*bestPath)->end(); ++j) {
				if (!oss.str().empty()) oss << ", "; oss << (*j)->id;
			}
			CVMWA_LOG("Debug", "Preferred path: " << oss.str() );
#endif

			// We cannot continue (since we found a better path)
			return;
		}

		// Continue with the items
		findShortestPath( path, n, state, clipLength, bestPath );
	}
}

/**
 * Build the path to go to the given state and start the FSM subsystem
 */
void SimpleFSM::FSMGoto(int state) {

	// Allow only one thread to steer the FSM
	boost::mutex::scoped_lock lock(fsmGotoMutex);

    CVMWA_LOG("Debug", "Going towards " << state);

	// Reset path
	fsmCurrentPath.clear();

	// Prepare clip length
	size_t clipLength = fsmNodes.size();

	// Prepare solver
	std::vector< FSMNode * > srcPath;
	std::vector< FSMNode * > *resPath = NULL;
	findShortestPath( srcPath, fsmCurrentNode, state, &clipLength, &resPath );

	// Check if we actually found a path
	if (resPath != NULL) {

		// Update target path and release resPath memory
		fsmCurrentPath.assign( resPath->begin()+1, resPath->end() );
		delete resPath;

		// Switch active target
		fsmTargetState = state;

	}

	// If we have progress feedback, update the max
	if (fsmProgress) {

		// Count the actual tasks in the path (skipping state nodes)
		int pathCount = 0;
		for (std::list<FSMNode*>::iterator j= fsmCurrentPath.begin(); j!=fsmCurrentPath.end(); ++j) {
			FSMNode* node = *j;
			if (node->handler) pathCount++;
		}

		// Restart progress
        fsmProgress->restart( fsmProgressResetMsg, false );

		// Update max tasks that will be passed through
        fsmProgress->setMax( pathCount, false );

	}

	// Notify possibly paused thread
	if (fsmThread != NULL)
		_fsmWakeup();

#ifdef LOGGING
	// Present best path
	std::ostringstream oss;
	for (std::list<FSMNode*>::iterator j= fsmCurrentPath.begin(); j!=fsmCurrentPath.end(); ++j) {
		if (!oss.str().empty()) oss << ", "; oss << (*j)->id;
	}
	CVMWA_LOG("Debug", "Best path: " << oss.str() );
#endif

}


/**
 * Skew the current path by switching to given state and then continuing
 * to the state pointed by goto
 */
void SimpleFSM::FSMSkew(int state) {
    CVMWA_LOG("Debug", "Skewing through " << state << " towards " << fsmTargetState);
	std::map<int,FSMNode>::iterator pt;

	// Search given state
	pt = fsmNodes.find( state );
	if (pt == fsmNodes.end()) return;

	// Switch current node to the skewed state
	fsmCurrentNode = &((*pt).second);

	// Continue towards the active target
	FSMGoto( fsmTargetState );

}

/**
 * Local function to do FSMContinue when needed in a threaded way
 */
void SimpleFSM::FSMThreadLoop() {

	// Catch interruptions
	try {

		// Infinite thread loop
		while (true) {

			// Keep running until we run out of steps
			while (FSMContinue(true)) {

				// Yield our time slice after executing an action
				fsmThread->yield();

			};

			// After the above loop, we have drained the
			// event queue. Enter paused state and wait
			// to be notified when this changes.
			_fsmPause();
		}

	} catch (boost::thread_interrupted &e) {
		CVMWA_LOG("Debug", "Thread interrupted");

	}

	// Cleanup
	fsmThread = NULL;
}

/**
 * Local function to exit the FSM Thread
 */
void SimpleFSM::FSMThreadStop() {

	// Ensure we have a running thread
	if (fsmThread == NULL) return;

	// Interrupt and reap
	fsmThread->interrupt();
	fsmThread->join();

}

/**
 * Infinitely pause, waiting for a wakeup signal
 */
void SimpleFSM::_fsmPause() {
	CVMWA_LOG("Debug", "Entering paused state");

	// If we are already not paused, don't
	// do anything
	if (fsmtPaused) {
	    boost::unique_lock<boost::mutex> lock(fsmtPauseMutex);
	    while(fsmtPaused) {
	        fsmtPauseChanged.wait(lock);
	    }
	}

    // Reset paused state
    fsmtPaused = true;
	CVMWA_LOG("Debug", "Exiting paused state");

}

/**
 * Send a wakeup signal
 */
void SimpleFSM::_fsmWakeup() {
	CVMWA_LOG("Debug", "Waking-up paused thread");

    {
        boost::unique_lock<boost::mutex> lock(fsmtPauseMutex);
        fsmtPaused = false;
    }
    fsmtPauseChanged.notify_all();
}

/**
 * Set a progress object to use with SimpleFSM.
 *
 * This will enable automatic re-calibration of the maximum elements of the finite-task
 * progress feedback mechanism when the FSM engine is picking a different path.
 *
 */
void SimpleFSM::FSMUseProgress ( const FiniteTaskPtr & pf, const std::string & resetMessage ) {
	fsmProgress = pf;
	fsmProgressResetMsg = resetMessage;
}

/**
 * Trigger the "Doing" action of the SimpleFSM progress feedback -if available-
 *
 * This function should be placed in the beginning of all the FSM action handlers in order
 * to provide more meaningul message to the user.
 */
void SimpleFSM::FSMDoing ( const std::string & message ) {
	CVMWA_LOG("Debug", "Doing " << message);
	if (fsmProgress) {
		fsmProgress->doing(message);
	}
}

/**
 * Trigger the "Done" action of the SimpleFSM progress feedback -if available-
 *
 * This function should be placed in the end of all the FSM action handlers in order
 * to provide more meaningul message to the user.
 */
void SimpleFSM::FSMDone ( const std::string & message ) {
	CVMWA_LOG("Debug", "Done " << message);
	if (fsmProgress) {
		fsmProgress->done(message);
	}
}

/**
 * Trigger the "begin" action of the SimpleFSM progress feedback.
 * This function cannot be used when FSMDoing/FSMDone are used.
 */
template <typename T> boost::shared_ptr<T> 
SimpleFSM::FSMBegin( const std::string& message ) {
	boost::shared_ptr<T> ptr = boost::shared_ptr<T>();
	if (fsmProgress) {
		ptr = fsmProgress->begin<T>(message);
	}
	return ptr;
}

/**
 * Trigger the "Fail" action of the SimpleFSM progress feedback -if available-
 */
void SimpleFSM::FSMFail ( const std::string & message, const int errorCode ) {
	CVMWA_LOG("Debug", "Done " << message);
	if (fsmProgress) {
		fsmProgress->fail(message, errorCode);
	}
}

/**
 * Wait until the FSM reaches the specified state
 */
void SimpleFSM::FSMWaitFor	( int state, int timeout ) {

	// Find the state
	std::map<int,FSMNode>::iterator pt;
	pt = fsmNodes.find( state );
	if (pt == fsmNodes.end()) return;

	// If we are already on this state, don't do anything
	if (fsmCurrentNode == fsmwState) return;

	// Switch to the target state
	fsmwState = &((*pt).second);

	/*
    {
        boost::unique_lock<boost::mutex> lock(fsmwStateMutex);
        fsmtPaused = false;
    }
    fsmwStateChanged.notify_all();
	*/

	// Wait for state
    boost::unique_lock<boost::mutex> lock(fsmwStateMutex);
    fsmwStateChanged.wait(lock);

}

/**
 * Start an FSM thread
 */
boost::thread * SimpleFSM::FSMThreadStart() {
	// If we are already running, return the thread
	if (fsmThread != NULL)
		return fsmThread;
	
	// Set the current state to paused, effectively
	// stopping the FSM execution if no wakeup signals are piled
	fsmtPaused = true;

	// Start and return the new thread
	fsmThread = new boost::thread(boost::bind(&SimpleFSM::FSMThreadLoop, this));
	return fsmThread;
}

/**
 * Release mutex upon destruction
 */
SimpleFSM::~SimpleFSM() {

	// Join thread
	FSMThreadStop();

}

/**
 * Implement some known templates
 */
template FiniteTaskPtr      SimpleFSM::FSMBegin<FiniteTask>( const std::string& message );
template VariableTaskPtr    SimpleFSM::FSMBegin<VariableTask>( const std::string& message );
template BooleanTaskPtr     SimpleFSM::FSMBegin<BooleanTask>( const std::string& message );
