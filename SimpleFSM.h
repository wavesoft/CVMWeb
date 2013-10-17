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

#ifndef SIMPLEFSM_H
#define SIMPLEFSM_H

#include "Utilities.h"
#include <list>
#include <vector>
#include <map>

typedef boost::function< void () >	fsmHandler;

// Forward declerations
struct  _FSMNode;
typedef _FSMNode FSMNode;

/**
 * Structure of the FSM node
 * This is in principle a directional graph
 */
struct  _FSMNode{

	// FSM description
	int								id;
	unsigned char 					type;
	fsmHandler						handler;
	std::vector<FSMNode*>			children;

};

/**
 * Helper macro for FSM registry
 */
#define FSM_REGISTRY(root,block) \
 	FSMRegistryBegin();\
 	block; \
 	FSMRegistryEnd(root);

#define FSM_HANDLER(id,cb,...) \
 	FSMRegistryAdd(id, boost::bind(cb, this), __VA_ARGS__, 0);

#define FSM_STATE(id,...) \
 	FSMRegistryAdd(id, 0, __VA_ARGS__, 0);

/**
 * Auto-routed Finite-State-Machine class
 */
class SimpleFSM {
public:

	/**
	 * Pick the closest path to go from current state to the given state
	 */
	void 							FSMGoto(int state);

	/**
	 * Skew the current path by switching to given state and then continuing
	 * to the state pointed by goto
	 */
	void 							FSMSkew(int state);

	/**
	 * Called externally to continue to the next FSM action
	 */
	bool 							FSMContinue();

protected:

	void 					        FSMRegistryBegin();
	void 					        FSMRegistryAdd( int id, fsmHandler handler, ... );
	void 					        FSMRegistryEnd( int rootID );

    std::map<int,std::vector<int>>  fsmTmpRouteLinks;
	std::map<int,FSMNode>	        fsmNodes;
	FSMNode	*						fsmRootNode;
	FSMNode *						fsmCurrentNode;
	std::list<FSMNode*>				fsmCurrentPath;
	int								fsmTargetState;
	bool 							fsmInsideHandler;

};


#endif