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

#include "CVMWebCrypto.h"

using namespace std;

/**
 * Public key used for key-list validation
 */
const int CVMWAPI_PUBKEY_DER_SIZE        = 1598;
const unsigned char * CVMWAPI_PUBKEY_DER = (const unsigned char *)
    "\x30\x82\x02\x22\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01\x05\x00\x03\x82\x02\x0f\x00\x30\x82\x02\x0a\x02\x82\x02\x01"
    "\x00\xe5\x61\x3f\x86\xf9\x16\x4b\x7c\x92\x14\xbb\xf0\x5d\xd1\x07\x2e\xdc\x70\x46\x41\xea\x48\x32\x64\x6e\x6a\x7c\x62\xcf\x61\x7f"
    "\xbe\x51\x2e\x25\xdd\xf8\x86\xf6\xb6\x3b\x81\x11\x04\x7b\xf8\xdd\x14\xcf\x10\xd5\x20\xc5\xda\xfb\x50\x00\x5e\x5e\xce\x5a\x8a\xbe"
    "\x99\xfb\xbb\x5f\xd2\xcd\x39\x6b\xc1\xd7\x8c\x7f\x0b\x28\x3a\xdb\x06\xa8\xb0\x8e\xca\xae\xa6\x96\xbe\x36\xd2\x1a\x57\x3a\x52\x51"
    "\xd8\xf4\xa0\x9f\xea\x3c\x14\x44\xec\x83\xdf\x98\x3e\x3c\xd8\xb0\x8c\xca\xc7\xbf\xc1\xd7\xfe\x37\x14\x8f\x71\x8f\x3d\x8b\xa2\x70"
    "\x95\x84\xa9\x90\x52\x7a\x32\x45\x91\x14\x5c\x3d\x03\x0d\xc9\xd4\xfe\xa3\x3e\xe8\xfa\xdd\x39\x15\xda\x1d\x01\x23\x37\x3f\x94\x66"
    "\x4f\x37\xda\xf2\xc7\x5e\x1f\x8b\xdf\xce\x76\x52\x2d\x71\x65\x55\xe1\xb6\xb5\xfd\x76\x7b\x0c\x0e\xc9\xc6\x9f\x00\xb6\xfc\x21\x72"
    "\x34\xd9\xe6\x84\x9d\xb5\xa1\xc8\x0b\xb3\xdf\xda\x7e\xc8\xab\x1a\x89\xc4\x14\x54\xcf\xaa\x05\xf4\x71\xa3\x82\xf6\x9b\xa8\x96\x91"
    "\x9f\xb4\x5b\x7c\x04\x3b\x0d\x51\x47\xf7\x8e\x21\x85\xde\x98\x8b\x8e\x52\xfe\x7c\x54\x32\x7a\x2f\xda\xfa\x6b\x3a\x9a\x1c\xc7\x96"
    "\x91\xbb\x20\x77\xb8\xd9\x36\x40\xe1\x65\x87\x0e\x12\x05\xcb\x09\xc4\x97\xc5\xde\x93\x2c\x76\x30\xa8\xf4\x3e\x7f\xf0\x04\x0a\xa1"
    "\xb6\x05\x35\xbf\x22\x55\xf0\xd0\xea\xd8\x28\x1a\x94\x38\xfe\x2a\x37\xcf\x44\x79\xf2\xf8\x55\xe4\x7a\xd5\x87\x1d\xab\xee\xa5\x80"
    "\xed\x5f\x8f\xde\xd4\x96\xcc\x4d\x03\x9a\x0a\xd4\x32\x87\xe0\xd3\x28\xa8\x1c\xfb\x2d\x29\xcc\x67\x11\x59\xe1\xd0\xfe\xce\x37\x68"
    "\x62\xed\x59\xb4\xfa\xb5\xa6\x44\x01\x2b\x53\x8c\x91\x5e\x29\xea\x63\x9a\xaa\x66\xd0\xff\x12\xc7\x03\x40\x6e\x6d\x56\xac\x4e\xa5"
    "\xfb\x26\x35\xf9\x81\x2b\x0d\x8f\xec\x1c\xdc\xbe\x53\xea\xb2\x0a\xed\x0e\x62\xbf\x4f\xb0\x6e\x72\x05\x2b\x4e\x68\x40\xb9\x4c\xd6"
    "\xc3\x9b\x79\x79\x91\xee\xbb\x40\xf5\x8a\x41\x41\xdf\xeb\x52\xee\x6f\x2d\x35\x33\xa6\x22\xad\x19\x41\xc1\xf1\x48\x80\xaa\x53\xef"
    "\x4d\xd9\x3e\x77\x12\x2d\x1a\x4a\x37\x6f\x6a\xb6\x1e\xc5\x11\x88\x25\x6d\x95\x94\xcc\xf5\x72\x54\x97\x0c\x59\x25\x88\x8b\x0c\x08"
    "\xd8\x70\xa1\xad\x90\x2d\x6b\x5e\xbb\x18\x23\x64\x58\xef\x03\x89\x70\x26\xbe\xf6\xbb\xb8\xd5\x85\xbe\xab\x39\xb3\x80\x22\x24\xf0"
    "\x91\x02\x03\x01\x00\x01";

    
/**
 * The global static RSA object used for list validation 
 */
EVP_PKEY * cvmPublicKey;
 
/* This array will store all of the mutexes available to OpenSSL. */ 
static MUTEX_TYPE *mutex_buf= NULL;
 
/**
 * Locking function helper for OpenSSL
 */
static void locking_function(int mode, int n, const char * file, int line) {
  if (mode & CRYPTO_LOCK)
    MUTEX_LOCK(mutex_buf[n]);
  else
    MUTEX_UNLOCK(mutex_buf[n]);
}
 
/**
 * ID function helper for OpenSSL
 */
static unsigned long id_function(void) {
  return ((unsigned long)THREAD_ID);
}
 
/**
 * Initialize cryptographic library
 */
int cryptoInitialize(void) {
    int i;
 
    // Enable multi-threading
    mutex_buf = (MUTEX_TYPE*) malloc(CRYPTO_num_locks(  ) * sizeof(MUTEX_TYPE));
    if (!mutex_buf) {
        CVMWA_LOG( "Crypto", "Unable to allocate MUTEX!" );
        return 0;
    }
    for (i = 0;  i < CRYPTO_num_locks(  );  i++)
    MUTEX_SETUP(mutex_buf[i]);
    CRYPTO_set_id_callback(id_function);
    CRYPTO_set_locking_callback(locking_function);
  
    // Seed the OpenSSL's PRAND functions
    #ifdef _WIN32
    RAND_screen();
    #else
    RAND_load_file("/dev/urandom", 1024);
    #endif
    
    // Load key into memory
    cvmPublicKey = d2i_PUBKEY( NULL, &CVMWAPI_PUBKEY_DER, CVMWAPI_PUBKEY_DER_SIZE );
    if (cvmPublicKey == NULL) {
        CVMWA_LOG( "Crypto", "Unable to load public key!" );
        return 0;
    }
     
    // Init completed
    return 1;
}
 
/**
 * Cleanup cryptographic library
 */
int cryptoCleanup(void) {
    int i;
 
    // Cleanup multi-threading support
    if (!mutex_buf) return 0;
    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);
    for (i = 0;  i < CRYPTO_num_locks(  );  i++)
    MUTEX_CLEANUP(mutex_buf[i]);
    free(mutex_buf);
    mutex_buf = NULL;
    
    // Free the public key
    EVP_PKEY_free( cvmPublicKey );
    return 1;
    
}


/**
 * Initialize the cryptographic subsystem
 */
CVMWebCrypto::CVMWebCrypto () {

    // Defaults to invalid
    valid = false;
    
    // Reset timers
    lastUpdateTimestamp = 0;
    keystoreTimestamp = 0;
        
}

/**
 * Cleanup
 */
CVMWebCrypto::~CVMWebCrypto () {
}

/**
 * Validate the signature of the given data file, using the contents
 * of the given signature file.
 */
bool validateSignature( std::string dataFile, std::string sigFile ) {

    std::string keys;
    std::string sigData;
    
    // Open files
    std::ifstream iData( dataFile.c_str(), ifstream::binary  );
    std::ifstream iSig( sigFile.c_str(), ifstream::binary );
    if (iData.fail()) return false;
    if (iSig.fail()) return false;
    
    // Allocate buffers
    iData.seekg(0, std::ios::end);   
    keys.reserve(iData.tellg());
    iData.seekg(0, std::ios::beg);
    iSig.seekg(0, std::ios::end);   
    sigData.reserve(iSig.tellg());
    iSig.seekg(0, std::ios::beg);

    // Read contents
    keys.assign((std::istreambuf_iterator<char>(iData)),
            std::istreambuf_iterator<char>());
    sigData.assign((std::istreambuf_iterator<char>(iSig)),
            std::istreambuf_iterator<char>());

    // Close files
    iSig.close();
    iData.close();

    CVMWA_LOG( "Crypto", "Verifying store signature" );
    //hexDump( "Store data", (void *)keys.c_str(), keys.length() );
    //hexDump( "Store signature", (void *) sigData.c_str(), sigData.length() );

    // Verify signature
    EVP_MD_CTX ctx;
    EVP_VerifyInit( &ctx, EVP_sha512());
    EVP_VerifyUpdate( &ctx, keys.c_str(), keys.length() );
    int ans = EVP_VerifyFinal( &ctx, (unsigned char *) sigData.c_str(), sigData.length(), cvmPublicKey );
    if (ans != 1) return false;
    
    // Looks good
    return true;
}

/**
 * Download the updated version of the authorized keystore
 */
int CVMWebCrypto::updateAuthorizedKeystore( DownloadProviderPtr downloadProvider ) {
    bool needsReload = true;
    time_t currTime;
    time( &currTime );

    // Don't do this frequently
    if (valid && (lastUpdateTimestamp - currTime < CRYPTO_FREQUENT_THRESSHOLD)) {
        CVMWA_LOG("Crypto", "Last call to updateAuthorizedKeystore was suciffiently recent.");
        return 0;
    }
    valid = false;

    // Check if it has been some time since we last checked the keystore
    std::string localKeystore = config.getPath("domainkeys.conf");
    std::string localKeystoreSig = config.getPath("domainkeys.dat");
    if ( file_exists(localKeystore) && file_exists(localKeystoreSig) ) {
        
        // Check for external modifications
        time_t storeTime = config.getLastModified("domainkeys.lst");

        // Check if we are still within the  validity period
        if ((currTime - storeTime) < CRYPTO_STORE_VALIDITY) {

            // We are still within it's validity time
            CVMWA_LOG( "Crypto", "Store still valid" );

            // Check if the file was changed externally
            if ((keystoreTimestamp != 0) && (storeTime != keystoreTimestamp)) {

                // Timestamp changed. Check if signature is still valid
                if (validateSignature( localKeystore, localKeystoreSig )) {

                    // Signature is still valid. No reload is needed
                    CVMWA_LOG( "Crypto", "Store timestamp changed but is still valid" );
                    needsReload = false;

                }

            } else {

                // No need to reload anything. We are good and no
                // modifications occured externally
                needsReload = false;

            }

        }

    }
    
    // If we need reload, do it now
    if (needsReload) {
        // Download the authorized keystore
        CVMWA_LOG( "Crypto", "Downloading updated keystore" );
        int res = downloadProvider->downloadFile( CRYPTO_URL_STORE, localKeystore );
        if ( res != HVE_OK ) return res;

        // Download the keystore signature
        CVMWA_LOG( "Crypto", "Downloading store signature" );
        res = downloadProvider->downloadFile( CRYPTO_URL_SIGNATURE, localKeystoreSig );
        if ( res != HVE_OK ) return res;
    
        // Validate files
        CVMWA_LOG( "Crypto", "Validating signature" );
        if (!validateSignature( localKeystore, localKeystoreSig ))
            return HVE_NOT_VALIDATED;
    }
    
    // Everything looks good, read the key map
    CVMWA_LOG( "Crypto", "Loading keystore map" );
    config.loadMap( "domainkeys", &domainKeys );

    // Update the timestamp of the keystore
    keystoreTimestamp = config.getLastModified("domainkeys.lst");
    
    // We are ready!
    valid = true;
    return 0;
    
}

/**
 * Use the domain's key to decrypt the given data
 */
 int CVMWebCrypto::validateDomainData ( std::string domain, std::string signature, const unsigned char * data, const int dataLen ) {

    // Check if domain does not exist
    if (!isDomainValid(domain)) {
        CVMWA_LOG("Debug", "Domain " << domain << " not found in the local keystore");
        return HVE_NOT_FOUND;
    }

    // Load public key of the domain
    std::string domainData = base64_decode( domainKeys[domain] );
    const unsigned char * pData = ( const unsigned char * ) domainData.c_str();
    EVP_PKEY * domainKey = d2i_PUBKEY( NULL, &pData, domainData.length() );
    if (domainKey == NULL) {
        CVMWA_LOG( "Crypto", "Unable to load domain public key!" );
        return HVE_EXTERNAL_ERROR;
    }
    
    // Decode the base64 signature
    std::string sigData = base64_decode( signature );

    // Validate signature
    CVMWA_LOG( "Crypto", "Validating signature: '" << signature << "' of domain '" << domain << "'" );
    //hexDump( "Validating domain data",  (void *)data, dataLen );
    EVP_MD_CTX ctx;
    EVP_MD_CTX_init(&ctx);
    EVP_VerifyInit( &ctx, EVP_sha512());
    EVP_VerifyUpdate( &ctx, data, dataLen );
    int ans = EVP_VerifyFinal( &ctx, (unsigned char *) sigData.c_str(), sigData.length(), domainKey );
    CVMWA_LOG( "Crypto", "EVP_VerifyFinal = " << ans );
    if (ans != 1) return HVE_NOT_VALIDATED;
    
    // Free the key
    EVP_PKEY_free( domainKey );
    return HVE_OK;
    
}

/**
 * Check if the domain is validated
 */
bool CVMWebCrypto::isDomainValid ( std::string domain ) {
    return (domainKeys.find(domain) != domainKeys.end());
}

/**
 * Generate a unique, cryptographically strong salt
 */
std::string CVMWebCrypto::generateSalt() {
    const unsigned char SALT_SIZE = 64;
    const char saltChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-";
    unsigned char randValue[SALT_SIZE];
    std::string saltData = "";
    
    // Generate random bytes
    RAND_bytes( randValue, SALT_SIZE );
    
    // Map random value to string
    for (int i=0; i<SALT_SIZE; i++) {
        saltData += saltChars[ randValue[i] % 0x3F ];
    }
    return saltData;
}

/**
 * Validate if the given set of data is validated by the variables
 */
int CVMWebCrypto::signatureValidate( std::string& domain, std::string& salt, FB::VariantMap& data ) {
    
    /* Populate checksum buffer */
    stringstream checksumBuf;
    string dataSignature = "";
    for (FB::VariantMap::iterator it=data.begin(); it!=data.end(); ++it) {
        string key = (*it).first;
        FB::variant value = (*it).second;
        toLowerCase( key );
        if (key.compare("signature") != 0) {
            if (value.is_of_type<string>()) {
                checksumBuf << key << "=" << urlEncode( value.cast<string>() ) << "\n";
            } else if (value.is_of_type<int>()) {
                checksumBuf <<key << "=" << value.cast<int>() << "\n";
            } else if (value.is_of_type<long>()) {
                checksumBuf << key << "="  << value.cast<long>() << "\n";
            }
        } else {
            // Store signature and skip it from the checksum
            dataSignature = value.cast<string>();
        }
    }
    
    /* Append salt to the data */
    checksumBuf << salt;
    
    /* Check for invalid signature */
    if (dataSignature.empty()) return HVE_USAGE_ERROR;

    /* Extract pointers */
    string strHash = checksumBuf.str();

    /* Validate signature */
    return validateDomainData( domain, dataSignature, (const unsigned char * ) strHash.c_str(), strHash.length() );
    
}
