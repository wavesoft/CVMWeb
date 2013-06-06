
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
    window.ba = __pluginSingleton;
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
        return (plugin.version != undefined);
    },
    
    /**
     * Static function to check if the hypervisor is there
     */
    hasHypervisor: function() {
        var plugin = getPlugin();
        return (plugin.version != undefined) && (plugin.hypervisorName != "");
    }
    
};

var CVMWebAPI = GLOBAL.CVMWebAPI = function( config ) {
    var cfg = ( config == undefined) ? { } : config;

    // Create instance if the plugin is not instantiated
    if (!cfg.plugin) {
    } else if (typeof(cfg.plugin) == 'string') {
        cfg.plugin = document.getElementById(cfg.plugin);
    }

    // Collect info
    this.pluginInstalled = (cfg.plugin.version != undefined);
    this.pluginVersion = cfg.plugin.version;
    this.hvInstalled = (cfg.plugin.hypervisorName != "");
    this.hvVersion = cfg.plugin.hypervisorVersion;
    this.hvName = cfg.plugin.hypervisorName;

    // Store handles
    this.plugin = cfg.plugin;

    // Events stack
    this.events = { };

    // If we are valid, fetch the daemon instance
    this.daemon = null;
    if (this.pluginInstalled) {
    
        // Request daemon access
        this.daemon = this.plugin.requestDaemonAccess();
    }

};

function fireEvent() {
    var name = arguments.shift();
    if (CVMWebApi.events[name] == undefined) return;
    var callbacks = CVMWebApi.events[name];
    callbacks.apply( CVMWebApi, arguments );
}

CVMWebAPI.prototype.addEventListener = function(name, handler) {
    if (this.events[name] == undefined) this.events[name] = [ ];
    this.events[name].push( handler );
};

CVMWebAPI.prototype.removeEventListener = function(name, handler) {
    if (this.events[name] == undefined) return;
    var i = this.events[name].indexOf(handler);
    if (i>=0) this.events[name].splice(i,1);
};

CVMWebAPI.prototype.installHypervisor = function() {
    if (this.hvInstalled) return false;
    this.plugin.installHypervisor();
    return true;
};

CVMWebAPI.prototype.requestSession = function( name, key, callback ) {
    if (!this.hvInstalled) return false;
    if (!this.pluginInstalled) return false;
    var error = this.plugin.requestSession(name,key,callback,function(errCode){
    
    });
    return true;
};

CVMWebAPI.prototype.startDaemon = function( ) {
    if (this.daemon == null) return;
    if (this.daemon.running) return;
    return this.daemon.start();  
};

CVMWebAPI.prototype.stopDaemon = function( ) {
    if (this.daemon == null) return;
    if (!this.daemon.running) return;
    return this.daemon.stop();  
};
