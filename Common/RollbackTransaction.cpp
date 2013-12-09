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

#include "RollbackTransaction.h"

/**
 * Fire the callback function, ignoring any possible exceptions
 */
void RollbackTransactionEntry::call() {
	try {
		callback(arguments);
	} catch (...) {
		CVMWA_LOG("Error", "Error handling exception");
	}
}

/**
 * Register a rollback action
 */
void RollbackTransaction::add( const callbackTransaction & callback, ... ) {
    va_list pl;
    void * l;
    std::vector<void *> args;

    // Store all arguments
    va_start(pl, callback);
    while ((l = va_arg(pl,void *)) != 0) {
        args.push_back(l);
    }
    va_end(pl);

    // Store in database
    actions.push_back( RollbackTransactionEntry( callback, args ) );

}
