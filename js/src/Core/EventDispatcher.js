
_NS_.EventDispatcher = function(e) {
    this.events = { };
};

/**
 * Fire an event to the registered handlers
 */
_NS_.EventDispatcher.fire = function( name ) {
    var name = arguments.shift();
    window.console.log("Firing ",name," (", arguments, ")");
    if (this.events[name] == undefined) return;
    var callbacks = this.events[name];
    for (var i=0; i<callbacks.length; i++) {
        callbacks[i].apply( this, arguments );
    }
};

/**
 * Register a listener on the given event
 */
_NS_.EventDispatcher.addEventListener = function( name, listener ) {
    if (this.events[name] == undefined) this.events[name]=[];
    this.events[name].push( listener );
};

/**
 * Unregister a listener from the given event
 */
_NS_.EventDispatcher.removeEventListener = function( name, listener ) {
    if (this.events[name] == undefined) return;
    var i = this.events[name].indexOf(listener);
    if (i<0) return;
    this.events.splice(i,1);
};
