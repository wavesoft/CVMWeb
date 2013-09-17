
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

#include <openssl/rand.h>

using namespace std;

void exec_thread( std::string uuid ) {
    cout << "[" << uuid << "] Started" << endl;
    
    NAMED_MUTEX_LOCK(uuid);
        
    /* Emulate exec */
    cout << "[" << uuid << "] *** Running ***" << endl;
    sleepMs(5000);

    NAMED_MUTEX_UNLOCK;

    cout << "[" << uuid << "] Completed" << endl;
}

int main( int argc, char ** argv ) {
    //cout << "Updating SHMEM=" << updateSharedMemoryID("9d9a7f71-9cdc-4b54-a804-cd1e8688922d") << endl;
    boost::thread t1(boost::bind(&exec_thread, "first"));
    boost::thread t2(boost::bind(&exec_thread, "second"));
    boost::thread t3(boost::bind(&exec_thread, "third"));
    boost::thread t4(boost::bind(&exec_thread, "first"));
    boost::thread t5(boost::bind(&exec_thread, "first"));
    boost::thread t6(boost::bind(&exec_thread, "second"));
    
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    return 0;
}
