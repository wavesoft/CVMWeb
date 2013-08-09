
/**
 * Developer-friendly wrapper for the session object
 */
_NS_.WebAPIPlugin = function( plugin_ref, daemon_ref ) {
    _NS_.EventDispatcher.call(this); // Superclass constructor
    this.__plugin = plugin_ref;
    this.__daemon = daemon_ref;
    
    // Connect plugin properties with this object properties using getters/setters
    Object.defineProperties(this, {
        "domain": { get: function () { return this.__plugin.domain; } }
    });
    
    // Bind on install progress events
    var inProgress = false,
        pGroupID = 0;
    this.__plugin.addEventListener('installProgress', (function(a,b,c){ 

        // Notify before the first progress event
        if (!inProgress) {
            this.__fire('installProgressBegin'); 
            pGroupID = notifyGlobalBeginProgress(b);
            inProgress = true;
        }

        // Forward progress events in percentage
        this.__fire('installProgress', Math.round(100 * a / b) ,c ); 
        notifyGlobalProgress( a,b,c, pGroupID );

        // Notify when total equals current
        if (a == b) {
            this.__fire('installProgressEnd'); 
            inProgress = false;
        };

    }).bind(this));

    // Forward installation events
    this.__plugin.addEventListener('install',        (function(){      this.__fire('installCompleted'); notifyGlobalEndProgress(pGroupID); }).bind(this));
    this.__plugin.addEventListener('installError',   (function(){      this.__fire('installError');     notifyGlobalEndProgress(pGroupID); }).bind(this));
    
};

/**
 * Subclass event dispatcher
 */
_NS_.WebAPIPlugin.prototype = Object.create( _NS_.EventDispatcher.prototype );
    
/**
 * Check if the hypervisor is there
 */
_NS_.WebAPIPlugin.prototype.hasHypervisor = function() {
    if (!this.__plugin) return false;
    return (this.__plugin.version != undefined) && (this.__plugin.hypervisorName != "");
},

/**
 * Request session
 *
 * @param configURL     Specifies the location for the configuration information to use for creating the new VM
 * @param cbOk          The function to call if everything went as expected and the session is up and running
 * @param cbFail        [Optional] A callback that will be called if an error occured while requesting the session
 *
 * ( For more details on the format of the VMCP URL check the https://github.com/wavesoft/CVMWeb/wiki/VMCP-Reference )
 * 
 */
_NS_.WebAPIPlugin.prototype.requestSession = function( configURL, cbOK, cbFail ) {
    
    // Sanitize user input
    if (!configURL) return false;
    if (!cbOK) return false;
    if (typeof(configURL) != "string") return false;
    if (typeof(cbOK) != "function") return false;
    if ((cbFail) && (typeof(cbFail) != "function")) return false;
    
    // Bind events to the global event handler using the same smart progress event handler
    // as in session. We create this function before we initiate the session in order to get
    // events from the onProgress() callback of requestSafeSession
    var inProgress = false,
        pGroupID = 0,
        progressFunction = (function(a,b,c){ 

            // Notify before the first progress event
            if (!inProgress) {
                this.__fire('progressBegin'); 
                pGroupID = notifyGlobalBeginProgress(b);
                inProgress = true;
            }

            // Forward progress events in percentage
            this.__fire('progress', Math.round(100 * a / b) ,c );
            notifyGlobalProgress( a,b,c, pGroupID );
            console.log("[DBG] ",a,"==",b);

            // Notify when total equals current
            if (a == b) {
                this.__fire('progressEnd'); 
                notifyGlobalEndProgress(pGroupID);
                inProgress = false;
            }

        }).bind(this);
        
    // Request safe session (Using a VMCP URL)
    this.__plugin.requestSafeSession( configURL, (function(session) {
        cSession = session;
        
        // If we are activating debug logging, hook the 'debug' event
        // to the console log.
        if (_NS_.debugLogging) {
            cSession.addEventListener('debug', function(log) {
                console.log("[Debug] " + log);
            });
        }
        
        // Bind event listener
        cSession.addEventListener('progress', progressFunction);
        
        // Open session if it's closed
        if ((session.state == STATE_CLOSED) || (session.state == STATE_ERROR)) {
            
            // Register some temporary event handlers
            var once = true;
            var f_open = (function() {
                if (once) { once=false } else { return };
                session.removeEventListener( 'open', f_open );
                cbOK( new _NS_.WebAPISession( this.__plugin, this.__daemon, session ) );
            }).bind(this);
            var f_error = (function( msg, code ) {
                if (once) { once=false } else { return };
                session.removeEventListener( 'error', f_error );
                callError( cbFail, "Could not open session: " + error_string(code) + ".", code )
            }).bind(this);
            session.addEventListener('open', f_open);
            session.addEventListener('openError', f_error);
            
            // Open session
            session.open();
            
        } else {
            
            // Session is already open, notify caller
            cbOK( new _NS_.WebAPISession( this.__plugin, this.__daemon, session ) );
            
        }
        
    }).bind(this), function(code) {
        console.error("RequestSession Error #" + code);
        callError( cbFail, "Could not request session: " + error_string(code) + ".", code );
    }, 
        progressFunction
    );
    
};