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
                return reject();
            if (!(this.socket instanceof WebSocket) || this.socket.readyState !== this.socket.OPEN)
                return reject();

            // Generate a new set of ECDH keys for this client->server, server->client interaction
            let ecdhKeys = await utils.crypto.asymmetric.generateKeyPair();
            if (ecdhKeys === null) return reject();

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

            let response = await responsePromise as ArrayBuffer;

            // Decrypt the server response with the current symmetric keys
            let decryptedResponse = await utils.crypto.symmetric.decrypt(this.symmetricKeys, response);
            if (decryptedResponse === null) return reject();

            // Unmarshal the server's public key and response
            let [serverKey, responseData] = await utils.crypto.asymmetric.unmarshallPublicKey(decryptedResponse); 

            // Derive the new symmetric keys from the ecdh keys
            let symmetricKeys = await utils.crypto.asymmetric.deriveSharedSymmetricKeys(this.asymmetricKeys, serverKey, false);
            if (symmetricKeys === null) return reject();

            this.symmetricKeys = symmetricKeys;

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

            let response = await responsePromise as ArrayBuffer;

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

export class Channel {
    // constants for class initialization
    static NULL_CRYPTO = {
        asymmetric: {
            public: null,
            private: null,
            salt: null
        },
        symmetric: {
            aes: null,
            hmac: null
        },
        context: null
    };

    static AsymmetricSaltLength: number = (512 >> 3);
    
    // This channel implementation uses async and await to provide a synchronous API
    // To provide this the class will maintain the promise, resolver, and rejecter of the most recent operation
    promise: Promise<unknown> = null;
    resolver: (value?: unknown) => void = null;
    rejecter: (reason?: unknown) => void = null;

    // The class will hold the current cryptographic keys and message context used in HKDF
    crypto: {
        asymmetric: {
            public: ArrayBuffer;
            private:CryptoKey;
            salt:   ArrayBuffer
        };
        symmetric: {
            aes: CryptoKey;
            hmac:CryptoKey;
        },
        context: ArrayBuffer;
    } = Channel.NULL_CRYPTO;

    // The class will hold the WebSocket instanced used to communicate with the server
    socket: WebSocket = null;

    constructor () {}

    // Since opening the socket will require performing a hadnshake and all functions will be
    // intended to be used as synchrounous the socket cannot be opened in the constructor
    async Open (): Promise<void> {
        // Always wait for the previous operation to complete
        this.promise !== null && await this.promise;
        // An open socket can't be opened
        this.socket instanceof WebSocket && this.socket.close();

        // Create a new websocket and setup parameters and listeners
        this.promise = new Promise((resolve, reject) => {
            this.resolver = resolve;
            this.rejecter = reject;

            this.socket = new WebSocket("wss://" + window.location.host + "/socket");
            this.socket.binaryType = "arraybuffer";

            this.socket.onopen = this.success.bind(this);
            this.socket.onerror = this.failure.bind(this);
            this.socket.onmessage = this.success.bind(this);
            this.socket.onclose = this.success.bind(this);
        });
        try {
            await this.promise;
        } catch (ex) {
            // The websocket failed to open
            this.Fatal("Websocket failed to open");
            return;
        }
        this.promise = null;

        // request a new set of ECDH Keys. Error checking should be done here,
        // advancing beyond this step is impossible if an error occurs
        try {
            await this.getNewAsymmetricKeys();
        } catch (ex) {
            // This is a fatal error
            this.Fatal("Failed to get asymmetric keys");
            return null;
        }

        // The function needs to create a packet with the new keys
        let payload = this.packPublicKeyData(new ArrayBuffer(0));

        // The function needs to send the payload to the server and wait for a response
        this.promise = new Promise((resolve, reject) => {
            this.resolver = resolve;
            this.rejecter = reject;
            
            this.socket.send(payload);
        });
        let response: MessageEvent = null;
        try {
            response = await this.promise as MessageEvent;
        } catch (ex) {
            // An error occured sending a message to the server, this is a fatal error
            // The server and client cryptographic handshake failed
            this.Fatal("Failed to send message to server");
            return null;
        }
        this.promise = null;

        let [serverPublic, serverSalt] = this.unpackServerPublicKeyData(response.data);
        try {
            await this.updateRatchet(serverPublic, serverSalt);
        } catch (ex) {
            // The ratchet could not be updated, terminate the connection
            this.Fatal("Failed to advance cryptographic ratchet");
            return null;
        }
    }

    async Send (message: ArrayBuffer): Promise<ArrayBuffer> {
        // Always wait for the previous operation to complete
        this.promise !== null && await this.promise;

        // Verify the necessary components are setup
        if (this.crypto.symmetric.aes === null || this.crypto.symmetric.hmac === null) {
            // The symmetric keys aren't setup, something is wrong with the handshake
            // This is a fatal error
            this.Fatal("No encryption keys established");
            return null;
        }

        if (!(this.socket instanceof WebSocket) || this.socket.readyState != this.socket.OPEN) {
            // The websocket is not open, this is a fatal error
            this.Fatal("No websocket was opened");
            return null;
        }

        // To send data the function needs to first request a new set of ECDH Keys
        // Error checking should be done here, advancing beyond this step is impossible if an error occurs
        try {
            await this.getNewAsymmetricKeys();
        } catch (ex) {
            // This is a fatal error
            this.Fatal("Failed to get asymmetric keys");
            return null;
        }

        // The function needs to create a packet with the new keys and the user data
        let payload = this.packPublicKeyData(message);

        // The function needs to encrypt the data with the current encryption keys.
        // Error checking should be done here too.
        let packet: ArrayBuffer = null;
        try {
            packet = await Utils.Crypto.SymmetricEncrypt([this.crypto.symmetric.aes, this.crypto.symmetric.hmac], payload);
        } catch (ex) {
            // Message failed to encrypt, it must not be sent
            // fatal error
            this.Fatal("Failed to encrypt user message");
            return null;
        }

        // The function needs to send the packet to the server and wait for a response
        this.promise = new Promise((resolve, reject) => {
            this.resolver = resolve;
            this.rejecter = reject;
            
            this.socket.send(packet);
        });
        let response: MessageEvent = null;
        try {
            response = await this.promise as MessageEvent;
        } catch (ex) {
            // An error occured sending a message to the server, this is a fatal error
            // The server and client cryptographic ratchet may be out of sync
            this.Fatal("Failed to send message to server");
            return null;
        }
        this.promise = null;

        // The function needs to decrypt the server message
        // The packet from the server will include an Asymmetric public key, and a salt
        // That data waill be used to generate the new current AES Keys
        let decrypted: ArrayBuffer = null;
        try {
            decrypted = await Utils.Crypto.SymmetricDecrypt([this.crypto.symmetric.aes, this.crypto.symmetric.hmac], response.data);
        } catch (ex) {
            // The message failed to decrypt, this could be an attack or an error by the server
            // In either case the connection must be terminated, fatal error.
            this.Fatal("Failed to decrypt server message");
            return null;

        }
        let [serverPublic, serverSalt, serverData] = this.unpackServerPublicKeyData(decrypted);
        try {
            await this.updateRatchet(serverPublic, serverSalt);
        } catch (ex) {
            // The ratchet could not be updated, terminate the connection
            this.Fatal("Failed to advance cryptographic ratchet");
            return null;
        }

        let context = new Uint8Array(payload.byteLength + serverData.byteLength);
        context.set(new Uint8Array(payload));
        context.set(new Uint8Array(serverData), payload.byteLength);
        this.crypto.context = context;

        return serverData;
    }

    async getNewAsymmetricKeys (): Promise<void> {
        [this.crypto.asymmetric.public, this.crypto.asymmetric.private] = await Utils.Crypto.GenerateAsymmetricKeyPair();
        this.crypto.asymmetric.salt = Utils.Crypto.RandomBytes(Channel.AsymmetricSaltLength);
    }

    packPublicKeyData (data: ArrayBuffer): ArrayBuffer {
        let totalLength = 1 + this.crypto.asymmetric.public.byteLength;
        totalLength += 1 + this.crypto.asymmetric.salt.byteLength;
        totalLength += data.byteLength;

        let packed = new Uint8Array(totalLength);
        let i = 0;
        packed[i] = this.crypto.asymmetric.public.byteLength;         i++;
        packed.set(new Uint8Array(this.crypto.asymmetric.public), i); i += this.crypto.asymmetric.public.byteLength;
        packed[i] = this.crypto.asymmetric.salt.byteLength;           i++;
        packed.set(new Uint8Array(this.crypto.asymmetric.salt), i);   i += this.crypto.asymmetric.salt.byteLength;
        packed.set(new Uint8Array(data), i);                          i += data.byteLength;

        return packed;
    }
    unpackServerPublicKeyData (packed: ArrayBuffer): [ArrayBuffer, ArrayBuffer, ArrayBuffer] {
        let buff = new Uint8Array(packed);
        let i = 0;

        let pubLen  = buff[i++];
        let pubKey  = buff.slice(i, (i += pubLen));
        let saltLen = buff[i++];
        let salt    = buff.slice(i, (i += saltLen));
        let data    = buff.slice(i);

        return [pubKey.buffer, salt.buffer, data.buffer];
    }
    async updateRatchet (serverPublic: ArrayBuffer, serverSalt: ArrayBuffer): Promise<void> {
        // Combine the client and server salts
        let clientStart = new Uint8Array(this.crypto.asymmetric.salt).slice(0, this.crypto.asymmetric.salt.byteLength >> 1);
        let clientEnd = new Uint8Array(this.crypto.asymmetric.salt).slice(this.crypto.asymmetric.salt.byteLength >> 1);
        let serverStart = new Uint8Array(serverSalt).slice(0, serverSalt.byteLength >> 1);
        let serverEnd = new Uint8Array(serverSalt).slice(serverSalt.byteLength >> 1);
        // The aes salt is the first half of client and second half of server
        let aessalt = new Uint8Array(clientStart.length + serverEnd.length);
        aessalt.set(clientStart);
        aessalt.set(serverEnd, clientStart.length);
        // The hmac salt is the second half of client and first half of server
        let hmacsalt = new Uint8Array(clientEnd.length + serverStart.length);
        hmacsalt.set(clientEnd);
        hmacsalt.set(serverStart, clientEnd.length);        

        // Import the server public key bytes
        let serverKey = await Utils.Crypto.PublicBytesToRawKey(serverPublic);
        // Compute the shared secret
        let sharedBytes = await Utils.Crypto.DeriveSharedSecret(serverKey, this.crypto.asymmetric.private);
        // Derive a key using hkdf
        // If context isn't set ignore it
        let info = this.crypto.context === null ? new ArrayBuffer(0) : this.crypto.context;
        [this.crypto.symmetric.aes, this.crypto.symmetric.hmac] = await Utils.Crypto.DerivedKeyToSymmetricKeyPair(sharedBytes, aessalt, hmacsalt, info);
    }

    Fatal (reason: string): void {
        this.socket.close();
        this.rejecter !== null && this.rejecter();

        this.crypto = Channel.NULL_CRYPTO;

        throw reason;
    }

    success (ev: any): void {
        if (this.promise === null || this.resolver === null || this.rejecter === null) {
            // This function was called outside of it's intended use case to resolve a promise
            return;
        }

        this.resolver(ev);

        this.resolver = null;
        this.rejecter = null;
        this.promise = null;
    }

    failure (ev: any): void {
        if (this.promise === null || this.resolver === null || this.rejecter === null) {
            // This function was called outside of it's intended use case to resolve a promise
            return;
        }

        this.rejecter(ev);

        this.resolver = null;
        this.rejecter = null;
        this.promise = null;
    }
}