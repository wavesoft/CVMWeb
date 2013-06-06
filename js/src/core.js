(function(glob){
    
    var CVMWebAPI = glob.CVMWebAPI = function( config ) {
        var cfg = ( config == undefined) ? { } : config;
    
        // Create instance if the plugin is not instantiated
        if (!cfg.plugin) {
            cfg.plugin = document.createElement('embed');
            cfg.plugin.type = "application/x-cvmweb";
            cfg.plugin.style.width = "1px";
            cfg.plugin.style.height = "1px";
            document.body.appendChild( cfg.plugin );
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
    
})(window);
