
/**
 * Private variables
 */
var __pluginSingleton = null,
    __pageLoaded = false,
    __loadHooks = [];
    
/**
 * This is an asynchronous, overridable function to display messages.
 * The user can specify his own (more beautiful) confirmation dialog using
 * the .setConfirmFunction( ).
 */
var confirmFunction = function( message, callback ) { callback( window.confirm(message) ); };
_NS_.setConfirmFunction = function( customFunction ) { confirmFunction = customFunction; };

/**
 * Global function to initialize the plugin and callback when ready
 *
 * @param cbOK              A callback function that will be fired when a plugin instance is obtained
 * @param cbFail            [Optional] A callback that will be fired when an error occurs
 * @param setupEnvironment  [Optional] Set to 'true' to let library handle environment initialization
 */
_NS_.startCVMWebAPI = function( cbOK, cbFail, setupEnvironment ) {
    
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
        
        // Next steop of construction process, stored in a function as a reusable approach
        var continue_init = function() {
            // Request daemon access in order to be able to do status probing
            __pluginSingleton.requestDaemonAccess(
                function(daemon) {
                    cbOK( new _NS_.WebAPIPlugin( __pluginSingleton, daemon ) ); 
                },function(error) {
                    cbFail( "Unable to obtain daemon access: " + error_string(error), error );
                }
            );
        }
         
        
        // Validate plugin status
        if (__pluginSingleton.version == undefined) {
            
            // Check if we are told to take care of setting up the environment for the user
            if (setupEnvironment) {
                
                // Prompt the user for plugin installation
                // (We are using the overridable, asynchronous confirm function)
                confirmFunction(
                    "This website is using the CernVM Web API extension, but it doesn't seem to be installed in your browser.\n\nDo you want to install it now?",
                    function(confirmed) {
                        if (confirmed) {
                            // Check the browser
                            if (BrowserDetect.browser == "Firefox") {
                                window.location = "http://labs.wavesoft.gr/micro/res/cvmwebapi-1.1.4.xpi";
                            } else if (BrowserDetect.browser == "Chrome") {
                                window.location = "https://chrome.google.com/webstore/detail/cernvm-web-api/iakpghcolokcngbhjiihjcomioihjnfm";
                            } else {
                                window.location = "http://cernvm.cern.ch/portal/webapi";
                            }
                        } else {
                            cbFail( "Unable to load CernVM WebAPI Plugin. Make sure it's installed!", -100 );
                        }
                    }
                );
                
            } else {
                // Could not do anything, fire the error callback
                cbFail( "Unable to load CernVM WebAPI Plugin. Make sure it's installed!", -100 );
            }
            
        } else {
            console.log("Using CernVM WebAPI " + __pluginSingleton.version);
            
            // If we are told to check the environment, check if we have hypervisor
            if (setupEnvironment) {
                if (__pluginSingleton.hypervisorName == "") {

                    // Prompt the user for plugin installation
                    // (We are using the overridable, asynchronous confirm function)
                    confirmFunction(
                        "You are going to need a hypervisor before you can use this website. Do you allow the CernVM Web API extension to install Oracle VirtualBox for you?",
                        function(confirmed) {
                            if (confirmed) {

                                var once=true;
                                var cb_ok = function() {
                                    if (once) { once=false } else { return };
                                    __pluginSingleton.removeEventListener('install', cb_ok);

                                    // Hypervisor is now in place, continue
                                    continue_init();

                                };
                                var cb_error = function(code) {
                                    if (once) { once=false } else { return };
                                    __pluginSingleton.removeEventListener('installError', cb_error);
                                    cbFail( "Unable to install hypervisor!", -103 );
                                };

                                // Setup callbacks and install hypervisor
                                __pluginSingleton.addEventListener('install', cb_ok);
                                __pluginSingleton.addEventListener('installError', cb_error);
                                __pluginSingleton.installHypervisor();

                            } else {
                                cbFail( "User denied hypervisor installation!", -102 );
                            }
                        }
                    );
                    
                } else {
                    
                    // Hypervisor is there, continue...
                    continue_init();
                    
                }
            
            // Otherwise, that's the user's responsibility
            } else {
                continue_init();
            }
            
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
