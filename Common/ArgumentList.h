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
#ifndef ARGUMENTS_LIST_H
#define ARGUMENTS_LIST_H

#include <boost/variant.hpp>
#include <string>
#include <vector>

/* Typedef for variant callbacks */
typedef boost::variant< float, double, int, std::string >								VariantArg;
typedef std::vector< VariantArg >														VariantArgList;

/**
 * Helper class for easily building callback arguments
 */
class ArgumentList {
public:
	ArgumentList( ) : args() { };
	ArgumentList( VariantArg arg ) : args(1,arg) { };
	ArgumentList& operator()( VariantArg arg ) {
		args.push_back(arg);
		return *this;
	}
	operator VariantArgList& () {
		return args;
	}
private:
	VariantArgList 		args;

};

#endif /* end of include guard: ARGUMENTS_LIST_H */
