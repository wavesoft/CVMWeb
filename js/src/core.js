
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
var confirmFunction = _NS_.__globalConfirm = function( message, callback ) { 
    if (!callback) {
        if (__pluginSingleton!=null) __pluginSingleton.confirmCallback( window.confirm(message) );
    } else {
        callback( window.confirm(message) ); 
    }
};
_NS_.setConfirmFunction = function( customFunction ) {
    
    // Small script that will forward callback messages from
    // the user's function only once! 
    var messagePending = false;
    confirmFunction = _NS_.__globalConfirm = function(message, callback) {
        messagePending = true;
        customFunction( message, function(response) {
            if (messagePending) {
                messagePending = false;
                if (!callback) {
                    if (__pluginSingleton!=null) __pluginSingleton.confirmCallback( response );
                } else {
                    callback(response);
                }
            }
        });
    };
    
};

/**
 * Global flags
 */
_NS_.debugLogging = false;
_NS_.autoUpdate = true;

/**
 * This is an overridable alert function, so the user can specify
 * anoter, more beautiful implementation.
 */
var alertFunction = function(msg) { alert(msg) };
_NS_.setAlertFunction = function( customFunction ) { alertFunction = customFunction; };

/**
 * Global error handler
 */
var globalErrorHandler = function(msg, code) { };
_NS_.setGlobalErrorHandler = function( customFunction ) { globalErrorHandler = customFunction; };

/**
 * Global progress handlers
 */
var globalProgressBegin = function() { },
    globalProgressEnd = function() { },
    globalProgressEvent = function(percent, message) { };
_NS_.setGlobalProgressHandler = _NS_.setGlobalProgressHandlers = function( onProgress, onBegin, onEnd ) {
    if (onProgress) globalProgressEvent=onProgress;
    if (onBegin) globalProgressBegin=onBegin;
    if (onEnd) globalProgressEnd=onEnd;
};

/**
 * Helper function to start RDP client using the jar from CernVM
 */
_NS_.launchRDP = function( rdpURL, resolution ) {

    // Process resolution parameter
    var width=800, height=600, bpp=24;
    if (resolution != undefined ) {
        // Split <width>x<height>x<bpp> string into it's components
        var res_parts = resolution.split("x");
        width = parseInt(res_parts[0]);
        height = parseInt(res_parts[1]);
        if (res_parts.length > 2)
            bpp  = parseInt(res_parts[2]);
    }

    // Open web-RDP client from CernVM
    var w = window.open(
        'http://cernvm.cern.ch/releases/webapi/webrdp/webclient.html#' + rdpURL + ',' + width + ',' + height, 
        'WebRDPClient', 
        'width=' + width + ',height=' + height
    );

    // Align, center and focus
    w.moveTo( (screen.width - width)/2, (screen.height - height)/2 );
    setTimeout(function() { w.focus() }, 100);
    w.focus();

    /*

    //########################################//
    //########################################//
    //###### JAVA SOLUTION DEPRECATED ########//
    //###### CODE KEPT FOR REFERENCE  ########//
    //########################################//
    //########################################//

    // Prepare applet
    var applet = document.createElement('applet');
    applet.setAttribute('codebase', 'http://cernvm.cern.ch/releases/webapi/js');
    applet.setAttribute('code', 'net.propero.rdp.applet.RdpApplet.class');
    applet.setAttribute('archive', 'cernvm-properrdp-14.1.1.jar');
    applet.setAttribute('width', '1');
    applet.setAttribute('height', '1');
    applet.setAttribute('align', 'top');
    applet.style['visibility'] = 'hidden';

    // Additional JVM arguments
    var param = document.createElement('param');
    param.name = "java_arguments";
    param.value = "-d32";
    applet.appendChild(param);

    // Specify URL
    var param = document.createElement('param');
    param.name = "server";
    param.value = rdpURL;
    applet.appendChild(param);

    // Process resolution parameter
    var geom, bpp;
    if (resolution != undefined ) {
        // Split <width>x<height>x<bpp> string into it's components
        var res_parts = resolution.split("x");
        geom = res_parts[0] + 'x' + res_parts[1];
        bpp  = res_parts[2];
    }

    // Put resolution information
    var param = document.createElement('param');
    param.name = "geometry";
    param.value = geom || "1024x768";
    applet.appendChild(param);

    // But bpp information
    if (bpp) {
        var param = document.createElement('param');
        param.name = "bpp";
        param.value = bpp;
        applet.appendChild(param);
    }

    // Put applet on body (it will pop-up immediately a new window)
    document.body.appendChild(applet);
    return applet;
    */

};

/**
 * Library-less way of presenting the oracle PUEL License
 */
function presentOracleLicense( cbAccept, cbDecline ) {
     var o = document.createElement('div'),
         c = document.createElement('div'),
         cControls = document.createElement('div'),
         cHeader = document.createElement('div'),
         fLicense = document.createElement('iframe'),
         lnkOk = document.createElement('input'),
         lnkCancel = document.createElement('input');

     lnkOk.innerHTML = 'Accept';
     lnkCancel.innerHTML = 'Decline';
     fLicense.src = 'https://www.virtualbox.org/wiki/VirtualBox_PUEL';
     fLicense.width = 700;
     fLicense.height = 450;
     fLicense.frameBorder = 0;

     // Prepare buttons
     lnkOk.type = 'button';
     lnkOk.value = 'Accept License';
     lnkCancel.type = 'button';
     lnkCancel.value = 'Decline License';
     lnkOk.onclick = function() {
        document.body.removeChild(o);
        if (cbAccept) cbAccept();
     };
     lnkCancel.onclick = function() {
        document.body.removeChild(o);
        if (cbDecline) cbDecline();
     };

     // Outer container
     o.style['display'] = 'block';
     o.style['position'] = 'absolute';
     o.style['width'] = '100%';
     o.style['height'] = '580px';
     o.style['margin'] = '-300px 0px 0px 0px';
     o.style['left'] = '0px';
     o.style['top'] = '50%';
     o.style['font'] = '14px Verdana, Helvetica, Tahoma, Arial';
     o.appendChild(c);

     // Actual container
     c.style['width'] = '705px';
     c.style['height'] = '580px';
     c.style['margin'] = 'auto';
     c.style['background'] = '#999';
     c.style['border'] = 'solid 2px #999';
     c.style['borderRadius'] = '5px';

     // Header
     cHeader.style['color'] = '#fff';
     cHeader.style['background'] = '#999';
     cHeader.style['height'] = '70px';
     cHeader.style['padding'] = '8px';
     cHeader.innerHTML = '<p>The CernVM WebAPI is going to install the VirtualBox Extension Pack which is licensed under VirtualBox Personal Use and Evaluation License (PUEL) License.</p><p>You must accept it to continue:</p>';

     // Controls container
     cControls.style['background'] = '#999';
     cControls.style['padding'] = '6px';
     cControls.style['textAlign'] = 'center';

     // Nest layout
     c.appendChild(cHeader);
     c.appendChild(fLicense);
     c.appendChild(cControls);
     cControls.appendChild(lnkOk);
     cControls.appendChild(lnkCancel);

     document.body.appendChild( o );
}


/**
 * High-level progress forwarding functions
 */
var currentProgressTotal = 0,
    lastProgressID = 0,
    progressGroups = { },
    completeTimer = 0,
    notifyGlobalBeginProgress = function( total ) {
        
        // Fore ProgressBegin if that's a new progress event
        if (currentProgressTotal == 0) {
            if (_NS_.debugLogging) console.log("[Progress] Started");
            globalProgressBegin();
        }
        
        // Abort globalProgressEnd event if it hasn't timed out yet
        if (completeTimer != 0) { 
            clearTimeout(completeTimer); 
            completeTimer=0
        };
        
        // Stack total progress
        currentProgressTotal+=total;
        
        // Allocate a progress group
        var id = ++lastProgressID;
        progressGroups['i'+id] = {'t':Number(total),'s': 0};
        return id;
        
    }, notifyGlobalEndProgress = function( i ) {
        
        // Update the total progress from the progress group
        currentProgressTotal-=progressGroups['i'+i].t;
        delete progressGroups['i'+i];

        // Check if that was the last progress
        if (currentProgressTotal == 0) {
            
            // Fire globalProgressEnd with a delay of 100ms, giving
            // enough time for the second progress event to jump-in
            
            if (completeTimer == 0) {
                completeTimer = setTimeout(function() {
                    completeTimer = 0;
                    if (_NS_.debugLogging) console.log("[Progress] Completed");
                    globalProgressEnd();
                }, 100);
            }
        }
        
    }, notifyGlobalProgress = function( step, total, message, i ) {
        var s=0,t=0;
        progressGroups['i'+i].s = Number(step);
        for (var k in progressGroups) {
            if (k.substr(0,1) == 'i') {
                s+= progressGroups['i'+i].s;
                t+= progressGroups['i'+i].t;
            }
        }
        if (_NS_.debugLogging) console.log("[Progress] " + s + "/" + t + ": " + message);
        globalProgressEvent( Math.round(100 * s / t), message );
    }

/**
 * Shorthand function to call both the global error handler and the user-specified
 * error callback (if it's valid)
 */
function callError( userCallback, message, code ) {

    // Console log is first, no matter what
    var lastError = "";
    if (__pluginSingleton != null) {
        lastError = __pluginSingleton.lastError;
        if ((lastError != undefined) && (lastError != ""))
            lastError = " ("+lastError+")";
    }
    if (!lastError) lastError="";
    console.error("[CernVM Web API] Error #" + code + ": " + message + " " + lastError);

    // Handle user's callback first
    if (userCallback)
        if (userCallback(message, code)) return;

    // And global error callback last
    globalErrorHandler( message, code );

};

/**
 * Global function to receive installation/environment setup progress updates
 */
var globalInstallProgressHandlers = [ ];
_NS_.addInstallProgressHandler = function(handler) {
    globalInstallProgressHandlers.push(handler);
};
_NS_.removeInstallProgressHandler = function(handler) {
    var i = globalInstallProgressHandlers.indexOf(handler);
    globalInstallProgressHandlers.splice(i,1);
};

/**
 * Let CVMWeb that the page is loaded (don't register page loaded)
 */
_NS_.markPageLoaded = function() {
    __pageLoaded = true;
};

/**
 * Global function to initialize the plugin and callback when ready
 *
 * @param cbOK              A callback function that will be fired when a plugin instance is obtained
 * @param cbFail            [Optional] A callback that will be fired when an error occurs
 * @param setupEnvironment  [Optional] Set to 'true' to let library handle environment initialization
 */
_NS_.startCVMWebAPI = function( cbOK, cbFail, setupEnvironment ) {
    
    // Check for missing cbFail function : We can say setupEnvironment=true
    // without having to specify a null cbFail.
    if (cbFail === true) {
        setupEnvironment = true;
        cbFail = null;
    }

    // Forward progress events to the global function handlers (if any)
    var cb_progress = function(curr, total, msg) {
        for (var i=0; i<globalInstallProgressHandlers.length; i++) {
            globalInstallProgressHandlers[i]( Math.round(100 * curr / total), msg);
        }
    }
    
    // Create a local callback that will check the plugin
    // status and trigger the appropriate user callbacks
    var cb_check = function() { 
        
        // Next steop of construction process, stored in a function as a reusable approach
        var continue_init = function() {
            // Request daemon access in order to be able to do status probing
            __pluginSingleton.requestDaemonAccess(
                function(daemon) {
                    cbOK( new _NS_.WebAPIPlugin( __pluginSingleton, daemon ) ); 
                },function(error) {
                    callError( cbFail, "Unable to obtain daemon access: " + error_string(error), error );
                }
            );
        }
        
        // Try to install a missing plugin
        var install_plugin = function() {
            
            // For chrome, we must add a link on head in advance
            if (BrowserDetect.browser == "Chrome") {
                var linkElm = document.createElement('link');
                linkElm.setAttribute("rel", "chrome-webstore-item");
                linkElm.setAttribute("href", "https://chrome.google.com/webstore/detail/iakpghcolokcngbhjiihjcomioihjnfm");
                document.head.appendChild(linkElm);
            }
            
            // Prompt the user for plugin installation
            // (We are using the overridable, asynchronous confirm function)
            confirmFunction(
                "This website is using the CernVM Web API extension, but it doesn't seem to be installed in your browser.\n\nDo you want to install it now?",
                function(confirmed) {
                    if (confirmed) {
                        // Check the browser
                        if (BrowserDetect.browser == "Firefox") {
                            
                            // For firefox, just point to the XPI, the user will be prompted
                            window.location = "http://cernvm.cern.ch/releases/webapi/plugin/cvmwebapi-latest.xpi";
                            
                        } else if (BrowserDetect.browser == "Chrome") {
                            
                            try {
                                // And then trigger the installation
                                chrome.webstore.install("https://chrome.google.com/webstore/detail/iakpghcolokcngbhjiihjcomioihjnfm", function() {
                                    // Installed, reload
                                    location.reload();
                                }, function(e) {
                                    // Automatic installation failed, try manual
                                    window.location = "https://chrome.google.com/webstore/detail/iakpghcolokcngbhjiihjcomioihjnfm";
                                });
                            } catch (e) {
                                // Automatic installation failed, try manual
                                window.location = "https://chrome.google.com/webstore/detail/iakpghcolokcngbhjiihjcomioihjnfm";
                            }
                            
                        } else {
                            window.location = "http://cernvm.cern.ch/portal/webapi";
                        }
                    } else {
                        callError( cbFail, "Unable to load CernVM WebAPI Plugin. Make sure it's installed!", -100 );
                    }
                }
            );
            
        } 

        // Function to validate plug-in status and continue
        var validate_plugin = function() {
            
            // Banner of the webAPI 
            if (__pluginSingleton.version == undefined) {
                console.log("CernVM WebAPI not installed");
            } else if (__pluginSingleton.hypervisorVersion == "") {
                console.log("Using CernVM WebAPI " + __pluginSingleton.version + " with no hypervisor installed");
            } else {
                console.log("Using CernVM WebAPI " + __pluginSingleton.version + " with " + __pluginSingleton.hypervisorName + " version " + __pluginSingleton.hypervisorVersion);
            }

            // Validate plugin status
            if (__pluginSingleton.version == undefined) {
                
                // Check if we are told to take care of setting up the environment for the user
                if (setupEnvironment) {
                
                    // Canclling timeout
                    var cTimeout = 0;
                
                    // Make sure it's not a VS Runtime error:
                    var s = document.createElement('script');
                    s.type = 'text/javascript';
                    if (BrowserDetect.browser == "Firefox") {
                        s.href = "resource://cvmwebapi/detector.js";
                    } else if (BrowserDetect.browser == "Chrome") {
                        s.href = "chrome-extension://nkboedinkpfjfdlaplmgjdiohgabopkn/files/detector.js";
                    }
                    
                    // Detector exists. Which means the extension is installed, but the
                    // binary component was not able to load. If we are on windows, 
                    s.onload = function() {
                        clearTimeout(cTimeout);
                        if (__pluginSingleton.version != window.__CVMDetector.version)
                            console.warn("Incompatible versions between the plug-in extension and the binary component!");
                        
                        if (BrowserDetect.OS == "Windows") {
                            confirmFunction(
                                "It seems you are missing the Microsoft Visual C++ 2010 Redistributable Package which is required in order to run the plug-in.\n" +
                                "Do you want to go to the microsoft website to download it?",
                                function(confirm) {
                                    if (confirm)
                                        window.location = 'http://www.microsoft.com/en-us/download/details.aspx?id=5555';
                                });
                                
                        } else if (BrowserDetect.OS == "Linux") {
                            alertFunction(
                                "Unfortunately, your linux distribution is not supported by this version of the plug-in.\n" +
                                "Currently only Ubuntu 12.04 (or newer) and simmilar distributions are supported."
                                );
                            
                        } else {
                            alertFunction(
                                "It seems that you have installed the CernVM WebAPI Extension, but the plugin was not unable to load.\n" +
                                "Please try restarting the browser and re-installing the extension."
                                );
                        }
                    }
                    
                    var installStarted = false;
                    s.onerror = function() {
                        // Detector does not exist. This menans
                        if (installStarted) return;
                        installStarted = true;
                        install_plugin();
                    }
                    cTimeout = setTimeout(function() {
                        // Operation timed out. Assume that the extension is not loaded
                        if (installStarted) return;
                        installStarted = true;
                        install_plugin();
                    }, 1000);
                    document.head.appendChild(s)
                    
                } else {
                    // Could not do anything, fire the error callback
                    callError( cbFail, "Unable to load CernVM WebAPI Plugin. Make sure it's installed!", -100 );
                }
                
            } else {
                
                // If we are told to check the environment, check if we have hypervisor
                if (setupEnvironment) {
                    if (__pluginSingleton.hypervisorName == "") {

                        // Prompt the user for plugin installation
                        // (We are using the overridable, asynchronous confirm function)
                        confirmFunction(
                            "You are going to need a hypervisor before you can use this website. Do you allow the CernVM Web API extension to install Oracle VirtualBox for you?",
                            function(confirmed) {
                                if (confirmed) {

                                    // The user has accepted the oracle license
                                    var confirmInstall = function() {

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
                                            callError( cbFail, "Unable to install hypervisor!", -103 );
                                        };

                                        // Setup callbacks and install hypervisor
                                        __pluginSingleton.addEventListener('install', cb_ok);
                                        __pluginSingleton.addEventListener('installError', cb_error);
                                        __pluginSingleton.addEventListener('installProgress', cb_progress);
                                        __pluginSingleton.installHypervisor();
                                    }

                                    // The user has declined the oracle license
                                    var abortInstall = function() {
                                        callError( cbFail, "User denied hypervisor installation!", -102 );
                                    }

                                    // Present oracle license
                                    presentOracleLicense( confirmInstall, abortInstall );

                                } else {
                                    callError( cbFail, "User denied hypervisor installation!", -102 );
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

        }

        // Callback for IE-initialized plug-in
        var ieTimeoutTimer, ieConfirmTimer,
            ieLoadCallback = window.__cvmwebapi_ieobject_loadcb = function() {
                // This is called when an IE object is loaded
                clearTimeout(ieTimeoutTimer);
                clearTimeout(ieConfirmTimer);
                validate_plugin();
            },
            ieErrorCallback = window.__cvmwebapi_ieobject_errorcb = function(sMsg) {
                // This is either called by IE when an installation failed, or by the timeout function
                clearTimeout(ieTimeoutTimer);
                clearTimeout(ieConfirmTimer);
                callError( cbFail, "Unable to install/initialize the CernVM Web API!", -101 );
                return;
            };

        // Singleton access to the plugin
        if (__pluginSingleton == null) {

            // Check if we already have an embed element
            __pluginSingleton = document.getElementById('cvmweb-api');

            // Nope, create new instance
            if (!__pluginSingleton) {

                // Use 'object' on IE
                if (BrowserDetect.browser == "Explorer") {

                    // Allocate element
                    var eHost = document.createElement('div');

                    // Create an object inside it
                    eHost.innerHTML = "<object id=\"cvmweb-api\" classid=\"CLSID:3300d921-2cce-5903-85aa-947fda74fb46\" codebase=\"http://cernvm.cern.ch/releases/webapi/plugin/cvmwebapi-1.3.3.cab#version=1,3,3\" width=\"1\" height=\"1\"><param name=\"onload\" value=\"__cvmwebapi_ieobject_loadcb\" /></object>";
                    document.body.appendChild(eHost);

                    // Fetch plugin singleton
                    __pluginSingleton = document.getElementById('cvmweb-api');

                    // Still problems?
                    if (__pluginSingleton == null) {
                        callError( cbFail, "Your browser does not support the <object /> tag!", -101 );
                        return;
                    }

                    // Start the timeout for waiting for plug-in to set-up (Allow 30 sec)
                    ieTimeoutTimer = setTimeout(function() {
                        ieErrorCallback("Timed out");
                    }, 30000);

                    // Unless we immediately get the plug-in up and running (within 1 sec)
                    // inform user that he has to install it manually.
                    ieConfirmTimer = setTimeout(function() {
                        alertFunction(
                                "Follow the on-screen instructions and allow the installation of CernVM WebAPI"
                            );
                    }, 1000);

                } else {

                    __pluginSingleton = document.createElement('embed');

                    // Still problems?
                    if (__pluginSingleton == null) {
                        callError( cbFail, "Your browser does not support the <embed /> tag!", -101 );
                        return;
                    }

                    // Initialize
                    __pluginSingleton.type = "application/x-cvmweb";
                    __pluginSingleton.id = "cvmweb-api";
                    __pluginSingleton.style.width = "1px";
                    __pluginSingleton.style.height = "1px";
                    document.body.appendChild( __pluginSingleton );

                    // Continue with plug-in validation
                    validate_plugin();
                }
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
