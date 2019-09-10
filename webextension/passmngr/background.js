function openPage () {
    let page = {
        type: "detached_panel",
        url: "index.html",
        titlePreface: "passmngr",
        width: 600,
        height: 600
    };

    let panel = browser.windows.create(page);
}

browser.browserAction.onClicked.addListener(openPage);