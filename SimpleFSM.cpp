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
	fsmCurrentNode = fsmRootNode;

	// Flush temp arrays
	fsmTmpRouteLinks.clear();

}

/**
 * Complete FSM registry decleration and build FSM tree
 */
bool SimpleFSM::FSMContinue() {
	if (fsmInsideHandler) return;
	if (fsmCurrentPath.empty()) return;
	fsmInsideHandler = true;

	// Get next action in the path
	FSMNode * next = fsmCurrentPath.front();
    fsmCurrentPath.pop_front();

	// Skip state nodes
	while ((!next->handler) && !fsmCurrentPath.empty()) {
		next = fsmCurrentPath.front();
        fsmCurrentPath.pop_front();
	}

	// Use guarded execution
	try {

		// Run the new state
		if (next->handler)
			next->handler();

	} catch ( std::exception &e ) {
		CVMWA_LOG("Exception", e.what() );
	} catch ( ... ) {
		CVMWA_LOG("Exception", "Unknown exception" );
	}

	// We are now outside the handler
	fsmInsideHandler = false;

}

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
    CVMWA_LOG("Debug", "Going towards " << state);

	// Reset path
	fsmCurrentPath.clear();

	// Prepare clip length
	size_t clipLength = fsmNodes.size();

	// Prepare receiver
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
    CVMWA_LOG("Debug", "Skewing througgh " << state << " towards " << fsmTargetState);
	std::map<int,FSMNode>::iterator pt;

	// Search given state
	pt = fsmNodes.find( state );
	if (pt == fsmNodes.end()) return;

	// Switch current node to the skewed state
	fsmCurrentNode = &((*pt).second);

	// Continue towards the active target
	FSMGoto( fsmTargetState );

}