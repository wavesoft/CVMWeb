var WS_ENDPOINT = "ws://127.0.0.1:1793",
	WS_URI = "cernvm-webapi:";

/**
 * WebAPI Socket handler
 */
WebAPI.Socket = function() {

	// Superclass constructor
	WebAPI.EventDispatcher.call(this);

	// Status flags
	this.connecting = false;
	this.connected = false;
	this.socket = null;

	// Event ID and callback tracking
	this.lastID = 0;
	this.responseCallbacks = {};

	// Parse authentication token from URL hash
	this.authToken = "";
	if (window.location.hash)
		this.authToken = window.location.hash.substr(1);

}

/**
 * Subclass event dispatcher
 */
WebAPI.Socket.prototype = Object.create( WebAPI.EventDispatcher.prototype );

/**
 * Cleanup after shutdown/close
 */
WebAPI.Socket.prototype.__handleClose = function() {
	this.__fire("disconnected");
}

/**
 * Handle connection acknowlegement
 */
WebAPI.Socket.prototype.__handleOpen = function(data) {
	this.__fire("connected", data['version']);
}

/**
 * Handle raw incoming data
 */
WebAPI.Socket.prototype.__handleData = function(data) {
	var o = JSON.parse(data);

	// Fire callbacks if there is an ID
	if (o['id'] != undefined) {
		var cb = this.responseCallbacks[o['id']];
		if (cb != undefined) cb(o);
	} 

	// Fire event if we got an event response
	else if (o['type'] == "event") {
		this.__fire(o['name'], o['data']);
	}

}

/**
 * Send a JSON frame
 */
WebAPI.Socket.prototype.send = function(action, data, responseCallback, responseTimeout) {
	var self = this;

	// Calculate next frame's ID
	var frameID = "a-" + (++this.lastID);
	var frame = {
		'type': 'action',
		'name': action,
		'id': frameID,
		'data': data || { }
	};

	// Register response callback
	if (responseCallback) {
		var timeoutTimer = null;

		// Register a timeout timer
		// unless the responseTimeout is set to 0
		if (responseTimeout !== 0) {
			timeoutTimer = setTimeout(function() {

				// Remove slot
				delete self.responseCallbacks[frameID];

				// Respond error
				responseCallback(null, "error", {"error": "Response timeout"});

			}, responseTimeout || 10000);
		}

		// Register a callback for the frame of the given type
		this.responseCallbacks[frameID] = function(data) {
			if (timeoutTimer!=null) clearTimeout(timeoutTimer);
			delete self.responseCallbacks[frameID];
			responseCallback(data['data'] || { }, data['type'], data);
		};
	}

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

		// Send handshake and wait for response
		// to finalize the connection
		self.send("handshake", {
			"version": WebAPI.version,
			"auth": self.authToken
		}, function(data, type, raw) {
			console.info("Successfuly contacted with CernVM WebAPI v" + data['version']);
			self.__handleOpen(data);
		});

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