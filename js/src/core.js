
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
     * Note: Check 
     * 
     */
    requetSession: function( configURL, callback ) {
        var plugin = getPlugin();
        if (!plugin) return false;
        if (!config) return false;
        if (!callback) return false;
        
    }
        
};
