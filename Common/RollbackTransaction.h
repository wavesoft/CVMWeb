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
#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "Utilities.h"  // It also contains the common global headers
#include "CrashReport.h"

#include <vector>

/**
 * The transaction rollback action template
 */
typedef boost::function< void (const std::vector<void *>& ) >		callbackTransaction;

/**
 * A rollback transaction entry in the RollbackTransaction class
 */
class RollbackTransactionEntry {
public:

	/**
	 * Construct a rollback transaction entry
	 */
	RollbackTransactionEntry( const callbackTransaction & cb, const std::vector<void *> & args ) :
		callback(cb), arguments(args) { };

	/**
	 * The function to call in order to roll-back the actions taken
	 */
	callbackTransaction		callback;

	/**
	 * The data array to pass to the function
	 */
	std::vector<void *>		arguments;

	/**
	 * Fire the callback function
	 */
	void 					call();

};

/**
 * A rollback transaction class allows a set of actions
 * to be executed in reverse order if a critical error occures.
 */
class RollbackTransaction {
public:

	/**
	 * "Checkpoint" the transaction, effectively discarding
	 * all the stored actions. This is an alias to 'clear'
	 */
	void 					checkpoint();

	/**
	 * Discard all the stored rollback actions.
	 */
	void 					clear();

	/**
	 * Add a rollback action.
	 */
	void 					add( const callbackTransaction & callback, ... );

	/**
	 * Rollback actions.
	 */
	void 					rollback();


private:

	/**
	 * A list of transaction actions to execute
	 */
	std::vector<RollbackTransactionEntry>	actions;

};

#endif /* end of include guard: TRANSACTION_H */
