
/**
 * Developer-friendly wrapper for the session object
 */
_NS_.WebAPISession = function( plugin_ref, daemon_ref, session_ref ) {
    _NS_.EventDispatcher.call(this); // Superclass constructor
    this.__plugin = plugin_ref;
    this.__daemon = daemon_ref;
    this.__session = session_ref;
    this.__valid = true;
    
    // Now that we have a session we can probe for daemon status
    this.__daemonStatus = false;
    this.__systemStatus = false;
    
    // Accessible function
    this.autoUpdate = _NS_.autoUpdate;
    
    // If we are activating debug logging, hook the 'debug' event
    // to the console log.
    if (_NS_.debugLogging) {
        this.__session.addEventListener('debug', function(log) {
            console.log("[Debug] " + log);
        });
    }
    
    // Local function to check for updates in the daemon
    this.__checkDaemonUpdates = function() {
        
        // Update daemon status
        var running = this.__daemon.isRunning;
            
        // Check daemon state change
        if (running != this.__daemonStatus) {
            this.__daemonStatus = running;
            this.__fire('daemonStateChange', running);
        }
        
        // Check system idle state change
        if (running) {
            
            // (We use the running check to ensure that no delays
            // are introduced becuse of IPC timeouts)
            var idle = this.__daemon.isSystemIdle;
            if (idle != this.__systemStatus) {
                this.__systemStatus = idle;
                this.__fire('systemStateChange', idle);
            }
            
        }
        
    };
    
    // Schedule update probing timer
    var __updateTimer = setInterval( (function(){
        
        // Update session status
        if (this.autoUpdate)
            this.__session.update();
        
        // Update daemon status
        this.__checkDaemonUpdates();
        
    }).bind(this), 5000);
    
    // Connect plugin properties with this object properties using getters/setters
    var u = undefined;
    Object.defineProperties(this, {
        "state"         :   {   get: function () { if (!this.__valid) return u; return this.__session.state;                 } },
        "stateName"     :   {   get: function () { if (!this.__valid) return u; return state_string(this.__session.state );  } },
        "ip"            :   {   get: function () { if (!this.__valid) return u; return this.__session.ip;                    } },
        "ram"           :   {   get: function () { if (!this.__valid) return u; return this.__session.ram;                   } },
        "disk"          :   {   get: function () { if (!this.__valid) return u; return this.__session.disk;                  } },
        "apiURL"        :   {   get: function () { if (!this.__valid) return u; return this.__session.apiURL;                } },
        "rdpURL"        :   {   get: function () { if (!this.__valid) return u; return this.__session.rdpURL;                } },
        "executionCap"  :   {   get: function () { if (!this.__valid) return u; return this.__session.executionCap;          }, 
                                set: function(v) { this.__session.setExecutionCap(v);           } },
        "daemonMinCap"  :   {   get: function () { if (!this.__valid) return u; return this.__session.daemonMinCap;          }, 
                                set: function(v) { this.__session.daemonMinCap =v;              } },
        "daemonMaxCap"  :   {   get: function () { if (!this.__valid) return u; return this.__session.daemonMaxCap;          }, 
                                set: function(v) { this.__session.daemonMaxCap =v;              } },
        "daemonControlled": {   get: function () { if (!this.__valid) return u; return this.__session.daemonControlled;      }, 
                                set: function(v) { 
                                    this.__session.daemonControlled=v;
                                    
                                    // Include a more frequent update, since we know it is going to be
                                    // started very soon... 
                                    setTimeout( (function(){ 
                                        this.__checkDaemonUpdates(); 
                                    }).bind(this), 500)
                                    
                                }                                                                 },

        /* Version/DiskURL switching */
        "version"       :     {   get: function () {
                                        if (!this.__valid) return u; 
                                        return ((this.__session.flags & HVF_DEPLOYMENT_HDD) == 0) ? this.__session.version : "";               
                                    } 
                              }, 
        "diskURL"       :     {   get: function () {
                                      if (!this.__valid) return u; 
                                      return ((this.__session.flags & HVF_DEPLOYMENT_HDD) != 0) ? this.__session.version : "";               
                                  } 
                              },
        
        /* A smarter way of accessing flags */
          "flags"         :   {   get: function () {
                                    // Return a smart object with properties that when changed
                                    // they automatically update the session object.
                                    if (!this.__valid) return u; 
                                    return new SessionFlags(this.__session);
                                  },
                                  set: function(v) {
                                    // If the user is setting a number, update the flags object directly
                                    if (typeof(v) == 'number') {
                                        this.__session.flags = v;

                                    // Otherwise parse the flags from an object
                                    } else if (typeof(v) == 'object') {
                                        this.__session.flags = parseSessionFlags(v);
                                    }
                                  } 
                              },
          "daemonFlags"   :   {   get: function () {
                                    // Return a smart object with properties that when changed
                                    // they automatically update the session object.
                                    if (!this.__valid) return u; 
                                    return new DaemonFlags(this.__session);
                                  },
                                  set: function(v) {
                                    // If the user is setting a number, update the flags object directly
                                    if (typeof(v) == 'number') {
                                        this.__session.daemonFlags = v;

                                    // Otherwise parse the flags from an object
                                    } else if (typeof(v) == 'object') {
                                        this.__session.daemonFlags = parseDaemonFlags(v);
                                    }
                                  } 
                              }
    });
    
    // Private function to do cleanup when the session is closed
    var __cleanupCB = (function() {
        clearInterval(__updateTimer);
        this.__valid = false;
    }).bind(this);

    // Bind events and delegate them to the event dispatcher
    this.__session.addEventListener('open',           (function(){      this.__fire('sessionStateChange', STATE_OPEN);    this.__fire('open');                  }).bind(this));
    this.__session.addEventListener('close',          (function(){      this.__fire('sessionStateChange', STATE_CLOSED);  this.__fire('close'); __cleanupCB();  }).bind(this));
    this.__session.addEventListener('start',          (function(){      this.__fire('sessionStateChange', STATE_STARTED); this.__fire('start');                 }).bind(this));
    this.__session.addEventListener('stop',           (function(){      this.__fire('sessionStateChange', STATE_OPEN);    this.__fire('stop');                  }).bind(this));
    this.__session.addEventListener('pause',          (function(){      this.__fire('sessionStateChange', STATE_PAUSED);  this.__fire('pause');                 }).bind(this));
    this.__session.addEventListener('resume',         (function(){      this.__fire('sessionStateChange', STATE_STARTED); this.__fire('resume');                }).bind(this));
    this.__session.addEventListener('hibernate',      (function(){      this.__fire('sessionStateChange', STATE_OPEN);    this.__fire('hibernate');             }).bind(this));
    this.__session.addEventListener('error',          (function(a,b,c){ this.__fire('error',a,b,c); }).bind(this));
    this.__session.addEventListener('reset',          (function(a,b,c){ this.__fire('reset'); }).bind(this));
    this.__session.addEventListener('openError',      (function(a,b){   this.__fire('openError',a,b); }).bind(this));
    this.__session.addEventListener('closeError',     (function(a,b){   this.__fire('closeError',a,b); }).bind(this));
    this.__session.addEventListener('startError',     (function(a,b){   this.__fire('startError',a,b); }).bind(this));
    this.__session.addEventListener('stopError',      (function(a,b){   this.__fire('stopError',a,b); }).bind(this));
    this.__session.addEventListener('pauseError',     (function(a,b){   this.__fire('pauseError',a,b); }).bind(this));
    this.__session.addEventListener('resetError',     (function(a,b){   this.__fire('resetError',a,b); }).bind(this));
    this.__session.addEventListener('hibernateError', (function(a,b){   this.__fire('hibernateError',a,b); }).bind(this));
    this.__session.addEventListener('apiAvailable',   (function(a,b){   this.__fire('apiAvailable', a,b); }).bind(this));
    this.__session.addEventListener('apiUnavailable', (function() {     this.__fire('apiUnavailable'); }).bind(this));

    // Smart progress events
    var inProgress = false;
    this.__session.addEventListener('progress',       (function(a,b,c){ 
        
        // Notify before the first progress event
        if (!inProgress) {
            this.__fire('progressBegin'); 
            inProgress = true;
        }
        
        // Forward progress events in percentage
        this.__fire('progress', Math.round(100 * a / b) ,c ); 
        
        // Notify when total equals current
        if (a == b) {
            this.__fire('progressEnd'); 
            inProgress = false;
        }
        
    }).bind(this));
    
    // Schedule an action to be called after the session is instantiated
    // and the user has already bound his event handlers.
    setTimeout((function() {
        this.__fire('daemonStateChange', this.__daemonStatus);
        this.__fire('systemStateChange', this.__systemStatus);
        if (this.__session.state != STATE_CLOSED) 
            this.__fire('sessionStateChange', this.__session.state );
    }).bind(this), 1);
    
    

};

/**
 * Subclass event dispatcher
 */
_NS_.WebAPISession.prototype = Object.create( _NS_.EventDispatcher.prototype );

/**
 * Proxy functions for start,stop,pause,resume,hibernate and close
 */
_NS_.WebAPISession.prototype.start = function( values, cbOK, cbFail ) {
    
    // Check for invalid session
    if (!this.__valid) return false;
    
    // Check for missing userVariables
    if (typeof(values) == 'function') {
        cbFail = cbOK;
        cbOK = values;
        values = false;
    }
    
    // Invalid values is empty object
    if (!values) values={ };
    
    // Setup proxies
    var cb_once=true,
        cb_proxy_ok=function() { if (cb_once) {cb_once=false} else {return}; if (cbOK) cbOK(); cleanup_cb(); },
        cb_proxy_fail=function(eMsg, eCode) { if (cb_once) {cb_once=false} else {return}; callError( cbFail, 'Could not start: '+eMsg,eCode); cleanup_cb(); },
        cleanup_cb = (function() {
            this.__session.removeEventListener('start', cb_proxy_ok);
            this.__session.removeEventListener('startError', cb_proxy_fail);
        }).bind(this);
        this.__session.addEventListener('start', cb_proxy_ok);
        this.__session.addEventListener('startError', cb_proxy_fail);
    
    // Start session
    var res = this.__session.start(values);
    if (res != HVE_SCHEDULED) {
        callError( cbFail, 'Could not start: '+error_string(res), res );
        return false;
    }
    
    // We are done, check for daemon updates
    this.__checkDaemonUpdates();
    return true;

};
_NS_.WebAPISession.prototype.stop = function( cbOK, cbFail ) {
    
    // Check for invalid session
    if (!this.__valid) return false;
    
    // Setup proxies
    var cb_once=true,
        cb_proxy_ok=function() { if (cb_once) {cb_once=false} else {return}; if (cbOK) cbOK(); cleanup_cb(); },
        cb_proxy_fail=function(eMsg, eCode) { if (cb_once) {cb_once=false} else {return}; callError( cbFail, 'Could not stop: '+eMsg,eCode); cleanup_cb(); },
        cleanup_cb = (function() {
            this.__session.removeEventListener('stop', cb_proxy_ok);
            this.__session.removeEventListener('stopError', cb_proxy_fail);
        }).bind(this);
        this.__session.addEventListener('stop', cb_proxy_ok);
        this.__session.addEventListener('stopError', cb_proxy_fail);
    
    // Stop session
    var res = this.__session.stop();
    if (res != HVE_SCHEDULED) {
        callError( cbFail, 'Could not stop: '+error_string(res), res );
        return false;
    }
    
    // We are done, check for daemon updates
    this.__checkDaemonUpdates();
    return true;

};
_NS_.WebAPISession.prototype.pause = function( cbOK, cbFail ) {
    
    // Check for invalid session
    if (!this.__valid) return false;
    
    // Setup proxies
    var cb_once=true,
        cb_proxy_ok=function() { if (cb_once) {cb_once=false} else {return}; if (cbOK) cbOK(); cleanup_cb(); },
        cb_proxy_fail=function(eMsg, eCode) { if (cb_once) {cb_once=false} else {return}; callError( cbFail, 'Could not pause: '+eMsg,eCode); cleanup_cb(); },
        cleanup_cb = (function() {
            this.__session.removeEventListener('pause', cb_proxy_ok);
            this.__session.removeEventListener('pauseError', cb_proxy_fail);
        }).bind(this);
        this.__session.addEventListener('pause', cb_proxy_ok);
        this.__session.addEventListener('pauseError', cb_proxy_fail);
    
    // Pause session
    var res = this.__session.pause();
    if (res != HVE_SCHEDULED) {
        callError( cbFail, 'Could not pause: '+error_string(res), res );
        return false;
    }
    
    // We are done, check for daemon updates
    this.__checkDaemonUpdates();
    return true;

};
_NS_.WebAPISession.prototype.resume = function( cbOK, cbFail ) {
    
    // Check for invalid session
    if (!this.__valid) return false;
    
    // Setup proxies
    var cb_once=true,
        cb_proxy_ok=function() { if (cb_once) {cb_once=false} else {return}; if (cbOK) cbOK(); cleanup_cb(); },
        cb_proxy_fail=function(eMsg, eCode) { if (cb_once) {cb_once=false} else {return}; callError( cbFail, 'Could not resume: '+eMsg,eCode); cleanup_cb(); },
        cleanup_cb = (function() {
            this.__session.removeEventListener('resume', cb_proxy_ok);
            this.__session.removeEventListener('resumeError', cb_proxy_fail);
        }).bind(this);
        this.__session.addEventListener('resume', cb_proxy_ok);
        this.__session.addEventListener('resumeError', cb_proxy_fail);
    
    // Resume session
    var res = this.__session.resume();
    if (res != HVE_SCHEDULED) {
        callError( cbFail, 'Could not resume: '+error_string(res), res );
        return false;
    }
    
    // We are done, check for daemon updates
    this.__checkDaemonUpdates();
    return true;

};
_NS_.WebAPISession.prototype.hibernate = function( cbOK, cbFail ) {
    
    // Check for invalid session
    if (!this.__valid) return false;
    
    // Setup proxies
    var cb_once=true,
        cb_proxy_ok=function() { if (cb_once) {cb_once=false} else {return}; if (cbOK) cbOK(); cleanup_cb(); },
        cb_proxy_fail=function(eMsg, eCode) { if (cb_once) {cb_once=false} else {return}; callError( cbFail, 'Could not hibernate: '+eMsg,eCode); cleanup_cb(); },
        cleanup_cb = (function() {
            this.__session.removeEventListener('hibernate', cb_proxy_ok);
            this.__session.removeEventListener('hibernateError', cb_proxy_fail);
        }).bind(this);
        this.__session.addEventListener('hibernate', cb_proxy_ok);
        this.__session.addEventListener('hibernateError', cb_proxy_fail);
    
    // Hibernate session
    var res = this.__session.hibernate();
    if (res != HVE_SCHEDULED) {
        callError( cbFail, 'Could not hibernate: '+error_string(res), res );
        return false;
    }
    
    // We are done, check for daemon updates
    this.__checkDaemonUpdates();
    return true;

};
_NS_.WebAPISession.prototype.reset = function( cbOK, cbFail ) {
    
    // Check for invalid session
    if (!this.__valid) return false;
    
    // Setup proxies
    var cb_once=true,
        cb_proxy_ok=function() { if (cb_once) {cb_once=false} else {return}; if (cbOK) cbOK(); cleanup_cb(); },
        cb_proxy_fail=function(eMsg, eCode) { if (cb_once) {cb_once=false} else {return}; callError( cbFail, 'Could not reset: '+eMsg,eCode); cleanup_cb(); },
        cleanup_cb = (function() {
            this.__session.removeEventListener('reset', cb_proxy_ok);
            this.__session.removeEventListener('resetError', cb_proxy_fail);
        }).bind(this);
        this.__session.addEventListener('reset', cb_proxy_ok);
        this.__session.addEventListener('resetError', cb_proxy_fail);
    
    // Reset session
    var res = this.__session.reset();
    if (res != HVE_SCHEDULED) {
        callError( cbFail, 'Could not reset: '+error_string(res), res );
        return false;
    }
    
    // We are done, check for daemon updates
    this.__checkDaemonUpdates();
    return true;

};
_NS_.WebAPISession.prototype.close = function( cbOK ) {
    
    // Check for invalid session
    if (!this.__valid) return false;
    
    // Setup proxies
    var cb_once=true,
        cb_proxy_ok=function() { if (cb_once) {cb_once=false} else {return}; if (cbOK) cbOK(); cleanup_cb(); },
        cleanup_cb = (function() {
            this.__session.removeEventListener('close', cb_proxy_ok);
        }).bind(this);
        this.__session.addEventListener('close', cb_proxy_ok);
    
    // Close & invalidate session
    this.__session.close();
    this.__valid = false;
    
    // We are done, check for daemon updates
    this.__checkDaemonUpdates();
    return true;

};