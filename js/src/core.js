
/**
 * Global initialize function that creates and returns only a single instance to the CernVM plugin.
 * You should use this function instead of accessing the singleton variable in order to initialize the
 * plugin only when it's needed and not on every time the page is loaded.
 * (That's because there is a small delay during the initialization that might annoy the users)
 */
var __pluginSingleton = null;
function getPlugin() {
    if (__pluginSingleton == null) {
        __pluginSingleton = document.createElement('embed');
        __pluginSingleton.type = "application/x-cvmweb";
        __pluginSingleton.style.width = "1px";
        __pluginSingleton.style.height = "1px";
        document.body.appendChild( __pluginSingleton );
    }
    return __pluginSingleton;
}

/**
 * CVM is our namespace
 */
var CVM = GLOBAL.CVM = {
    version : '1.0',
    
    /**
     * Static function to check if the plugin is there
     */
    hasPlugin: function() {
        var plugin = getPlugin();
        if (!plugin) return false;
        return (plugin.version != undefined);
    },
    
    /**
     * Static function to check if the hypervisor is there
     */
    hasHypervisor: function() {
        var plugin = getPlugin();
        if (!plugin) return false;
        return (plugin.version != undefined) && (plugin.hypervisorName != "");
    },
    
    /**
     * Request session
     *
     * @param configURL     Specifies the location for the configuration information to use for creating the new VM
     * @param callback      The function to call if everything went as expected and the session is up and running
     * @param errorCallback [Optional] A callback that will be called if an error occured while requesting the session
     *
     * ( For more details on the format of the VMCP URL check the https://github.com/wavesoft/CVMWeb/wiki/VMCP-Reference )
     * 
     */
    requetSession: function( configURL, callback, errorCallback ) {
        var plugin = getPlugin();
        if (!plugin) return false;
        if (!config) return false;
        if (!callback) return false;
        
        // Request safe session (Using a VMCP URL)
        plugin.requestSafeSession( configURL, function(session) {
            cSession = session;
            
            // Open session if it's closed
            if (session.state == STATE_CLOSED) {
                
                // Register some temporary event handlers
                var f_open = function() {
                    session.removeEventListener( 'open', f_open );
                    callback( session );
                };
                var f_error = function( code ) {
                    session.removeEventListener( 'error', f_error );
                    if (errorCallback) errorCallback( code )
                };
                session.addEventListener('open', f_open, f_error);
                session.addEventListener('openError', cb_open);
                
                // Open session
                session.open();
                
            } else {
                
                // Session is already open, notify caller
                callback( session );
                
            }
            
        }, function(e) {
            console.log("RequestSession Error #" + e);
            if (errorCallback) errorCallback(e);
        });
        
    }
        
};
