class strings {
    static toUTF8 (string) {
        // Input validation
        if (typeof string !== "string") {
            throw "Invalid input. 1st argument of strings.toUTF8 must be a string";
        }
        let tmp = [];
        for (let i = 0, j = 0; i < string.length; ++i) {
            let cp = string.codePointAt(i);
            if (cp <= 0x0000007F) {
                tmp.push(cp);
            } else if (cp <= 0x000007FF) {
                tmp.push(0xC0 | ((cp & 0x7C0) >> 6));
                tmp.push(0x80 | (cp & 0x3F))
            } else if (cp <= 0x0000FFFF) {
                tmp.push(0xE0 | ((cp & 0xF000) >> 12));
                tmp.push(0x80 | ((cp & 0xFC0) >> 6));
                tmp.push(0x80 | (cp & 0x3F));
            } else if (cp <= 0x001FFFFF) {
                tmp.push(0xF0 | ((cp & 0x1C0000) >> 18));
                tmp.push(0x80 | ((cp & 0x3F000) >> 12));
                tmp.push(0x80 | ((cp & 0xFC0) >> 6));
                tmp.push(0x80 | (cp & 0x3F));
            }
        }
        let bytes = new Uint8Array(tmp);
        return bytes.buffer;
    };
    static fromUTF8 (bytes) {
        // Input validation
        if (!(bytes instanceof ArrayBuffer || ArrayBuffer.isView(bytes))) {
            throw "Invalid input. 1st argument of string.fromUTF8 must be an ArrayBuffer";
        }
        let result = "";
        let u8 = new Uint8Array(buffer);
        for (let i = 0; i < u8.length; ++i) {
            if (u8[i] <= 0x7F) {
                result += String.fromCodePoint(u8[i]);
            } else if ((u8[i] >> 5) == 0x6) {
                // The next bytes should be part of this Unicode Code Point
                if (u8.length < (i + 2))
                    break;
                if ((u8[i+1] >> 6) == 0x2) {
                    let cp = ((u8[i] & 0x1F) << 6) | (u8[i+1] & 0x3F);
                    result += String.fromCodePoint(cp);
                    ++i;
                }
            } else if ((u8[i] >> 4) == 0xE) {
                // The next bytes should be part of this Unicode Code Point
                if (u8.length < (i + 3))
                    break;
                if (((u8[i+1] >> 6) == 0x2) && ((u8[i+2] >> 6) == 0x2)) {
                    let cp = ((u8[i] & 0xF) << 12) | ((u8[i+1] & 0x3F) << 6) | (u8[i+2] & 0x3F);
                    result += String.fromCodePoint(cp);
                    i += 2;
                }
            } else if ((u8[i] >> 3) == 0xF) {
                // The next bytes should be part of this Unicode Code Point
                if (u8.length < (i + 4))
                    break;
                if (((u8[i+1] >> 6) == 0x2) && ((u8[i+2] >> 6) == 0x2) && ((u8[i+3] >> 6) == 0x2)) {
                    let cp = ((u8[i] & 0x7) << 18) | ((u8[i+1] & 0x3F) << 12) | ((u8[i+2] & 0x3F) << 6) | (u8[i+3] & 0x3F);
                    result += String.fromCodePoint(cp);
                    i += 3;
                }
            }
        }
        return result;
    };
};

class bytes {
    static toBase64 (bytes) {
        // Input Validation
        if (!(bytes instanceof ArrayBuffer || ArrayBuffer.isView(bytes))) {
            throw "Invalid input. 1st argument of bytes.toBase64 must be an ArrayBuffer";
        }
        let result = "";
        let encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        let padding  = "=";

        let u8 = new Uint8Array(buffer);
        let remainder = u8.length % 3;
        let mainlen = u8.length - remainder;

        for (let i = 0; i < mainlen; i += 3) {
            let word = (u8[i] << 16) | (u8[i+1] << 8) | u8[i+2];
            result += encoding[(word & 0b111111000000000000000000) >> 18];
            result += encoding[(word & 0b000000111111000000000000) >> 12];
            result += encoding[(word & 0b000000000000111111000000) >>  6];
            result += encoding[(word & 0b000000000000000000111111)];
        }
        if (remainder == 2) {
            let word = (u8[mainlen] << 8) | u8[mainlen+1];
            result += encoding[(word & 0b1111110000000000) >> 10];
            result += encoding[(word & 0b0000001111110000) >>  4];
            result += encoding[(word & 0b0000000000001111) <<  2];
            result += padding;
        } else if (remainder == 1) {
            let word = u8[mainlen];
            result += encoding[(word & 0b11111100) >> 2];
            result += encoding[(word & 0b00000011) << 4];
            result += padding;
            result += padding;
        }

        return result;
    };
    static fromBase64 (string) {
        // Input validation
        if (typeof string !== "string") {
            throw "Invalid input. 1st argument of bytes.fromBase64 must be a string";
        }
        let encodings = {"0":52,"1":53,"2":54,"3":55,"4":56,"5":57,"6":58,"7":59,"8":60,"9":61,"A":0,"B":1,"C":2,"D":3,"E":4,"F":5,
        "G":6,"H":7,"I":8,"J":9,"K":10,"L":11,"M":12,"N":13,"O":14,"P":15,"Q":16,"R":17,"S":18,"T":19,"U":20,"V":21,"W":22,"X":23,"Y":24,"Z":25,"a":26,"b":27,"c":28,"d":29,"e":30,"f":31,"g":32,"h":33,"i":34,"j":35,"k":36,"l":37,"m":38,"n":39,"o":40,
        "p":41,"q":42,"r":43,"s":44,"t":45,"u":46,"v":47,"w":48,"x":49,"y":50,"z":51,"+":62,"/":63};
    
        let remainder = string.length % 4;
        if (remainder > 0)
            throw "Invalid padding on base64 string";
        let lastChunkBCnt = (string.endsWith("=") ? (string.endsWith("==") ? 1 : 2) : 0);
        let mainlen = string.length - (lastChunkBCnt === 0 ? 0 : 4);
        let nbytes = ((mainlen / 4) * 3) + lastChunkBCnt;
        let bytes = new Uint8Array(nbytes);

        let j = 0;
        for (let i = 0; i < mainlen; i += 4) {
            let word = (encodings[string[i]] << 18) | (encodings[string[i+1]] << 12) | (encodings[string[i+2]] << 6) | encodings[string[i+3]];
            bytes[j++] = (word & 0b111111110000000000000000) >> 16;
            bytes[j++] = (word & 0b000000001111111100000000) >> 8;
            bytes[j++] = (word & 0b000000000000000011111111);
        }

        if (lastChunkBCnt === 1) {
            let word = (encodings[string[mainlen]] << 2) | (encodings[string[mainlen+1]] >> 4);
            bytes[j++] = word;
        } else if (lastChunkBCnt === 2) {
            let word = (encodings[string[mainlen]] << 10) | (encodings[string[mainlen+1]] << 4) | (encodings[string[mainlen+2]] >>2);
            bytes[j++] = (word & 0b1111111100000000) >> 8;
            bytes[j++] = (word & 0b0000000011111111);
        }

        return bytes.buffer;
    };
};

class encryption {
    static isKeypair(keys) {
        return "aes" in keys && "hmac" in keys && keys.aes instanceof CryptoKey && keys.hmac instanceof CryptoKey;
    }
    static async sign (keys, data) {
        // Validate the type of the inputs
        if (!encryption.isKeypair(keys)) {
            return "Invalid input. 1st argument of encryption.sign must be a keypair.";
        } else if (!(data instanceof ArrayBuffer || ArrayBuffer.isView(data))) {
            return "Inalid input. 2nd argument of encryption.sign must be an ArrayBuffer."
        }

        let signature = null;
        try {
            signature = await window.crypto.subtle.sign("HMAC", keys.hmac, data);
        } catch (ex) {
            return null;
        }

        return signature;
    };
    static async verify (keys, signature, data) {
        // Validate the type of the inputs
        if (!encryption.isKeypair(keys)) {
            return "Invalid input. 1st argument of encryption.sign must be a keypair.";
        } else if (!(signature instanceof ArrayBuffer || ArrayBuffer.isView(signature))) {
            return "Inalid input. 2nd argument of encryption.sign must be an ArrayBuffer."
        } else if (!(data instanceof ArrayBuffer || ArrayBuffer.isView(data))) {
            return "Inalid input. 3rd argument of encryption.sign must be an ArrayBuffer."
        }

        let verified = false;
        try {
            verified = await window.crypto.subtle.verify("HMAC", keys.hmac, signature, data);
        } catch (ex) {
            return false;
        }

        return verified;
    };

    static async encrypt (keys, data) {
        // Validate the type of the inputs
        if (!encryption.isKeypair(keys)) {
            return "Invalid input. 1st argument of encryption.encrypt must be a keypair.";
        } else if (!(data instanceof ArrayBuffer || ArrayBuffer.isView(data))) {
            return "Inalid input. 2nd argument of encryption.encrypt must be an ArrayBuffer."
        }

        let iv = encryption.randomBytes(encryption.AES_IV_LENGTH);
        if (iv === null) {
            return null;
        }
        let aesParams = {name: encryption.AES_ALGO, iv: iv};

        let cipherText = null;
        try {
            cipherText = await window.crypto.subtle.encrypt(aesParams, keys.aes, data);
        } catch (ex) {
            return null;
        }

        let tmp = new Uint8Array(iv.byteLength + cipherText.byteLength);
        tmp.set(new Uint8Array(iv));
        tmp.set(new Uint8Array(cipherText), iv.byteLength);

        let signature = await encryption.sign(keys, tmp.buffer);
        if (signature === null) {
            return null;
        }
        let signatureArray = new Uint8Array(signature);

        let result = new Uint8Array(signatureArray.length + tmp.length);
        result.set(signatureArray);
        result.set(tmp, signatureArray.length);

        return result.buffer;
    };
    static async decrypt (keys, data) {
        // Validate the type of the inputs
        if (!encryption.isKeypair(keys)) {
            return "Invalid input. 1st argument of encryption.decrypt must be a keypair.";
        } else if (!(data instanceof ArrayBuffer || ArrayBuffer.isView(data))) {
            return "Inalid input. 2nd argument of encryption.decrypt must be an ArrayBuffer."
        }

        let dataArr = new Uint8Array(data);
        let signature = dataArr.slice(0, encryption.HMAC_LENGTH);
        let combined = dataArr.slice(signature.length);
        let iv = combined.slice(0, encryption.AES_IV_LENGTH);
        let cipherText = combined.slice(iv.length);

        let verified = await encryption.verify(keys, signature, combined);
        if (!verified) {
            return null;
        }

        let plainText = null;
        try {
            let aesParams = {name: encryption.AES_ALGO, iv: iv.buffer};
            plainText = await window.crypto.subtle.decrypt(aesParams, keys.aes, cipherText);
        } catch (ex) {
            return null;
        }

        return plainText;
    };

    static async lowEntropyToKeys (bytes, salt) {
        let bits = null;
        
        try {
            let raw = await window.crypto.subtle.importKey("raw", bytes, "PBKDF2", false, ["deriveBits"]);
            let params = {name: "PBKDF2", iterations: encryption.PBKDF2_ITERATIONS, salt: salt, hash: encryption.PBKDF2_HASH};
            bits = await window.crypto.subtle.deriveBits(params, raw, encryption.PBKDF2_BITS);
        } catch (ex) {
            bits = null;
            return null;
        }

        return await encryption.highEntropyToKeys(bits, salt);
    };
    static async highEntropyToKeys (bytes, salt) {
        let aesParams = {name: encryption.AES_ALGO, length: encryption.AES_LENGTH};
        let hmacParams = {name: "HMAC", hash: encryption.HMAC_HASH};

        let saltArray = new Uint8Array(salt);
        let aesSalt = saltArray.slice(0, saltArray.length >> 1);
        let hmacSalt = saltArray.slice(saltArray.length >> 1);

        let bytesArray = new Uint8Array(bytes);
        let aesBytes = bytesArray.slice(0, bytesArray.length >> 1);
        let hmacBytes = bytesArray.slice(bytesArray.length >> 1);

        let aesKey, hmacKey = null;

        try {
            let aesRaw = window.crypto.subtle.importKey("raw", aesBytes, "HKDF", false, ["deriveKey"]);
            let hmacRaw = window.crypto.subtle.importKey("raw", hmacBytes, "HKDF", false, ["deriveKey"]);

            let aesHKDFParams = {name: "HKDF", info: new ArrayBuffer(0), salt: aesSalt, hash: encryption.HKDF_HASH};
            let hmacHKDFParams = {name: "HKDF", info: new ArrayBuffer(0), salt: hmacSalt, hash: encryption.HKDF_HASH};

            let aesPromise = window.crypto.subtle.deriveKey(aesHKDFParams, aesRaw, aesParams, false, ["encrypt", "decrypt"]);
            let hmacPromise = window.crypto.subtle.deriveKey(hmacHKDFParams, hmacRaw, hmacParams, false, ["sign", "verify"]);

            aesKey = await aesPromise;
            hmacKey = await hmacPromise;
        } catch (ex) {
            return null;
        }

        return {aes: aesKey, hmac: hmacKey};
    };
    static async randomKeys () {
        let aesParams = {name: encryption.AES_ALGO, length: encryption.AES_LENGTH};
        let hmacParams = {name: "HMAC", hash: encryption.HMAC_HASH};

        let aesPromise = window.crypto.subtle.generateKey(aesParams, false, ["encrypt", "decrypt"]);
        let hmacPromise = window.crypto.subtle.generateKey(hmacParams, false, ["sign", "verify"]);

        let aesKey = await aesPromise;
        let hmacKey = await hmacPromise;

        return {aes: aesKey, hmac: hmacKey};
    };

    static randomBytes (count) {
        // Validate input
        if (typeof count !== "number" || count <= 0) {
            throw "Invalid input. 1st argument of encryption.randomBytes must be a number between 1 and Inf";
        }

        let bytes = new Uint8Array(count);
        try {
            window.crypto.getRandomValues(bytes);
        } catch (ex) {
            return null;
        }

        return bytes;
    };
};

class passwords {
    static byteDivision (bytes, byte) {
        let u8 = new Uint8Array(bytes);
        let result = new Uint8Array(u8.length);
        let u16 = new Uint16Array(3);
        for (let i = 0; i < u8.length; i++) {
            // The value to operate on next is the previous remainder * 256 + current digit
            u16[0] = (u16[1] << 8) + u8[i];
            // The quotient u16[2] and remainder u16[1] are calculated
            u16[2] = u16[0] / byte;
            u16[1] = u16[0] - (u16[2] * byte);

            result[i] = u16[2];
        }

        let leadingZeroCount = 0;
        for (let i = 0; i < result.length; i++) {
            if (result[i] === 0) {
                leadingZeroCount++;
            } else {
                break;
            }
        }

        return {q: result.slice(leadingZeroCount), r: u16[1]};
    };
    static fromBytes (bytes, dictionary) {
        let result = "";
        let u8 = new Uint8Array(bytes);
        while (u8.length > 0) {
            let qr = passwords.byteDivision(u8, dictionary.length);
            result += dictionary[qr.r];
            u8 = qr.q;
        }

        let bits = Math.log2(dictionary.length);
        while (result.length * bits < u8.length * 8) {
            result = dictionary[0] + result;
        }

        return result;
    };
};

passwords.ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
passwords.DEFAULT = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789`~!@#$%^&*_+=|;:/?.>,<";

encryption.ALGORITHM = "AES-256-CBC_HMAC-SHA-384";
encryption.PBKDF2_ITERATIONS = 4194369;
encryption.PBKDF2_HASH = "SHA-512";
encryption.PBKDF2_BITS = 512;
encryption.HKDF_HASH = "SHA-256";
encryption.AES_ALGO = "AES-CBC";
encryption.AES_LENGTH = 256;
encryption.AES_IV_LENGTH = 128 >> 3;
encryption.HMAC_HASH = "SHA-384";
encryption.HMAC_LENGTH = 382 >> 3;

class appStates {};
appStates.NOT_CONFIGURED = 0x00;
appStates.CONFIGURED     = 0x01;
appStates.LOCKED         = 0x02;
appStates.UNLOCKED       = 0x03;

appStates.CMD_GET_STATE  = 0x04;
appStates.CMD_CONFIGURE  = 0x05;
appStates.CMD_PASSWORD   = 0x06;
appStates.CMD_LOCK       = 0x07;
appStates.CMD_RESET      = 0x08;

appStates.REQ_ADD_RECORD   = 0x09;
appStates.REQ_GET_RECORD   = 0x10;
appStates.REQ_GEN_PASSWORD = 0x11;