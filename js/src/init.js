
/**
 * This file is always included last in the build chain. 
 * Here we do the static initialization of the plugin
 */
 
 
/**
* Hook on the onLoad event
* (I can't use jQuery because it's supposed to be a non-dependant library)
*/
window.addEventListener('load', function(e) {
    __pageLoaded = true;
    for (var i=0; i<__loadHooks.length; i++) {
        __loadHooks[i]();
    }
});