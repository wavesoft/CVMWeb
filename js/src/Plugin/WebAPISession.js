
/**
 * Developer-friendly wrapper for the session object
 */
_NS_.WebAPISession = function( plugin_ref, daemon_ref, session_ref ) {
    _NS_.EventDispatcher.call(this); // Superclass constructor
    this.__plugin = plugin_ref;
    this.__daemon = daemon_ref;
    this.__session = session_ref;
    
    // Now that we have a session we can probe for daemon status
    this.__daemonStatus = false;
    this.__systemStatus = false;
    
    setInterval((function() {
        var running = this.__daemon.isRunning,
            idle = this.__daemon.isSystemIdle;
            
        // Check daemon state change
        if (running != this.__daemonStatus) {
            this.__daemonStatus = running;
            this.fire('daemonStateChange', running);
        }
        
        // Check system idle state change
        if (idle != this.__systemStatus) {
            this.__systemStatus = idle;
            this.fire('systemStateChange', idle);
        }
        
    }).bind(this), 5000);
    
    // Connect plugin properties with this object properties using getters/setters
    Object.defineProperties(this, {
        "state" : { get: function () { return this.__session.state; } },
        "ip"    : { get: function () { return this.__session.ip; } }
    });

    // Schedule an action to be called after the session is instantiated
    // and the user has already bound his event handlers.
    setTimeout((function() {
        this.fire('daemonStateChange', this.__daemonStatus);
        this.fire('systemStateChange', this.__systemStatus);
        if (this.__session.state != STATE_CLOSED) 
            this.fire('sessionStateChange', this.__session.state );
    }).bind(this), 1);
    
    // Bind events and delegate them to the event dispatcher
    this.__session.addEventListener('open',           (function(){      this.fire('sessionStateChange', STATE_OPEN);    this.fire('open'); }).bind(this));
    this.__session.addEventListener('close',          (function(){      this.fire('sessionStateChange', STATE_CLOSED);  this.fire('close'); }).bind(this));
    this.__session.addEventListener('start',          (function(){      this.fire('sessionStateChange', STATE_STARTED); this.fire('start'); }).bind(this));
    this.__session.addEventListener('stop',           (function(){      this.fire('sessionStateChange', STATE_OPEN);    this.fire('stop'); }).bind(this));
    this.__session.addEventListener('pause',          (function(){      this.fire('sessionStateChange', STATE_PAUSED);  this.fire('pause'); }).bind(this));
    this.__session.addEventListener('resume',         (function(){      this.fire('sessionStateChange', STATE_STARTED); this.fire('resume'); }).bind(this));
    this.__session.addEventListener('hibernate',      (function(){      this.fire('sessionStateChange', STATE_OPEN);    this.fire('hibernate'); }).bind(this));
    this.__session.addEventListener('error',          (function(a,b,c){ this.fire('error',a,b,c); }).bind(this));
    this.__session.addEventListener('reset',          (function(a,b,c){ this.fire('reset'); }).bind(this));
    this.__session.addEventListener('apiAvailable',   (function(a,b){   this.fire('apiAvailable', a,b); }).bind(this));
    this.__session.addEventListener('apiUnavailable', (function() {     this.fire('apiUnavailable'); }).bind(this));

    // Smart progress events
    var inProgress = false;
    this.__session.addEventListener('progress',       (function(a,b,c){ 
        
        // Notify before the first progress event
        if (!inProgress) {
            this.fire('progressBegin'); 
            inProgress = true;
        }
        
        // Forward progress events in percentage
        this.fire('progress', Math.round(100 * a / b) ,c ); 
        
        // Notify when total equals current
        if (a == b) {
            this.fire('progressEnd'); 
            inProgress = false;
        }
        
    }).bind(this));

};

/**
 * Subclass event dispatcher
 */
_NS_.WebAPISession.prototype = Object.create( _NS_.EventDispatcher.prototype );

/**
 * Merge/override user-data
 */
_NS_.WebAPISession.prototype.start = function( userVariables, cbOK, cbFail ) {
    var cb_proxy_ok=null,
        cb_proxy_free=null,;
    
    
    this.__session.start();
};

_NS_.WebAPISession.prototype.stop = function( userVariables ) {
    this.__session.stop();
};
