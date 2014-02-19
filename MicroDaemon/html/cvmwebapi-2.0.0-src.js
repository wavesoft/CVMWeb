window.CVM={'version':'2.0.0'};(function(_NS_) {
/**
 * Private variables
 */
var __pluginSingleton = null,
    __pageLoaded = false,
    __loadHooks = [];

/**
 * Core namespace
 */
_NS_.debugLogging = true;
_NS_.version = '1.0'

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
 * @param unused  			[Optional] Provided for backwards compatibility. We ALWAYS setup the environment
 */
_NS_.startCVMWebAPI = function( cbOK, cbFail, unused ) {

	// Function that actually does what we want
	var fn_start = function() {

		// Create a CernVM WebAPI Plugin Instance
		var instance = new _NS_.WebAPIPlugin();

		// Connect and wait for status
		instance.connect(function( hasAPI ) {
			if (hasAPI) {

				// We do have an API and we have a connection,
				// there will be more progress events.
				cbOK( instance );

			} else {

				// There is no API, ask the user to install the plug-in
				var cFrame = document.createElement('iframe');
				cFrame.src = "//cernvm-online.cern.ch";
				cFrame.width = "100%";
				cFrame.height = 400;
				cFrame.frameBorder = 0;

				// Show frame
				UserInteraction.createFramedWindow( cFrame );

			}
		});

	};

    // If the page is still loading, request an onload hook,
    // otherwise proceed with verification
    if (!__pageLoaded) {
        __loadHooks.push( fn_start );
    } else {
        fn_start();
    }

};
_NS_.EventDispatcher = function(e) {
    this.events = { };
};

/**
 * Fire an event to the registered handlers
 */
_NS_.EventDispatcher.prototype.__fire = function( name, args ) {
    if (_NS_.debugLogging) console.log("Firing",name,"(", args, ")");
    if (this.events[name] == undefined) return;
    var callbacks = this.events[name];
    for (var i=0; i<callbacks.length; i++) {
        callbacks[i].apply( this, args );
    }
};

/**
 * Register a listener on the given event
 */
_NS_.EventDispatcher.prototype.addEventListener = function( name, listener ) {
    if (this.events[name] == undefined) this.events[name]=[];
    this.events[name].push( listener );
};

/**
 * Unregister a listener from the given event
 */
_NS_.EventDispatcher.prototype.removeEventListener = function( name, listener ) {
    if (this.events[name] == undefined) return;
    var i = this.events[name].indexOf(listener);
    if (i<0) return;
    this.events.splice(i,1);
};

/**
 * WebAPI Socket handler
 */
_NS_.ProgressFeedback = function() {
	
};
var WS_ENDPOINT = "ws://127.0.0.1:1793",
	WS_URI = "cernvm-webapi:";

/**
 * WebAPI Socket handler
 */
_NS_.Socket = function() {

	// Superclass constructor
	_NS_.EventDispatcher.call(this);

	// The user interaction handler
	this.interaction = new UserInteraction(this);

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
_NS_.Socket.prototype = Object.create( _NS_.EventDispatcher.prototype );

/**
 * Cleanup after shutdown/close
 */
_NS_.Socket.prototype.__handleClose = function() {

	// Fire the disconnected event
	this.__fire("disconnected");

	// Hide any active user interaction - it's now useless
	UserInteraction.hideInteraction();

}

/**
 * Handle connection acknowlegement
 */
_NS_.Socket.prototype.__handleOpen = function(data) {
	this.__fire("connected", data['version']);
}

/**
 * Handle raw incoming data
 */
_NS_.Socket.prototype.__handleData = function(data) {
	var o = JSON.parse(data);

	// Forward all the frames of the given ID to the
	// frame-handling callback.
	if (o['id']) {
		var cb = this.responseCallbacks[o['id']];
		if (cb != undefined) cb(o);
	}

	// Fire event if we got an event response
	else if (o['type'] == "event") {
		var data = o['data'];

		// [Event] User Interaction
		if (o['name'] == "interact") {
			// Forward to user interaction controller
			this.interaction.handleInteractionEvent(o['data']);

		} else {
			this.__fire(o['name'], o['data']);

		}

	}

}

/**
 * Send a JSON frame
 */
_NS_.Socket.prototype.send = function(action, data, responseEvents, responseTimeout) {
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
	if (responseEvents) {
		var timeoutTimer = null,
			eventify = function(name) {
				if (!name) return "";
				return "on" + name[0].toUpperCase() + name.substr(1);
			}

		// Register a timeout timer
		// unless the responseTimeout is set to 0
		if (responseTimeout !== 0) {
			timeoutTimer = setTimeout(function() {

				// Remove slot
				delete self.responseCallbacks[frameID];

				// Send error event
				if (responseEvents['onError'])
					responseEvents['onError']("Response timeout");

			}, responseTimeout || 10000);
		}

		// Register a callback that will be fired when we receive
		// a frame with the specified ID.
		this.responseCallbacks[frameID] = function(data) {

			// We got a response, reset timeout timer
			if (timeoutTimer!=null) clearTimeout(timeoutTimer);

			// Cleanup when we received a 'succeed' frame
			if ((data['name'] == 'succeed') || (data['name'] == 'failed'))
				delete self.responseCallbacks[frameID];

			// Pick and fire the appropriate event response
			var evName = eventify(data['name']);
			if (responseEvents[evName]) {

				// Fire the function with the array list as arguments
				responseEvents[evName].apply(
						self, data['data'] || []
					);

			}

		};
	}

	// Send JSON Frame
	this.socket.send(JSON.stringify(frame));
}

/**
 * Close connection
 */
_NS_.Socket.prototype.close = function() {
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
_NS_.Socket.prototype.connect = function( cbAPIState ) {
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
				if (!self.connecting) return;
				cb(false);
			};
			socket.onopen = function(e) {
				clearTimeout(timeoutCb);
				if (!self.connecting) return;
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
			"version": _NS_.version,
			"auth": self.authToken
		}, function(data, type, raw) {
			console.info("Successfuly contacted with CernVM WebAPI v" + data['version']);
			self.__handleOpen(data);
		});

		// We managed to connect, we do have an API installed
		if (cbAPIState) cbAPIState(true);

	};

	/**
	 * Callback to handle a failure to open a socket
	 */
	var socket_failure = function( socket ) {
		console.error("Unable to contact CernVM WebAPI");
		self.connecting = false;
		self.connected = false;

		// We could not connect nor launch the plug-in, therefore
		// we are missing API components.
		if (cbAPIState) cbAPIState(false);

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
			var e = document.createElement('iframe'); 
			e.style.display="none"; 
			e.src = WS_URI + "launch";
			document.body.appendChild(e);

			// And start loop for 5 sec
			check_loop(checkloop_cb, 5000);
		}
	});	

}
/**
 * Flags for the UserInteraction
 */
var UI_OK 			= 0x01,
	UI_CANCEL 		= 0x02,
	UI_NOTAGAIN		= 0x100;

/**
 * The private WebAPI Interaction class
 */
var UserInteraction = _NS_.UserInteraction = function( socket ) {
	this.socket = socket;
};


/**
 * Hide the active interaction screen
 */
UserInteraction.hideInteraction = function() {
	if (UserInteraction.activeScreen) {
		document.body.removeChild(UserInteraction.activeScreen);
		UserInteraction.activeScreen = null;
	}
}

/**
 * Create a framed button
 */
UserInteraction.createButton = function( title, baseColor ) {
	var button = document.createElement('button');

	// Place tittle
	button.innerHTML = title;

	// Style button
	button.style.display = 'inline-block';
	button.style.marginBottom = '0';
	button.style.textAlign = 'center';
	button.style.verticalAlign = 'middle';
	button.style.borderStyle = 'solid';
	button.style.borderWidth = '1px';
	button.style.borderRadius 
		= button.style.webkitBorderRadius 
		= button.style.mozBorderRadius 
		= "4px";
	button.style.userSelect 
		= button.style.webkitUserSelect 
		= button.style.mozUserSelect 
		= button.style.msUserSelect 
		= "none";
	button.style.margin = '5px';
	button.style.padding = '6px 12px';
	button.style.cursor = 'pointer';

	// Setup color
	var shadeColor = function(color, percent) {
			var num = parseInt(color.slice(1),16), amt = Math.round(2.55 * percent), R = (num >> 16) + amt, G = (num >> 8 & 0x00FF) + amt, B = (num & 0x0000FF) + amt;
			return "#" + (0x1000000 + (R<255?R<1?0:R:255)*0x10000 + (G<255?G<1?0:G:255)*0x100 + (B<255?B<1?0:B:255)).toString(16).slice(1);
		},
		yiqColor = function (bgColor) {
			var num = parseInt(bgColor.slice(1), 16),
				r = (num >> 16), g = (num >> 8 & 0x00FF), b = (num & 0x0000FF),
				yiq = (r * 299 + g * 587 + b * 114) / 1000;
			return (yiq >= 128) ? 'black' : 'white';
		};

	// Lighten for background
	button.style.backgroundColor = baseColor;
	button.style.borderColor = shadeColor(baseColor, -20);

	// Hover
	button.onmouseover = function() {
		button.style.backgroundColor = shadeColor(baseColor, -10);
	}
	button.onmouseout = function() {
		button.style.backgroundColor = baseColor;
	}

	// Pick foreground color according to the intensity of the background
	button.style.color = yiqColor( baseColor );

	// Return button
	return button;

}

/**
 * Create a framed window, used for various reasons
 */
UserInteraction.createFramedWindow = function( body, header, footer, cbClose ) {
	var floater = document.createElement('div'),
		content = document.createElement('div'),
		cHeader = document.createElement('div'),
		cFooter = document.createElement('div'),
		cBody = document.createElement('div');

	// Make floater full-screen overlay
	floater.style.position = "absolute";
	floater.style.left = "0";
	floater.style.top = "0";
	floater.style.right = "0";
	floater.style.bottom = "0";
	floater.style.zIndex = 60000;
	floater.style.backgroundColor = "rgba(255,255,255,0.8)";
	floater.appendChild(content);

	// Prepare vertical-centering
	content.style.marginLeft = "auto";
	content.style.marginRight = "auto";
	content.style.marginBottom = 0;
	content.style.marginTop = 0;

	// Frame style
	content.style.backgroundColor = "#FCFCFC";
	content.style.border = "solid 1px #E6E6E6";
	content.style.borderRadius 
		= content.style.webkitBorderRadius 
		= content.style.mozBorderRadius 
		= "5px";
	content.style.boxShadow 
		= content.style.webkitBoxShadow 
		= content.style.mozBoxShadow 
		= "1px 2px 4px 1px rgba(0,0,0,0.2)";
	content.style.padding = "10px";
	content.style.fontFamily = "Verdana, Geneva, sans-serif";
	content.style.fontSize = "14px";
	content.style.color = "#666;"
	content.style.width = "70%";

	// Style header
	cHeader.style.color = "#333"

	// Style footer
	cFooter.style.textAlign = "center";
	cFooter.style.color = "#333"

	// Append header
	content.appendChild(cHeader);
	if (header) {
		if (typeof(header) == "string") {
			cHeader.innerHTML = header;
			cHeader.style.fontSize = "1.6em";
			cHeader.style.marginBottom = "8px";
		} else {
			cHeader.appendChild(header);
		}
	}

	// Append body
	if (body) cBody.appendChild(body);
	content.appendChild(cBody);

	// Append footer
	content.appendChild(cFooter);
	if (footer) {
		if (typeof(footer) == "string") {
			cFooter.innerHTML = footer;
		} else {
			cFooter.appendChild(footer);
		}
	}

	// Update vertical-centering information
	var updateMargin = function() {
		var top = (window.innerHeight-content.clientHeight)/2;
		if (top < 0) top = 0;
		content.style.marginTop = top + "px";
	}

	// Close when clicking the floater
	floater.onclick = function() {
		if (cbClose) {
			cbClose();
		} else {
			UserInteraction.hideInteraction();
		}
	}

	// Stop propagation in content
	content.onclick = function(event) {
		event.stopPropagation();
	}

	// Remove previous element
	UserInteraction.hideInteraction();
	UserInteraction.activeScreen = floater;

	// Append element in the body
	document.body.appendChild(floater);
	updateMargin();

	// Return root element
	return floater;

}

/**
 * Create a license window
 */
UserInteraction.displayLicenseWindow = function( title, body, isURL, cbAccept, cbDecline ) {
	var cControls = document.createElement('div'),
		lnkSpacer = document.createElement('span'),
		cBody;

	// Prepare elements
	lnkSpacer.innerHTML = "&nbsp;";

	// Prepare iFrame or div depending on if we have URL or body
	if (isURL) {
		cBody = document.createElement('iframe'),
		cBody.src = body;
		cBody.width = "100%";
		cBody.height = 450;
		cBody.frameBorder = 0;
	} else {
		cBody = document.createElement('div');
		cBody.width = "100%";
		cBody.style.height = '450px';
		cBody.style.display = 'block';
		cBody.innerHTML = body;
	}

	// Prepare buttons
	var	lnkOk = UserInteraction.createButton('Accept License', '#E1E1E1');
		lnkCancel = UserInteraction.createButton('Decline License', '#FAFAFA');

	// Style controls
	cControls.style.padding = '6px';
	cControls.appendChild(lnkOk);
	cControls.appendChild(lnkSpacer);
	cControls.appendChild(lnkCancel);

	// Create framed window
	var elm;
	elm = UserInteraction.createFramedWindow( cBody, title, cControls, function() {
	   document.body.removeChild(elm);
	   if (cbDecline) cbDecline();
	});

	// Bind link callbacks
	lnkOk.onclick = function() {
	   document.body.removeChild(elm);
	   if (cbAccept) cbAccept();
	};
	lnkCancel.onclick = function() {
	   document.body.removeChild(elm);
	   if (cbDecline) cbDecline();
	};
	
}

/** 
 * Confirm function
 */
UserInteraction.confirm = function( title, body, callback ) {
	var cBody = document.createElement('div'),
		cButtons = document.createElement('div');

	// Prepare body
	cBody.innerHTML = body;
	cBody.style.width = '100%';

	// Prepare buttons
	var	win,
		lnkOk = UserInteraction.createButton('Ok', '#E1E1E1'),
		lnkCancel = UserInteraction.createButton('Cancel', '#FAFAFA');

	lnkOk.onclick = function() {
		document.body.removeChild(win);
		callback(true);
	};
	lnkCancel.onclick = function() {
		document.body.removeChild(win);
		callback(false);
	};

	// Nest
	cButtons.appendChild(lnkOk);
	cButtons.appendChild(lnkCancel);

	// Display window
	win = UserInteraction.createFramedWindow( cBody, title, cButtons, function() {
		document.body.removeChild(win);
		callback(false);
	});

}

/** 
 * Alert function
 */
UserInteraction.alert = function( title, body, callback ) {
	var cBody = document.createElement('div'),
		cButtons = document.createElement('div');

	// Prepare body
	cBody.innerHTML = body;
	cBody.style.width = '100%';

	// Prepare button
	var win, lnkOk = UserInteraction.createButton('Ok', '#FAFAFA');
	lnkOk.onclick = function() {
		document.body.removeChild(win);
	};
	cButtons.appendChild(lnkOk);

	// Display window
	win = UserInteraction.createFramedWindow( cBody, title, cButtons );

}

/** 
 * License confirm (by buffer) function
 */
UserInteraction.confirmLicense = function( title, body, callback ) {
	UserInteraction.displayLicenseWindow(title, body, false, function(){
		callback(true);
	}, function() {
		callback(false);
	});
}

/** 
 * License confirm (by URL) function
 */
UserInteraction.confirmLicenseURL = function( title, url, callback ) {
	UserInteraction.displayLicenseWindow(title, url, true, function(){
		callback(true);
	}, function() {
		callback(false);
	});
}

/**
 * Handle interaction event
 */
UserInteraction.prototype.handleInteractionEvent = function( data ) {
	var socket = this.socket;

	// Confirmation window
	if (data[0] == 'confirm') {

		// Fire the confirmation function
		UserInteraction.confirm( data[1], data[2], function(result, notagain) {

			// Send back interaction callback response
			if (result) {
				socket.send("interactionCallback", {"result": UI_OK | (notagain ? UI_NOTAGAIN : 0) });
			} else {
				socket.send("interactionCallback", {"result": UI_CANCEL | (notagain ? UI_NOTAGAIN : 0) });
			}

		});

	}

	// Alert window
	else if (data[0] == 'alert') {

		// Fire the confirmation function
		UserInteraction.alert( data[1], data[2], function(result) { });

	}

	// License confirmation with buffer
	else if (data[0] == 'confirmLicense') {

		// Fire the confirmation function
		UserInteraction.confirmLicense( data[1], data[2], function(result, notagain) {

			// Send back interaction callback response
			if (result) {
				socket.send("interactionCallback", {"result": UI_OK | (notagain ? UI_NOTAGAIN : 0) });
			} else {
				socket.send("interactionCallback", {"result": UI_CANCEL | (notagain ? UI_NOTAGAIN : 0) });
			}

		});

	}

	// License confirmation with URL
	else if (data[0] == 'confirmLicenseURL') {

		// Fire the confirmation function
		UserInteraction.confirmLicenseURL( data[1], data[2], function(result, notagain) {

			// Send back interaction callback response
			if (result) {
				socket.send("interactionCallback", {"result": UI_OK | (notagain ? UI_NOTAGAIN : 0) });
			} else {
				socket.send("interactionCallback", {"result": UI_CANCEL | (notagain ? UI_NOTAGAIN : 0) });
			}

		});

	}


}
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

/**
 * Open a session and call the cbOk when ready
 */
_NS_.WebAPIPlugin.prototype.requestSession = function(vmcp, cbOk, cbFail) {
	var self = this;

	// Send requestSession
	this.send("requestSession", {
		"vmcp": vmcp
	}, {
		onSucceed : function( msg, session_id ) {

			// Create a new session object
			var session = new _NS_.WebAPISession( self, session_id );

			// Receive events with id=session_id
			self.responseCallbacks[session_id] = function(data) {
				session.handleEvent(data);
			}

			// Fire the ok callback
			if (cbOk) cbOk(session);

		},
		onFailed: function( msg, code ) {

			// Fire the failed callback
			if (cbFail) cbFail(msg, code);

		},
		onProgress: function( msg, percent ) {
			self.__fire("progress", [msg, percent]);
		},
		onStarted: function( msg ) {
			self.__fire("started", [msg]);
		},
		onCompleted: function( msg ) {
			self.__fire("completed", [msg]);
		}
	});


};
/**
 * WebAPI Socket handler
 */
_NS_.WebAPISession = function( socket, session_id ) {

	// Superclass initialize
	_NS_.EventDispatcher.call(this);

	// Keep references
	this.socket = socket;
	this.session_id = session_id;

}

/**
 * Subclass event dispatcher
 */
_NS_.WebAPISession.prototype = Object.create( _NS_.EventDispatcher.prototype );

/**
 * Handle incoming event
 */
_NS_.WebAPISession.prototype.handleEvent = function(data) {
	this.__fire(data['name'], data['data']);
}

_NS_.WebAPISession.prototype.start = function( values ) {
	// Send a start message
	this.socket.send("start", {
		"session_id": this.session_id,
		"parameters": values || { }
	})
}

_NS_.WebAPISession.prototype.stop = function() {
	// Send a stop message
	this.socket.send("stop", {
		"session_id": this.session_id
	})
}

_NS_.WebAPISession.prototype.pause = function() {
	// Send a pause message
	this.socket.send("pause", {
		"session_id": this.session_id
	})
}

_NS_.WebAPISession.prototype.resume = function() {
	// Send a resume message
	this.socket.send("resume", {
		"session_id": this.session_id
	})
}

_NS_.WebAPISession.prototype.reset = function() {
	// Send a reset message
	this.socket.send("reset", {
		"session_id": this.session_id
	})
}

_NS_.WebAPISession.prototype.hibernate = function() {
	// Send a hibernate message
	this.socket.send("hibernate", {
		"session_id": this.session_id
	})
}

_NS_.WebAPISession.prototype.close = function() {
	// Send a close message
	this.socket.send("close", {
		"session_id": this.session_id
	})
}

/**
 * This file is always included last in the build chain. 
 * Here we do the static initialization of the plugin
 */
 
/**
* By default use 'load' handler, unless user has jQuery loaded
*/
if (window['jQuery'] == undefined) {
	if (__pageLoaded) return;
	window.addEventListener('load', function(e) {
	    __pageLoaded = true;
	    for (var i=0; i<__loadHooks.length; i++) {
	        __loadHooks[i]();
	    }
	});
} else {
	jQuery(function(){
		if (__pageLoaded) return;
		__pageLoaded = true;
	    for (var i=0; i<__loadHooks.length; i++) {
	        __loadHooks[i]();
	    }
	});
}

})(window.CVM);
