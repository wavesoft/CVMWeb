
/**
 * Developer-friendly wrapper for the session object
 */
_NS_.WebAPISession = function( plugin_ref, raemon_ref, session_ref ) {
    var self = this;
    _NS_.EventDispatcher.call(this); // Superclass constructor
    this.__plugin = plugin_ref;
    this.__daemon = daemon_ref;
    this.__session = session_ref;
    
    // Now that we have a session we can probe for daemon status
    this.__daemonStatus = false;
    this.__systemStatus = false;
    setInterval(function() {
        var running = self.__daemon.isRunning,
            idle = self.__daemon.isRunning;
            
        // Check daemon state change
        if (running != self.__daemonStatus) {
            self.__daemonStatus = running;
            self.fire('daemonStateChanged', running);
        }
        
        // Check system idle state change
        if (idle != self.__systemStatus) {
            self.__systemStatus = idle;
            self.fire('systemStateChanged', idle);
        }
        
    }, 5000);
    
    // Connect plugin properties with this object properties using getters/setters
    Object.defineProperties(this, {
        "state" : { get: function () { return this.__session.state; } },
        "ip"    : { get: function () { return this.__session.ip; } }
    });

    // Schedule an action to be called after the session is instantiated
    // and the user has already bound his event handlers.
    setTimeout(function() {
        self.fire('daemonStateChanged', self.__daemonStatus);
        self.fire('systemStateChanged', self.__systemStatus);
        if (self.__session.state != STATE_CLOSED) 
            self.fire('sessionStateChange', self.__session.state );
    }, 10);

};

/**
 * Subclass event dispatcher
 */
_NS_.WebAPISession.prototype = Object.create( _NS_.EventDispatcher.prototype );
