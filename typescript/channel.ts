import * as utils from "./utils.js"

export class channel {
    promise: Promise<unknown> = null;
    resolver: (value?: unknown) => void = null;
    rejecter: (reason?: unknown) => void = null;

    socket: WebSocket = null;

    asymmetricKeys: utils.AsymmetricKeyPair = null;
    symmetricKeys: utils.SymmetricKeyPair = null;

    context: ArrayBuffer = null;

    constructor () {}

    async send (message: ArrayBuffer): Promise<ArrayBuffer> {
        // If the channel is currently waiting for another event
        this.promise !== null && await this.promise;

        // Wrap this task in a promise to stop other tasks from interupting it
        this.promise = new Promise(async (resolve, reject) => {
            // Validate the channel has been setup before sending
            if (this.symmetricKeys === null || this.asymmetricKeys === null)
                return reject(null);
            if (!(this.socket instanceof WebSocket) || this.socket.readyState !== this.socket.OPEN)
                return reject(null);

            // Generate a new set of ECDH keys for this client->server, server->client interaction
            let ecdhKeys = await utils.crypto.asymmetric.generateKeyPair();
            if (ecdhKeys === null) return reject(null);

            this.asymmetricKeys = ecdhKeys;
            // Marshall the ecdh public key with the message data
            let messagePayload = await utils.crypto.asymmetric.marshallPublicKey(this.asymmetricKeys, message);

            // Encrypt the message payload with the current symmetric key
            let encryptedPayload = await utils.crypto.symmetric.encrypt(this.symmetricKeys, messagePayload);

            // Send the encrypted payload and wait for a response
            let responsePromise = new Promise((res, rej) => {
                this.resolver = res;
                this.rejecter = rej;

                this.socket.send(encryptedPayload);
            });

            let response: ArrayBuffer = null;
            try {
                response = await responsePromise as ArrayBuffer;
            } catch (ex) {
                reject(null);
            }

            // Decrypt the server response with the current symmetric keys
            let decryptedResponse = await utils.crypto.symmetric.decrypt(this.symmetricKeys, response);
            if (decryptedResponse === null) return reject(null);

            // Unmarshal the server's public key and response
            let [serverKey, responseData] = await utils.crypto.asymmetric.unmarshallPublicKey(decryptedResponse); 

            // Derive the new symmetric keys from the ecdh keys
            let info = new ArrayBuffer(0);
            if (this.context === null) {    
                info = this.context;
            }
            let symmetricKeys = await utils.crypto.asymmetric.deriveSharedSymmetricKeys(this.asymmetricKeys, serverKey, info, false);
            if (symmetricKeys === null) return reject(null);

            this.symmetricKeys = symmetricKeys;

            let contextArr = new Uint8Array(message.byteLength + responseData.byteLength);
            contextArr.set(new Uint8Array(message));
            contextArr.set(new Uint8Array(responseData), message.byteLength);

            this.context = contextArr.buffer;

            return resolve(responseData);
        });

        let result: ArrayBuffer = null;
        try {
            result = await this.promise as ArrayBuffer;
        } catch (ex) {}

        return result;
    }

    // return boolean representing 'successfully opened' 
    async open (): Promise<boolean> {
        this.promise !== null && await this.promise;

        this.promise = new Promise(async (resolve, reject) => {
            // Calling open on an initialized channel will reset everything
            this.asymmetricKeys = null;
            this.symmetricKeys = null;

            // If there is an open websocket already close it
            this.socket instanceof WebSocket && this.socket.close();

            // If there is a resolver/rejecter register call the rejecter then clear both
            this.rejecter !== null && this.rejecter();
            this.resolver = null;
            this.rejecter = null;

            let openPromise = new Promise((res, rej) => {
                this.resolver = res;
                this.rejecter = rej;

                // Open a new socket at the default PassMNGR location
                this.socket = new WebSocket("wss://" + window.location.host + "/socket");
                this.socket.binaryType = "arraybuffer";

                // Register the appropriate event listeners.
                // These will resolve promises through the class' resolver, rejecter members
                this.socket.onopen = this.success.bind(this);
                this.socket.onerror = this.failure.bind(this);
                this.socket.onmessage = this.success.bind(this);
                this.socket.onclose = this.success.bind(this);
            });
            
            // Wait for the websocket to open, handling errors
            try {
                await openPromise;
            } catch (ex) {
                return reject(false);
            }

            // Generate a new set of asymmetric keys for future use
            let asymmetricKeys = await utils.crypto.asymmetric.generateKeyPair();
            if (asymmetricKeys === null) return reject(false);
            this.asymmetricKeys = asymmetricKeys;

            // Marshal the ecdh keys for tranmission to the server
            let payload = await utils.crypto.asymmetric.marshallPublicKey(this.asymmetricKeys, new ArrayBuffer(0));

            // Send the public key to the server and wait for a response
            let responsePromise = new Promise((res, rej) => {
                this.resolver = res;
                this.rejecter = rej;

                this.socket.send(payload);
            });

            let response: ArrayBuffer = null;
            try {
                response = await responsePromise as ArrayBuffer;
            } catch (ex) {
                reject(false);
            }

            let [serverKey, info] = await utils.crypto.asymmetric.unmarshallPublicKey(response);
            
            // This is the first message so there is no message context to integrate into the HKDF
            let symmetricKeys = await utils.crypto.asymmetric.deriveSharedSymmetricKeys(this.asymmetricKeys, serverKey, new ArrayBuffer(0), false);
            if (symmetricKeys === null) return reject(null);

            this.symmetricKeys = symmetricKeys;

            resolve(true);
        });

        let success = false;
        try {
            success = await this.promise as boolean;
        } catch (ex) {}

        return success;
    }

    success (ev: any): void {
        this.resolver !== null && this.resolver(ev);
        this.resolver = null;
        this.rejecter = null;
    }

    failure (ev: any): void {
        this.rejecter !== null && this.rejecter(ev);
        this.resolver = null;
        this.rejecter = null;
    }
}