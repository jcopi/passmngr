if (!browser) var browser = chrome;

// The first thing the UI should do is query the application state from the background script
// on receiving the state, the application will set the UI appropriately.
browser.runtime.sendMessage({"request":appStates.CMD_GET_STATE, data:null}).then((value) => {
    // The response to a get state request should be a state constant
    switch (value) {
    case appStates.NOT_CONFIGURED:
        break;
    case appStates.CONFIGURED:
        break;
    case appStates.LOCKED:
        break;
    case appStates.UNLOCKED:
        break;
    default:
        break;
    }
}, (reason) => {

});