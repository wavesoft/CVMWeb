var WS_ENDPOINT = "ws://127.0.0.1:1793",
	WS_URI = "cernvm-webapi:";

/**
 * WebAPI Socket handler
 */
WebAPI.Socket = function() {
	WebAPI.EventDispatcher.call(this); // Superclass constructor
	this.connecting = false;
	this.connected = false;
	this.socket = null;
}

/**
 * Subclass event dispatcher
 */
WebAPI.Socket.prototype = Object.create( WebAPI.EventDispatcher.prototype );

/**
 * Cleanup after shutdown/close
 */
WebAPI.Socket.prototype.__handleClose = function() {

}

/**
 * Handle raw incoming data
 */
WebAPI.Socket.prototype.__handleData = function(data) {
	var o = JSON.parse(data);

}

/**
 * Send a JSON frame
 */
WebAPI.Socket.prototype.send = function(action, data) {
	var frame = {
		'action': action,
		'data': data || {}
	};

	// Send JSON Frame
	this.socket.send(JSON.stringify(frame));
}

/**
 * Close connection
 */
WebAPI.Socket.prototype.close = function() {
	if (!this.connected) return;

	// Disconnect
	this.socket.close();
	this.connected = false;

	// Handle disconnection
	this.__handleClose();

}

/**
 * Establish connection
 */
WebAPI.Socket.prototype.connect = function() {
	var self = this;
	if (this.connected) return;

	// Concurrency-check
	if (this.connecting) return;
	this.connecting = true;

	/**
	 * Socket probing function
	 *
	 * This function tries to open a websocket and fires the callback
	 * when a status is known. The first parameter to the callback is a boolean
	 * value, wich defines if the socket could be oppened or not.
	 *
	 * The second parameter is the websocket instance.
	 */
	var probe_socket = function(cb) {
		try {

			// Safari bugfix: When everything else fails
			var timeoutCb = setTimeout(function() {
				cb(false);
			}, 100);

			// Setup websocket & callbacks
			var socket = new WebSocket(WS_ENDPOINT);
			socket.onerror = function(e) {
				clearTimeout(timeoutCb);
				cb(false);
			};
			socket.onopen = function(e) {
				clearTimeout(timeoutCb);
				cb(true, socket);
			};

		} catch(e) {
			console.warn("[socket] Error setting up socket! ",e);
			cb(false);
		}
	};

	/**
	 * Socket check loop
	 */
	var check_loop = function( cb, timeout, _retryDelay, _startTime) {

		// Get current time
		var time = new Date().getTime();
		if (!_startTime) _startTime=time;
		if (!_retryDelay) _retryDelay=50;

		// Calculate how many milliseconds are left until we 
		// reach the timeout.
		var msLeft = timeout - (time - _startTime);

		// Register a callback that will be fired when we reach
		// the timeout defined
		var timeoutTimer = setTimeout(function() {
			cb(false);
		}, msLeft);

		// Setup probe callback
		var probe_cb = function( state, socket ) {
				// Don't fire timeout callback
				if (state) {
					clearTimeout(timeoutTimer);
					cb(true, socket); // We found an open socket
				} else {
					// If we don't have enough time to retry,
					// just wait for the timeoutTimer to kick-in
					if (msLeft < _retryDelay) return;
					// Otherwise clear timeout timer
					clearTimeout(timeoutTimer);
					// And re-schedule websocket poll
					setTimeout(function() {
						check_loop( cb, timeout, _retryDelay, _startTime );
					}, _retryDelay);
				}
			};

		// And send probe
		probe_socket( probe_cb );

	};

	/**
	 * Callback to handle a successful pick of socket
	 */
	var socket_success = function( socket ) {
		console.info("Successfuly contacted CernVM WebAPI");
		self.connecting = false;
		self.connected = true;

		// Bind extra handlers
		self.socket = socket;
		self.socket.onclose = function() {
			console.warn("Remotely disconnected from CernVM WebAPI");
			self.__handleClose();
		};
		self.socket.onmessage = function(e) {
			self.__handleData(e.data);
		};

		// Send handshake
		self.send("handshake", {"version": WebAPI.version});

	};

	/**
	 * Callback to handle a failure to open a socket
	 */
	var socket_failure = function( socket ) {
		console.error("Unable to contact CernVM WebAPI");
		self.connecting = false;
		self.connected = false;
	};

	/**
	 * Callback function from check_loop to initialize
	 * the websocket and to bind it with the rest of the
	 * class instance.
	 */
	var checkloop_cb = function( state, socket ) {

		// Check state
		if (!state) {
			socket_failure();

		} else {
			// We got a socket!
			socket_success(socket);
		}

	};

	// First, check if we can directly contact a socket
	probe_socket(function(state, socket) {
		if (state) {
			// A socket is directly available
			socket_success(socket);
		} else {
			// We ned to do a URL launch
			window.location = WS_URI + "launch";
			check_loop(checkloop_cb, 5000);
		}
	});	

}