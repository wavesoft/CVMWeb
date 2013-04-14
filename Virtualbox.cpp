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

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <algorithm>

#include "Hypervisor.h"
#include "Virtualbox.h"
using namespace std;

/** =========================================== **\
                   Tool Functions
\** =========================================== **/

/**
 * Dump a map
 */
void mapDump(map<string, string> m) {
    for (std::map<string, string>::iterator it=m.begin(); it!=m.end(); ++it) {
        string k = (*it).first;
        string v = (*it).second;
        cout << k << " => " << v << "\n";
    }
}


/**
 * Split the given line into a key and value using the delimited provided
 */
int getKV( string line, string * key, string * value, char delim, int offset ) {
    int a = line.find( delim, offset );
    *key = line.substr(offset, a-offset);
    int b = a+1;
    while ( ((line[b] == ' ') || (line[b] == '\t')) && (b<line.length())) b++;
    *value = line.substr(b, string::npos);
    return a;
}

/**
 * Tokenize a key-value like output from VBoxManage into an easy-to-use hashmap
 */
map<string, string> tokenize( vector<string> * lines, char delim ) {
    map<string, string> ans;
    string line, key, value;
    for (vector<string>::iterator i = lines->begin(); i != lines->end(); i++) {
        line = *i;
        if (line.find(delim) != string::npos) {
            getKV(line, &key, &value, delim, 0);
            if (ans.find(key) == ans.end()) {
                ans[key] = value;
            }
        }
    }
    return ans;
};

/**
 * Tokenize a list of repeating key-value groups
 */
vector< map<string, string> > tokenizeList( vector<string> * lines, char delim ) {
    vector< map<string, string> > ans;
    map<string, string> row;
    string line, key, value;
    for (vector<string>::iterator i = lines->begin(); i != lines->end(); i++) {
        line = *i;
        if (line.find(delim) != string::npos) {
            getKV(line, &key, &value, delim, 0);
            row[key] = value;
        } else if (line.length() == 0) { // Empty line -> List delimiter
            ans.push_back(row);
            row.clear();
        }
    }
    return ans;
};

/** =========================================== **\
            VBoxSession Implementation
\** =========================================== **/

/**
 * Execute command and log debug message
 */
int VBoxSession::wrapExec( std::string cmd, std::vector<std::string> * stdout ) {
    ostringstream oss;
    string line;
    int ans;
    
    /* Debug log command */
    if (this->onDebug!=NULL) (this->onDebug)("Executing '"+cmd+"'", this->cbObject);
    
    /* Run command */
    ans = this->host->exec( cmd, stdout );
    
    /* Debug log response */
    if (this->onDebug!=NULL) {
        if (stdout != NULL) {
            for (vector<string>::iterator i = stdout->begin(); i != stdout->end(); i++) {
                line = *i;
                (this->onDebug)("Line: "+line, this->cbObject);
            }
        } else {
            (this->onDebug)("(Output ignored)", this->cbObject);
        }
        oss << "return = " << ans;
        (this->onDebug)(oss.str(), this->cbObject);
    }
    return ans;
};

/**
 * Open new session
 */
int VBoxSession::open( int cpus, int memory, int disk, std::string cvmVersion ) { 
    ostringstream args;
    vector<string> lines;
    map<string, string> toks;
    string stdout, uuid, ifHO, vmIso, kk, kv;
    char * tmp;
    int ans;
    bool needsUpdate;

    /* Validate state */
    if ((this->state != STATE_CLOSED) && (this->state != STATE_ERROR)) return HVE_INVALID_STATE;
    this->state = STATE_OPPENING;
    
    /* Update session */
    this->cpus = cpus;
    this->memory = memory;
    this->executionCap = 100;
        
    /* (1) Create slot */
    if (this->onProgress!=NULL) (this->onProgress)(1, 11, "Allocating VM slot", this->cbObject);
    ans = this->getMachineUUID( this->name, &uuid );
    if (ans != 0) {
        this->state = STATE_ERROR;
        return ans;
    }
    
    /* Detect the host-only adapter */
    if (this->onProgress!=NULL) (this->onProgress)(2, 11, "Setting up local network", this->cbObject);
    ifHO = this->getHostOnlyAdapter();
    if (ifHO.empty()) {
        this->state = STATE_ERROR;
        return HVE_CREATE_ERROR;
    }

    /* (2) Set parameters */
    args.str("");
    args << "modifyvm "
        << uuid
        << " --cpus "         << cpus
        << " --memory "       << memory
        << " --vram "         << "32"
        << " --acpi "         << "on"
        << " --ioapic "       << "on"
        << " --boot1 "        << "dvd" << " --boot2 " << "none" << " --boot3 " << "none" << " --boot4 " << "none"
        << " --nic1 "         << "nat"
        << " --natdnsproxy1 " << "on"
        << " --nic2 "         << "hostonly" << " --hostonlyadapter2 \"" << ifHO << "\"";
    
    if (this->onProgress!=NULL) (this->onProgress)(3, 11, "Setting up VM", this->cbObject);
    ans = this->wrapExec(args.str(), NULL);
    cout << "Modify VM=" << ans << "\n";
    if (ans != 0) {
        this->state = STATE_ERROR;
        return HVE_MODIFY_ERROR;
    }

    /* Fetch information to validate disks */
    if (this->onProgress!=NULL) (this->onProgress)(4, 11, "Fetching machine info", this->cbObject);
    map<string, string> machineInfo = this->getMachineInfo();

    /* Check for scratch disk */
    if (machineInfo.find("SATA (0, 0)") == machineInfo.end()) {
        
        /* Create a hard disk for this VM */
        tmp = tmpnam(NULL);
        string vmDisk = tmp;
        vmDisk += ".vdi";
    
        /* (4) Create disk */
        args.str("");
        args << "createhd "
            << " --filename "   << "\"" << vmDisk << "\""
            << " --size "       << disk;
    
        if (this->onProgress!=NULL) (this->onProgress)(5, 11, "Creating scratch disk", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        cout << "Create HD=" << ans << "\n";
        if (ans != 0) {
            this->state = STATE_ERROR;
            return HVE_MODIFY_ERROR;
        }
        
        /* (4.b) VirtualBox < 4.0 needs openmedium */
        if (this->host->verMajor < 4) {
            args.str("");
            args << "openmedium "
                << " disk "   << "\"" << vmDisk << "\"";
            if (this->onProgress!=NULL) (this->onProgress)(6, 11, "Importing disk", this->cbObject);
            ans = this->wrapExec(args.str(), NULL);
            cout << "Close medium=" << ans << "\n";
            if (ans != 0) {
                this->state = STATE_ERROR;
                return HVE_MODIFY_ERROR;
            }
        }

        /* (5) Attach disk to the SATA controller */
        args.str("");
        args << "storageattach "
            << uuid
            << " --storagectl " << "SATA"
            << " --port "       << "0"
            << " --device "     << "0"
            << " --type "       << "hdd"
            << " --setuuid "    << "\"\"" 
            << " --medium "     << "\"" << vmDisk << "\"";

        if (this->onProgress!=NULL) (this->onProgress)(7, 11, "Attaching hard disk", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        cout << "Storage Attach=" << ans << "\n";
        if (ans != 0) {
            this->state = STATE_ERROR;
            return HVE_MODIFY_ERROR;
        }
        
    }
    
    /* Check if the CernVM Version the machine is using is the one we need */
    needsUpdate = true;
    if (machineInfo.find("IDE (1, 0)") != machineInfo.end()) {
        
        /* Get the filename of the iso */
        getKV( machineInfo["IDE (1, 0)"], &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);
        
        /* Get the filename of the given version */
        this->host->cernVMCached( cvmVersion, &kv );
        
        /* If they are the same, we are lucky */
        if (kv.compare(kk) == 0) {
            cout << "Same versions (" << kv << ")\n";
            needsUpdate = false;
        } else {
            cout << "CernVM iso is different : " << kk << " / " << kv << "\n";
            
            /* Unmount previount iso */
            args.str("");
            args << "storageattach "
                << uuid
                << " --storagectl " << "IDE"
                << " --port "       << "1"
                << " --device "     << "0"
                << " --medium "     << "none";

            if (this->onProgress!=NULL) (this->onProgress)(8, 11, "Detachining previous CernVM ISO", this->cbObject);
            ans = this->wrapExec(args.str(), NULL);
            cout << "Detaching ISO=" << ans << "\n";
            if (ans != 0) {
                this->state = STATE_ERROR;
                return HVE_MODIFY_ERROR;
            }
        }
    
    }
    
    /* Check if we need to update the CD-ROM attaching business */
    if (needsUpdate) {
        
        /* Download CernVM */
        if (this->onProgress!=NULL) (this->onProgress)(9, 11, "Downloading CernVM", this->cbObject);
        if (this->host->cernVMDownload( cvmVersion, &vmIso ) != 0) {
            this->state = STATE_ERROR;
            return HVE_IO_ERROR;
        }

        /* (6) Attach CD-ROM to the IDE controller */
        args.str("");
        args << "storageattach "
            << uuid
            << " --storagectl " << "IDE"
            << " --port "       << "1"
            << " --device "     << "0"
            << " --type "       << "dvddrive"
            << " --setuuid "    << "\"\"" 
            << " --medium "     << "\"" << vmIso << "\"";
    
        if (this->onProgress!=NULL) (this->onProgress)(10, 11, "Attaching CD-ROM", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        cout << "Storage Attach (CernVM)=" << ans << "\n";
        if (ans != 0) {
            this->state = STATE_ERROR;
            return HVE_MODIFY_ERROR;
        }        
    }
    
    /* Store web-secret on the guest properties */
    this->setProperty("web-secret", this->key);

    /* Last callbacks */
    if (this->onProgress!=NULL) (this->onProgress)(11, 11, "Completed", this->cbObject);
    if (this->onOpen!=NULL) (this->onOpen)(this->cbObject);
    this->state = STATE_OPEN;
    this->uuid = uuid;
    
    return HVE_OK;

}

/**
 * Start VM with the given
 */
int VBoxSession::start( std::string userData ) { 
    string vmContextISO, kk, kv;
    ostringstream args;
    int ans;

    /* Validate state */
    if (this->state != STATE_OPEN) return HVE_INVALID_STATE;
    this->state = STATE_STARTING;

    /* Fetch information to validate disks */
    map<string, string> machineInfo = this->getMachineInfo();
    
    /* Detach & Delete previous context ISO */
    if (machineInfo.find("IDE (1, 1)") != machineInfo.end()) {
        
        /* Get the filename of the iso */
        getKV( machineInfo["IDE (1, 1)"], &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);

        cout << "Detaching " << kk << "\n";

        /* Detach iso */
        args.str("");
        args << "storageattach "
            << uuid
            << " --storagectl " << "IDE"
            << " --port "       << "1"
            << " --device "     << "1"
            << " --medium "     << "none";

        if (this->onProgress!=NULL) (this->onProgress)(1, 7, "Detaching contextualization CD-ROM", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        cout << "Storage Attach (context)=" << ans << "\n";
        if (ans != 0) {
            this->state = STATE_OPEN;
            return HVE_MODIFY_ERROR;
        }
        
        /* Unregister/delete iso */
        args.str("");
        args << "closemedium dvd "
            << "\"" << kk << "\"";

        if (this->onProgress!=NULL) (this->onProgress)(2, 7, "Closing contextualization CD-ROM", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        cout << "Closemedium (context)=" << ans << "\n";
        if (ans != 0) {
            this->state = STATE_OPEN;
            return HVE_MODIFY_ERROR;
        }
        
        /* Delete actual file */
        if (this->onProgress!=NULL) (this->onProgress)(3, 7, "Removing contextualization CD-ROM", this->cbObject);
        remove( kk.c_str() );
        
    }
    
    /* Create Context ISO */
    if (this->onProgress!=NULL) (this->onProgress)(4, 7, "Building contextualization CD-ROM", this->cbObject);
    if (this->host->buildContextISO( userData, &vmContextISO ) != 0) 
        return HVE_CREATE_ERROR;

    /* Attach CD-ROM to the IDE controller */
    args.str("");
    args << "storageattach "
        << uuid
        << " --storagectl " << "IDE"
        << " --port "       << "1"
        << " --device "     << "1"
        << " --type "       << "dvddrive"
        << " --setuuid "    << "\"\"" 
        << " --medium "     << "\"" << vmContextISO << "\"";

    if (this->onProgress!=NULL) (this->onProgress)(5, 7, "Attaching contextualization CD-ROM", this->cbObject);
    ans = this->wrapExec(args.str(), NULL);
    cout << "StorageAttach (context)=" << ans << "\n";
    if (ans != 0) {
        this->state = STATE_OPEN;
        return HVE_MODIFY_ERROR;
    }
    
    /* Start VM */
    if (this->onProgress!=NULL) (this->onProgress)(6, 7, "Starting VM", this->cbObject);
    ans = this->wrapExec("startvm \"" + this->uuid + "\" --type headless", NULL);
    cout << "Start VM=" << ans << "\n";
    if (ans != 0) {
        this->state = STATE_OPEN;
        return HVE_MODIFY_ERROR;
    }
    
    /* Update parameters */
    if (this->onProgress!=NULL) (this->onProgress)(7, 7, "Completed", this->cbObject);
    this->executionCap = 100;
    this->state = STATE_STARTED;
    return 0;
}

/**
 * Close VM
 */
int VBoxSession::close() { 
    string kk, kv;
    ostringstream args;
    int ans;

    /* Validate state */
    if ((this->state != STATE_OPEN) && (this->state != STATE_STARTED) && (this->state != STATE_ERROR)) return HVE_INVALID_STATE;

    /* Stop the VM if it's running (we don't care about the warnings) */
    if (this->onProgress!=NULL) (this->onProgress)(1, 9, "Shutting down the VM", this->cbObject);
    this->controlVM( "poweroff");
    
    /* Unmount, release and delete media */
    map<string, string> machineInfo = this->getMachineInfo();
    
    /* Detach & Delete context ISO */
    if (machineInfo.find("IDE (1, 1)") != machineInfo.end()) {
        
        /* Get the filename of the iso */
        getKV( machineInfo["IDE (1, 1)"], &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);

        cout << "Detaching " << kk << "\n";

        /* Detach iso */
        args.str("");
        args << "storageattach "
            << uuid
            << " --storagectl " << "IDE"
            << " --port "       << "1"
            << " --device "     << "1"
            << " --medium "     << "none";

        if (this->onProgress!=NULL) (this->onProgress)(2, 9, "Detaching contextualization CD-ROM", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        cout << "Storage Attach (context)=" << ans << "\n";
        if (ans != 0) return HVE_MODIFY_ERROR;
        
        /* Unregister/delete iso */
        args.str("");
        args << "closemedium dvd "
            << "\"" << kk << "\"";

        if (this->onProgress!=NULL) (this->onProgress)(3, 9, "Closing contextualization CD-ROM", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        cout << "Closemedium (context)=" << ans << "\n";
        if (ans != 0) return HVE_MODIFY_ERROR;
        
        /* Delete actual file */
        if (this->onProgress!=NULL) (this->onProgress)(4, 9, "Deleting contextualization CD-ROM", this->cbObject);
        remove( kk.c_str() );
        
    }
    
    /* Detach & Delete Disk */
    if (machineInfo.find("SATA (0, 0)") != machineInfo.end()) {
        
        /* Get the filename of the iso */
        getKV( machineInfo["SATA (0, 0)"], &kk, &kv, '(', 0 );
        kk = kk.substr(0, kk.length()-1);

        cout << "Detaching " << kk << "\n";

        /* Detach iso */
        args.str("");
        args << "storageattach "
            << uuid
            << " --storagectl " << "SATA"
            << " --port "       << "0"
            << " --device "     << "0"
            << " --medium "     << "none";

        if (this->onProgress!=NULL) (this->onProgress)(5, 9, "Detaching data disk", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        cout << "Storage Attach (context)=" << ans << "\n";
        if (ans != 0) return HVE_MODIFY_ERROR;
        
        /* Unregister/delete iso */
        args.str("");
        args << "closemedium disk "
            << "\"" << kk << "\"";

        if (this->onProgress!=NULL) (this->onProgress)(6, 9, "Closing data disk medium", this->cbObject);
        ans = this->wrapExec(args.str(), NULL);
        cout << "Closemedium (disk)=" << ans << "\n";
        if (ans != 0) return HVE_MODIFY_ERROR;
        
        /* Delete actual file */
        if (this->onProgress!=NULL) (this->onProgress)(7, 9, "Removing data disk", this->cbObject);
        remove( kk.c_str() );
        
    }    
    
    /* Unregister and delete VM */
    if (this->onProgress!=NULL) (this->onProgress)(8, 9, "Deleting VM", this->cbObject);
    ans = this->wrapExec("unregistervm \"" + this->uuid + "\" --delete", NULL);
    cout << "Unregister VM=" << ans << "\n";
    if (ans != 0) {
        this->state = STATE_ERROR;
        return HVE_CONTROL_ERROR;
    }
    
    /* OK */
    if (this->onProgress!=NULL) (this->onProgress)(9, 9, "Completed", this->cbObject);
    this->state = STATE_CLOSED;
    return HVE_OK;
}

/**
 * Create or fetch the UUID of the VM with the given name
 */
int VBoxSession::getMachineUUID( std::string mname, std::string * ans_uuid ) {
    ostringstream args;
    vector<string> lines;
    map<string, string> toks;
    string secret, name, uuid;
    
    /* List the running VMs in the system */
    int ans = this->wrapExec("list vms", &lines);
    if (ans != 0) {
        return HVE_QUERY_ERROR;
    }
    
    /* Tokenize */
    toks = tokenize( &lines, '{' );
    for (std::map<string, string>::iterator it=toks.begin(); it!=toks.end(); ++it) {
        name = (*it).first;
        uuid = (*it).second;
        name = name.substr(1, name.length()-3);
        uuid = uuid.substr(0, uuid.length()-1);
        
        /* Compare name */
        if (name.compare(mname) == 0)  {
            *ans_uuid = uuid;
            return 0;
        }
    }
    
    /* Not found, create VM */
    args.str("");
    args << "createvm"
        << " --name \"" << mname << "\""
        << " --ostype Linux26_64"
        << " --register";
    
    ans = this->wrapExec(args.str(), &lines);
    if (ans != 0) return HVE_CREATE_ERROR;
    
    /* Parse output */
    toks = tokenize( &lines, ':' );
    uuid = toks["UUID"];
    
    /* (3a) Attach an IDE controller */
    args.str("");
    args << "storagectl "
        << uuid
        << " --name "       << "IDE"
        << " --add "        << "ide";
    
    ans = this->wrapExec(args.str(), NULL);
    if (ans != 0) return HVE_MODIFY_ERROR;

    /* (3b) Attach a SATA controller */
    args.str("");
    args << "storagectl "
        << uuid
        << " --name "       << "SATA"
        << " --add "        << "sata";
    
    ans = this->wrapExec(args.str(), NULL);
    if (ans != 0) return HVE_MODIFY_ERROR;
    
    /* OK */
    *ans_uuid = uuid;
    return 0;
}

/**
 * Resume VM
 */
int VBoxSession::resume() {
    int ans;
    
    /* Validate state */
    if (this->state != STATE_PAUSED) return HVE_INVALID_STATE;
    
    /* Resume VM */
    ans = this->controlVM("resume");
    this->state = STATE_STARTED;
    return ans;
}

/**
 * Pause VM
 */
int VBoxSession::pause() {
    int ans; 
    
    /* Validate state */
    if (this->state != STATE_STARTED) return HVE_INVALID_STATE;

    /* Pause VM */
    ans = this->controlVM("pause");
    this->state = STATE_PAUSED;
    return ans;
}

/**
 * Reset VM
 */
int VBoxSession::reset() {
    
    /* Validate state */
    if (this->state != STATE_STARTED) return HVE_INVALID_STATE;
    
    /* Reset VM */
    return this->controlVM( "reset" );
}

/**
 * Stop VM
 */
int VBoxSession::stop() {
    int ans;
    
    /* Validate state */
    if (this->state != STATE_STARTED) return HVE_INVALID_STATE;
    
    /* Stop VM */
    ans = this->controlVM( "poweroff" );
    this->state = STATE_OPEN;
    return ans;
    
}

/**
 * Set execution cap
 */
int VBoxSession::setExecutionCap(int cap) {
    
    /* Validate state */
    if ((this->state != STATE_STARTED) && (this->state != STATE_OPEN)) return HVE_INVALID_STATE;

    ostringstream os;
    os << "cpuexecutioncap " << cap;
    this->executionCap = cap;
    this->controlVM( os.str());
    return 0;
}

/**
 * Ensure the existance and return the name of the host-only adapter in the system
 */
std::string VBoxSession::getHostOnlyAdapter() {

    vector<string> lines;
    vector< map<string, string> > ifs;
    string ifName;
    
    /* Check if we already have host-only interfaces */
    int ans = this->wrapExec("list hostonlyifs", &lines);
    if (ans != 0) return "";
    
    /* Check if there is really nothing */
    if (lines.size() == 0) {
        ans = this->wrapExec("hostonlyif create", NULL);
        if (ans != 0) return "";
    
        /* Repeat check */
        ans = this->wrapExec("list hostonlyifs", &lines);
        if (ans != 0) return "";
        
        /* Still couldn't pick anything? Error! */
        if (lines.size() == 0) return "";
    }
    
    /* Process interfaces */
    ifs = tokenizeList( &lines, ':' );
    for (vector< map<string, string> >::iterator i = ifs.begin(); i != ifs.end(); i++) {
        map<string, string> iface = *i;
        ifName = iface["Name"];
        /*
        if (iface["DHCP"].compare("Disabled")) {
            ans = this->wrapExec("VBoxManage dhcpserver modify --ifname "+ifName+" --enable", NULL);
            if (ans != 0) return NULL;
        }
        */
        break;
    }
    
    /* Got my interface */
    return ifName;
};

/**
 * Return a property from the VirtualBox guest
 */
std::string VBoxSession::getProperty( std::string name ) { 
    vector<string> lines;
    string value;
    
    /* Invoke property query */
    int ans = this->wrapExec("guestproperty get \""+this->uuid+"\" \""+name+"\"", &lines);
    if (ans != 0) return "";
    
    /* Process response */
    value = lines[0];
    if (value.substr(0,6).compare("Value:") == 0) {
        return value.substr(7);
    } else {
        return "";
    }
    
}

/**
 * Set a property to the VirtualBox guest
 */
int VBoxSession::setProperty( std::string name, std::string value ) { 
    vector<string> lines;
    
    /* Perform property update */
    int ans = this->wrapExec("guestproperty set \""+this->uuid+"\" \""+name+"\" \""+value+"\"", &lines);
    if (ans != 0) return HVE_QUERY_ERROR;
    return 0;
    
}

/**
 * Send a controlVM something
 */
int VBoxSession::controlVM( std::string how ) {
    int ans = this->wrapExec("controlvm \""+this->uuid+"\" "+how, NULL);
    if (ans != 0) return HVE_CONTROL_ERROR;
    return 0;
}

/**
 * Start the Virtual Machine
 */
int VBoxSession::startVM() {
    int ans = this->wrapExec("startvm \""+this->uuid+"\" --type headless", NULL);
    if (ans != 0) return HVE_CONTROL_ERROR;
    return 0;
}

/** 
 * Return virtual machine information
 */
map<string, string> VBoxSession::getMachineInfo() {
    vector<string> lines;
    map<string, string> dat;
    
    /* Perform property update */
    int ans = this->wrapExec("showvminfo \""+this->uuid+"\"", &lines);
    if (ans != 0) return dat;
    
    /* Tokenize response */
    return tokenize( &lines, ':' );
};

/** =========================================== **\
            Virtualbox Implementation
\** =========================================== **/

/** 
 * Return virtual machine information
 */
map<string, string> Virtualbox::getMachineInfo( std::string uuid ) {
    vector<string> lines;
    map<string, string> dat;
    
    /* Perform property update */
    int ans = this->exec("showvminfo \""+uuid+"\"", &lines);
    if (ans != 0) return dat;
    
    /* Tokenize response */
    return tokenize( &lines, ':' );
};


/**
 * Return a property from the VirtualBox guest
 */
std::string Virtualbox::getProperty( std::string uuid, std::string name ) {
    vector<string> lines;
    string value;
    
    /* Invoke property query */
    int ans = this->exec("guestproperty get \""+uuid+"\" \""+name+"\"", &lines);
    if (ans != 0) return "";
    
    /* Process response */
    value = lines[0];
    if (value.substr(0,6).compare("Value:") == 0) {
        return value.substr(7);
    } else {
        return "";
    }
    
}

/**
 * Return Virtualbox sessions instead of classic
 */
HVSession * Virtualbox::allocateSession( std::string name, std::string key ) {
    VBoxSession * sess = new VBoxSession();
    sess->name = name;
    sess->key = key;
    sess->host = this;
    sess->state = 0;
    sess->ip = "";
    sess->cpus = 1;
    sess->memory = 256;
    sess->executionCap = 100;
    sess->disk = 1024;
    sess->version = DEFAULT_CERNVM_VERSION;
    return sess;
}

/**
 * Load session state from VirtualBox
 */
int Virtualbox::loadSessions() {
    vector<string> lines;
    map<string, string> vms, diskinfo;
    string secret, kk, kv;
    
    /* List the running VMs in the system */
    int ans = this->exec("list vms", &lines);
    if (ans != 0) return HVE_QUERY_ERROR;
    
    /* Tokenize */
    vms = tokenize( &lines, '{' );
    this->sessions.clear();
    for (std::map<string, string>::iterator it=vms.begin(); it!=vms.end(); ++it) {
        string name = (*it).first;
        string uuid = (*it).second;
        name = name.substr(1, name.length()-3);
        uuid = uuid.substr(0, uuid.length()-1);
        
        /* Check if this VM has a secret web key. If yes, it's managed by the WebAPI */
        secret = this->getProperty( uuid, "web-secret" );
        if (!secret.empty()) {
            
            /* Create a populate session object */
            HVSession * session = this->allocateSession( name, secret );
            ((VBoxSession*)session)->uuid = uuid;
            
            /* Collect details */
            map<string, string> info = this->getMachineInfo( uuid );
            string state = info["State"];
            if (state.find("running") != string::npos) {
                session->state = STATE_STARTED;
            } else if (state.find("paused") != string::npos) {
                session->state = STATE_PAUSED;
            } else {
                session->state = STATE_OPEN;
            }
            session->cpus = ston<int>( info["Number of CPUs"] );
            
            /* Parse memory */
            string mem = info["Memory size"];
            mem = mem.substr(0, mem.length()-2);
            session->memory = ston<int>(mem);
            
            /* Parse CernVM Version from the ISO */
            session->version = DEFAULT_CERNVM_VERSION;
            if (info.find("IDE (1, 0)") != info.end()) {

                /* Get the filename of the iso */
                getKV( info["IDE (1, 0)"], &kk, &kv, '(', 0 );
                kk = kk.substr(0, kk.length()-1);
                
                /* Extract CernVM Version from file */
                session->version = this->cernVMVersion( kk );
                if (session->version.empty()) 
                    session->version = DEFAULT_CERNVM_VERSION;
            }
            
            /* Parse disk size */
            session->disk = 1024;
            if (info.find("SATA (0, 0)") != info.end()) {

                /* Get the filename of the iso */
                getKV( info["SATA (0, 0)"], &kk, &kv, '(', 0 );
                kk = kk.substr(0, kk.length()-1);
                
                /* Collect disk info */
                int ans = this->exec("showhdinfo "+kk, &lines);
                if (ans == 0) {
                
                    /* Tokenize data */
                    diskinfo = tokenize( &lines, ':' );
                    if (diskinfo.find("Logical size") != info.end()) {
                        kk = diskinfo["Logical size"];
                        kk = kk.substr(0, kk.length()-7); // Strip " MBytes"
                        session->disk = ston<int>(kk);
                    }
                    
                }
            }

            /* Register this session */
            cout << "Registering session name=" << session->name << ", key=" << session->key << ", uuid=" << ((VBoxSession*)session)->uuid << ", state=" << session->state << "\n";
            this->registerSession(session);

        }
    }

    return 0;
}
