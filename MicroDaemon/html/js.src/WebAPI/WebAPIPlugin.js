
/**
 * WebAPI Socket handler
 */
_NS_.WebAPIPlugin = function() {

	// Superclass constructor
	_NS_.Socket.call(this);

}

/**
 * Subclass event dispatcher
 */
_NS_.WebAPIPlugin.prototype = Object.create( _NS_.Socket.prototype );
