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
#ifndef COMMON_CRYPTO_H
#define COMMON_CRYPTO_H

#include <Common/Config.h>
#include <Common/DownloadProvider.h>
#include <Common/Utilities.h>
#include <Common/Hypervisor.h>
#include <Common/LocalConfig.h>
#include <Common/CrashReport.h>

#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

/**
 * Cryptographic and validation routines for CernVM Web API
 */
class DomainKeystore 
{
public:

    /**
     * Static function to initialize the cryptographic subsystem
     */
    static int Initialize();

    /**
     * Static function to cleanup the cryptographic subsystem
     */
    static int Cleanup();
    
    /**
     * Constructor: Load public key in memory and initialize the cryptographic subsystem
     */
    DomainKeystore                        ();
    
    /**
     * Destructor: Cleanup public key
     */
    virtual ~DomainKeystore               ();

    /**
     * Update the authorized domain keystore from out webserers.
     */
    int     updateAuthorizedKeystore    ( DownloadProviderPtr );

    /**
     * Verify the authenticity of the contextualization information received form the given domain
     */
    int     validateDomainData          ( std::string domain, std::string signature, const unsigned char * pData, const int pDataLen );

    /**
     * Check if the given domain name is listed in the authorized domain keys
     */ 
    bool    isDomainValid               ( std::string domain );

    /**
     * Signature validation functions
     */
    int     signatureValidate           ( std::string& domain, std::string& salt, ParameterMapPtr data  );

    /**
     * Generate a unique salt
     */
    std::string generateSalt            ( );

    /**
     * Denotes that the class is valid and ready for use
     */
    bool    valid;

private:

    /**
     * Domain-keydata mapping
     */
    std::map< std::string, std::string > domainKeys;
    
    /**
     * Signature calculation properties
     */
    std::string signatureData;
    
    /**
     * LocalConfig for accessing the key store
     */
    LocalConfigPtr config;
    
    /**
     * The last time we loaded the keystore
     */
    time_t lastUpdateTimestamp;
    
    /**
     * The timestamp of the local keystore
     */
    time_t keystoreTimestamp;

};

#endif /* end of include guard: COMMON_CRYPTO_H */
