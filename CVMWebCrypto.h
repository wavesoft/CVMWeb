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

#ifndef CRYPTO_H_41EFXAZS
#define CRYPTO_H_41EFXAZS

#include "Utilities.h"
#include "Hypervisor.h"

#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

/**
 * Cryptographic and validation routines for CernVM Web API
 */
class CVMWebCrypto 
{
public:
    
    /**
     * Constructor: Load public key in memory and initialize the cryptographic subsystem
     */
    CVMWebCrypto                        ();
    
    /**
     * Destructor: Cleanup public key
     */
    virtual ~CVMWebCrypto               ();

    /**
     * Update the authorized domain keystore from out webserers.
     */
    int     updateAuthorizedKeystore    ();

    /**
     * Verify the authenticity of the contextualization information received form the given domain
     */
    int     validateDomainData          ( std::string domain, std::string data, std::string signature );

    /**
     * Check if the given domain name is listed in the authorized domain keys
     */ 
    bool    isDomainValid               ( std::string domain );

    /**
     * Denotes that the class is valid and ready for use
     */
    bool    valid;

private:
    
    /**
     * The global static RSA object used for list validation 
     */
    EVP_PKEY * cvmPublicKey;

    /**
     * Domain-keydata mapping
     */
    std::map< std::string, std::string > domainKeys;

};

#endif /* end of include guard: CRYPTO_H_41EFXAZS */
