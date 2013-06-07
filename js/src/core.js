
/**
 * Private variables
 */
var __pluginSingleton = null,
    __pageLoaded = false,
    __loadHooks = [];

/**
 * Global function to initialize the plugin and callback when ready
 */
_NS_.startCVMWebAPI = function( cbOK, cbFail ) {
    
    // Create a local callback that will check the plugin
    // status and trigger the appropriate user callbacks
    var cb_check = function() { 
        
        // Singleton access to the plugin
        if (__pluginSingleton == null) {

            // Check if we already have an embed element
            __pluginSingleton = document.getElementById('cvmweb-api');

            // Nope, create new instance
            if (!__pluginSingleton) {
                __pluginSingleton = document.createElement('embed');

                // Still problems?
                if (__pluginSingleton == null) {
                    cbFail( "Your browser does not support the <embed /> tag!", -101 );
                }

                // Initialize
                __pluginSingleton.type = "application/x-cvmweb";
                __pluginSingleton.id = "cvmweb-api";
                __pluginSingleton.style.width = "1px";
                __pluginSingleton.style.height = "1px";
                document.body.appendChild( __pluginSingleton );
            }

        }
        
        // Validate plugin status
        if (__pluginSingleton.version == undefined) {
            cbFail( "Unable to load CernVM WebAPI Plugin. Make sure it's installed!", -100 );
        } else {
            console.log("Using CernVM WebAPI " + __pluginSingleton.version);
            cbOK( new _NS_.WebAPIPlugin(__pluginSingleton) ); 
        }
        
    };
    
    // If the page is still loading, request an onload hook,
    // otherwise proceed with verification
    if (!__pageLoaded) {
        __loadHooks.push( cb_check );
    } else {
        cb_check();
    }
    
};
