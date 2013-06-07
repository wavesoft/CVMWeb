
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
    
    // Request safe session (Using a VMCP URL)
    this.__plugin.requestSafeSession( configURL, function(session) {
        cSession = session;
        
        // Open session if it's closed
        if ((session.state == STATE_CLOSED) || (session.state == STATE_ERROR)) {
            
            // Register some temporary event handlers
            var f_open = function() {
                session.removeEventListener( 'open', f_open );
                cbOK( new _NS_.WebAPISession( this.__plugin, this.__daemon, session ) );
            };
            var f_error = function( code ) {
                session.removeEventListener( 'error', f_error );
                if (cbFail) cbFail( "Could not open session: " + error_string(code) + ".", code )
            };
            session.addEventListener('open', f_open, f_error);
            session.addEventListener('openError', cb_open);
            
            // Open session
            session.open();
            
        } else {
            
            // Session is already open, notify caller
            cbOK( new _NS_.WebAPISession( this.__plugin, this.__daemon, session ) );
            
        }
        
    }, function(code) {
        console.log("RequestSession Error #" + code);
        if (cbFail) cbFail( "Could not request session: " + error_string(code) + ".", code );
    });
    
};