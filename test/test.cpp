
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>

#include "Utilities.h"
#include "Virtualbox.h"
#include "Hypervisor.h"
#include "ThinIPC.h"
#include "DaemonCtl.h"

#include "DownloadProvider.h"

#include <boost/thread/mutex.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include <openssl/rand.h>

using namespace std;

boost::interprocess::interprocess_mutex *   m_ipcMutex = NULL;
boost::interprocess::shared_memory_object * m_shmem = NULL;
boost::interprocess::mapped_region *        m_shregion = NULL;

int updateSharedMemoryID( std::string uuid ) {
    try {
        // Release previous objects
        if (m_ipcMutex != NULL) delete m_ipcMutex;
        if (m_shmem != NULL) delete m_shmem;
        if (m_shregion != NULL) delete m_shregion;

        // Calculate a shared memory name
        string shmemName = "cvmwebExecShmem" + uuid;

        // Create a shared memory object
        m_shmem = new boost::interprocess::shared_memory_object(
            boost::interprocess::open_or_create,
            shmemName.c_str(),
            boost::interprocess::read_write);
        if (m_shmem == NULL) {
            CVMWA_LOG( "Error", "Unable to allocate shared memory object with UUID " << uuid );
            return HVE_EXTERNAL_ERROR;
        }

        // Allocate memory
        m_shmem->truncate( sizeof( m_ipcMutex ) );
        m_shregion = new boost::interprocess::mapped_region( 
            *m_shmem, 
            boost::interprocess::read_write);
        
        void * ptr  = m_shregion->get_address();
        cout.write((const char *)ptr,8);
        
        m_ipcMutex = new(ptr)boost::interprocess::interprocess_mutex;
        if (m_ipcMutex == NULL) {
            CVMWA_LOG( "Error", "Unable to allocate shared memory mutex" );
            return HVE_EXTERNAL_ERROR;
        }
    } catch (boost::interprocess::interprocess_exception &e) {
        CVMWA_LOG( "Exception", "Interprocess operation exception: " << e.what() );
        return HVE_EXTERNAL_ERROR;
    }

    return HVE_OK;
}

void exec_thread( std::string uuid ) {

    // Calculate named mutex name
    string shMutexName = uuid;
    
    // Create a shared named mutex
    boost::interprocess::named_mutex::remove( shMutexName.c_str() );
    boost::interprocess::named_mutex mutex(
        boost::interprocess::open_or_create, 
        shMutexName.c_str());
        
    {
        /* Use only one exec per session, using inteprocess mutexes */
        boost::interprocess::scoped_lock< boost::interprocess::named_mutex > lock( mutex );
        
        /* Emulate exec */
        cout << "** Running***" << endl;
        sleepMs(5000);
        
    }
}

int main( int argc, char ** argv ) {
    //cout << "Updating SHMEM=" << updateSharedMemoryID("9d9a7f71-9cdc-4b54-a804-cd1e8688922d") << endl;
    exec_thread("123456789012345678901234567890");
    return 0;
}
