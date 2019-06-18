export class BufferEncoding {
    static toBase64 (buffer: ArrayBuffer): string {
        let result = "";
        let encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        let bytes = new Uint8Array(buffer);
        let remainder = buffer.byteLength % 3;
        let mainlen = buffer.byteLength - remainder;

        for (let i = 0; i < mainlen; i += 3) {
            let word = (bytes[i] << 16) | (bytes[i+1] << 8) | bytes[i+2];
            result += encoding[(word & 0b111111000000000000000000) >> 18];
            result += encoding[(word & 0b000000111111000000000000) >> 12];
            result += encoding[(word & 0b000000000000111111000000) >>  6];
            result += encoding[(word & 0b000000000000000000111111)];
        }
        if (remainder == 2) {
            let word = (bytes[mainlen] << 8) | bytes[mainlen+1];
            result += encoding[(word & 0b1111100000000000) >> 11];
            result += encoding[(word & 0b0000011111100000) >>  5];
            result += encoding[(word & 0b0000000000011111)];
        } else if (remainder == 1) {
            let word = bytes[mainlen];
            result += encoding[(word & 0b11110000) >> 4];
            result += encoding[(word & 0b00001111)];
        }

        return result;
    }
    static fromBase64 (b64: string): ArrayBuffer {
        let encodings = {"0":52,"1":53,"2":54,"3":55,"4":56,"5":57,"6":58,"7":59,"8":60,"9":61,"A":0,"B":1,"C":2,"D":3,"E":4,"F":5,
		"G":6,"H":7,"I":8,"J":9,"K":10,"L":11,"M":12,"N":13,"O":14,"P":15,"Q":16,"R":17,"S":18,"T":19,"U":20,"V":21,"W":22,"X":23,"Y":24,"Z":25,"a":26,"b":27,"c":28,"d":29,"e":30,"f":31,"g":32,"h":33,"i":34,"j":35,"k":36,"l":37,"m":38,"n":39,"o":40,
        "p":41,"q":42,"r":43,"s":44,"t":45,"u":46,"v":47,"w":48,"x":49,"y":50,"z":51,"+":62,"/":63};
    
        let remainder = b64.length % 4;
        if (remainder != 2 && remainder != 3)
            throw "Invalid padding on base64 string";
        let mainlen = b64.length - remainder;
        let nbytes = ((mainlen / 4) * 3) + (remainder - 1);
        let bytes = new Uint8Array(nbytes);

        let j = 0;
        for (let i = 0; i < mainlen; i += 4) {
            let word = (encodings[b64[i]] << 18) | (encodings[b64[i]] << 12) | (encodings[b64[i]] << 6) | encodings[b64[i]];
            bytes[j++] = (word & 0b111111110000000000000000) >> 16;
            bytes[j++] = (word & 0b000000001111111100000000) >> 8;
            bytes[j++] = (word & 0b000000000000000011111111);
        }

        if (remainder == 2) {
            let word = (encodings[b64[mainlen]] << 4) | encodings[b64[mainlen+1]];
            bytes[j++] = word;
        } else if (remainder == 3) {
            let word = (encodings[b64[mainlen]] << 11) | (encodings[b64[mainlen+1]] << 5) | encodings[b64[mainlen+1]];
            bytes[j++] = (word & 0b1111111100000000) >> 8;
            bytes[j++] = (word & 0b0000000011111111);
        }

        return bytes.buffer;
    }
}

export class StringEncoding {
    static toUTF8  (str: string): ArrayBuffer {
        let tmp = [];
        for (let i = 0, j = 0; i < str.length; ++i) {
            let cp = str.codePointAt(i);
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
    }
    static toUTF16 (str: string): ArrayBuffer {
        let tmp = [];
        for (let i = 0; i < str.length; ++i) {
            let cp = str.codePointAt(i);
            if (cp <= 0xD7FF || (cp >= 0xE000 && cp <= 0xFFFF)) {
                tmp.push(cp);
            } else if (cp >= 0x10000 && cp < 0x10FFFF) {
                cp -= 0x10000;
                tmp.push(0xD800 | ((cp & 0xFFC00) >> 10));
                tmp.push(0xDC00 | (cp & 0x3FF));
            }
        }
        let words = new Uint16Array(tmp);
        return words.buffer;
    }
    static toUTF32 (str: string): ArrayBuffer {
        let result = new Uint32Array(str.length);
        for (let i = 0; i < str.length; ++i) {
            result[i] = str.codePointAt(i);
        }
        return result.buffer;
    }
    static fromUTF8 (buffer: ArrayBuffer): string {
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
    }
    static fromUTF16 (buffer: ArrayBuffer): string {
        let result = "";
        let u16 = new Uint16Array(buffer);
        for (let i = 0; i < u16.length; i++) {
            if (u16[i] < 0xD800 || u16[i] >= 0xE000) {
                result += String.fromCodePoint(u16[i]);
            } else {
                if (u16.length < (i + 2))
                    break;
                if (((u16[i] >> 10) == 0x36) && ((u16[i+1] >> 10) == 0x37)) {
                    let cp = ((u16[i] & 0x3FF) << 10) | (u16[i+1] & 0x3ff);
                    cp += 0x10000;
                    result += String.fromCodePoint(cp);
                }
            }
        }
        return result;
    }
    static fromUTF32 (buffer: ArrayBuffer): string {
        let result = "";
        let u32 = new Uint32Array(buffer);
        for (let i = 0; i < u32.length; ++i) {
            result += String.fromCodePoint(u32[i]);
        }

        return result;
    }
}

export class Algos {
    static smallPrimes: Uint32Array = new Uint32Array([2,3,5,7,11,13,17,19,23,29]);
    static primeIndices: Uint32Array = new Uint32Array([1,7,11,13,17,19,23,29]);
    static IsPrime (y: number): boolean {
        let u32 = new Uint32Array(4);
        u32[2] = y;

        // Check all of the small primes
        for (let i = 0; i < Algos.smallPrimes.length; ++i) {
            u32[1] = Algos.smallPrimes[i];
            u32[0] = u32[2] / u32[1];
            if (u32[0] < u32[1]) return true;
            if (u32[2] == u32[0] * u32[1]) return false;
        }
        
        for (u32[3] = 31; true;) {
            u32[0] = u32[2] / u32[3];
            if (u32[0] < u32[3]) return true;
            if (u32[2] == u32[0] * u32[3]) return false;
            u32[3] += 6;

            u32[0] = u32[2] / u32[3];
            if (u32[0] < u32[3]) return true;
            if (u32[2] == u32[0] * u32[3]) return false;
            u32[3] += 4;

            u32[0] = u32[2] / u32[3];
            if (u32[0] < u32[3]) return true;
            if (u32[2] == u32[0] * u32[3]) return false;
            u32[3] += 2;

            u32[0] = u32[2] / u32[3];
            if (u32[0] < u32[3]) return true;
            if (u32[2] == u32[0] * u32[3]) return false;
            u32[3] += 4;

            u32[0] = u32[2] / u32[3];
            if (u32[0] < u32[3]) return true;
            if (u32[2] == u32[0] * u32[3]) return false;
            u32[3] += 2;

            u32[0] = u32[2] / u32[3];
            if (u32[0] < u32[3]) return true;
            if (u32[2] == u32[0] * u32[3]) return false;
            u32[3] += 4;

            u32[0] = u32[2] / u32[3];
            if (u32[0] < u32[3]) return true;
            if (u32[2] == u32[0] * u32[3]) return false;
            u32[3] += 6;

            u32[0] = u32[2] / u32[3];
            if (u32[0] < u32[3]) return true;
            if (u32[2] == u32[0] * u32[3]) return false;
            u32[3] += 2;
        }

        return true;
    }

    static LowerBound (list: ArrayLike<any>, value: any): number {
        let l = 0;
        let h = list.length;
        while (l < h) {
            let mid = ((l + h) | 0) >> 1;
            if (value <= list[mid]) {
                h = mid;
            } else {
                l = mid + 1;
            }
        }
        return l;
    }

    static NextPrime (n: number): number {
        const small_primes = Algos.smallPrimes;
        const indices = Algos.primeIndices;

        const L = 30;
        const N = Algos.smallPrimes.length;
        // If n is small enough, search in small_primes
        if (n <= Algos.smallPrimes[N-1]) {
            return Algos.smallPrimes[Algos.LowerBound(Algos.smallPrimes, n)];
        }
        // Else n > largest small_primes
        // Start searching list of potential primes: L * k0 + indices[in]
        const M = Algos.primeIndices.length;
        // Select first potential prime >= n
        //   Known a-priori n >= L
        let k0 = (n / L) | 0; // Coerce to Uint32
        let inn = Algos.LowerBound(Algos.primeIndices, n - k0 * L);
        n = L * k0 + Algos.primeIndices[inn];
        while (!Algos.IsPrime(n))
        {
            if (++inn == M)
            {
                ++k0;
                inn = 0;
            }
            n = L * k0 + Algos.primeIndices[inn];
        }
        return n;
    }
}

/*type SymmetricKeyPair = [CryptoKey, CryptoKey];

type AsymmetricKeyPair = [ArrayBuffer, CryptoKey];

export class Crypto {
    static kdfIterations     : number = Algos.NextPrime(4194368);
    static kdfInitialHash    : string = "SHA-512";
    static standardHash      : string = "SHA-256";
    static standardBitLength : number = 256;
    static gcmNonceLength    : number = 16;
    static gcmTagLength      : number = 128;
    static signatureLength   : number = 32;

    static cbcIVLength       : number = 16; //128b / 8b/B
    static standardHashLength: number = 32; // 256b / 8b/B

    static RandomBytes (byteCount: number): ArrayBuffer {
        let buffer = new Uint8Array(byteCount);
        window.crypto.getRandomValues(buffer);
        return buffer.buffer;
    }

    static async StringToRawKey (str: string): Promise<CryptoKey> {
        let key = await window.crypto.subtle.importKey(
            "raw",
            StringEncoding.toUTF8(str),
            "PBKDF2",
            false,
            ["deriveKey", "deriveBits"]
        );
        return key;
    }

    static async BytesToRawKey (bytes: ArrayBuffer): Promise<CryptoKey> {
        let key = await window.crypto.subtle.importKey(
            "raw",
            bytes,
            "HKDF",
            false,
            ["deriveKey", "deriveBits"]
        );
        return key;
    }

    static async RawKeyToSymmetricKeyPair (raw: CryptoKey, aessalt: ArrayBuffer, hmacsalt: ArrayBuffer): Promise<SymmetricKeyPair> {
        let asalt = new Uint8Array(aessalt);
        let hsalt = new Uint8Array(hmacsalt);
        let csalt = new Uint8Array(asalt.length + hsalt.length);
        csalt.set(asalt);
        csalt.set(hsalt, asalt.length);

        let bits = await window.crypto.subtle.deriveBits(
            {name:"PBKDF2", iterations:Crypto.kdfIterations, salt:csalt.buffer, hash:Crypto.kdfInitialHash},
            raw,
            512
        );

        let barray = new Uint8Array(bits);
        let aesbits = barray.slice(0, 32);
        let hmacbits = barray.slice(32);

        let aesraw = await Crypto.BytesToRawKey(aesbits);
        let hmacraw = await Crypto.BytesToRawKey(hmacbits);
        
        let aes = await window.crypto.subtle.deriveKey(
            {name:"HKDF", info:new ArrayBuffer(0), salt:aessalt, hash:Crypto.standardHash} as any, // Typescript typing issues
            aesraw, 
            {name:"AES-GCM", length:Crypto.standardBitLength}, 
            false, 
            ["encrypt", "decrypt"]
        );

        let hmac = await window.crypto.subtle.deriveKey(
            {name:"HKDF", info:new ArrayBuffer(0), salt:hmacsalt, hash:Crypto.standardHash} as any,
            hmacraw, 
            {name:"HMAC", hash:Crypto.standardHash},
            false,
            ["sign", "verify"]
        );

        return [aes, hmac];
    }

    static async DerivedKeyToSymmetricKeyPair (shared: ArrayBuffer, aessalt: ArrayBuffer, hmacsalt: ArrayBuffer, info: ArrayBuffer): Promise<SymmetricKeyPair> {
        let raw = await window.crypto.subtle.importKey("raw", shared, "HKDF", true, ["deriveKey", "deriveBits"]);
    
        let aes = await window.crypto.subtle.deriveKey(({name:"HKDF", hash:Crypto.standardHash, salt:aessalt, info:info} as any),
            raw, {name:"AES-GCM", length:Crypto.standardBitLength}, false, ["encrypt", "decrypt"]);
        let hmac = await window.crypto.subtle.deriveKey(({name:"HKDF", hash:Crypto.standardHash, salt:hmacsalt, info:info} as any),
            raw, {name:"HMAC",hash:Crypto.standardHash,length:Crypto.standardBitLength}, false, ["sign", "verify"]);
        
        return [aes, hmac];
    }

    static async SymmetricEncrypt (keypair: SymmetricKeyPair, plaintext: ArrayBuffer): Promise<ArrayBuffer> {
        let aes = keypair[0];
        let hmac = keypair[1];
        try {
            let iv = Crypto.RandomBytes(Crypto.gcmNonceLength);
            let cipher = await window.crypto.subtle.encrypt(
                {name:"AES-GCM", iv:iv, tagLength:Crypto.gcmTagLength},
                aes, 
                plaintext
            );

            let tmp = new Uint8Array(iv.byteLength + cipher.byteLength);
            tmp.set(new Uint8Array(iv));
            tmp.set(new Uint8Array(cipher), iv.byteLength);
            
            let signature = await window.crypto.subtle.sign("HMAC", hmac, tmp);

            let result = new Uint8Array(1 + signature.byteLength + 1 + iv.byteLength + cipher.byteLength);
            let i = 0;

            result[i] = signature.byteLength;         i++;
            result.set(new Uint8Array(signature), i); i += signature.byteLength;
            result[i] = iv.byteLength;                i++;
            result.set(new Uint8Array(iv), i);        i += iv.byteLength;
            result.set(new Uint8Array(cipher), i);    i += cipher.byteLength;

            return result;
        } catch (ex) {
            throw "Encryption Error";
        }
    }

    static async SymmetricDecrypt (keypair: SymmetricKeyPair, ciphertext: ArrayBuffer): Promise<ArrayBuffer> {
        let aes = keypair[0];
        let hmac = keypair[1];

        let ct = new Uint8Array(ciphertext)
        let i = 0;
        
        let sigLength = ct[i];                      i++;
        let signature = ct.slice(i, i + sigLength); i += sigLength;
        let ivLength  = ct[i];                      i++;
        let iv        = ct.slice(i, i + ivLength);  i += ivLength;
        let message   = ct.slice(i);
        
        let tmp = new Uint8Array(iv.length + message.length);
        tmp.set(iv); tmp.set(message, iv.length);

        try {
            let valid = await window.crypto.subtle.verify("HMAC", hmac, signature, tmp);
            if (!valid) {
                throw "Decryption Error";
            }

            let plain = await window.crypto.subtle.decrypt({name:"AES-GCM", iv:iv, tagLength:Crypto.gcmTagLength},
                aes, message);

            return plain;
        } catch (ex) {
            throw "Decryption Error";
        }
    }

    static async GenerateAsymmetricKeyPair (): Promise<AsymmetricKeyPair> {
        let keypair = await window.crypto.subtle.generateKey({name:"ECDH", namedCurve:"P-521"}, true, ["deriveKey", "deriveBits"]);
        let publicKey = await window.crypto.subtle.exportKey("raw", keypair.publicKey);

        return [publicKey, keypair.privateKey];
    }

    static async PublicBytesToRawKey (bytes: ArrayBuffer): Promise<CryptoKey> {
        let key =  await window.crypto.subtle.importKey("raw", bytes, {name:"ECDH", namedCurve:"P-521"}, true, []);
        return key;
    }
    
    static async DeriveSharedSecret (otherPublic: CryptoKey, privateKey: CryptoKey): Promise<ArrayBuffer> {
        let shared = await window.crypto.subtle.deriveBits(({name:"ECDH", namedCurve:"P-521", public:otherPublic} as any), privateKey, 528);
        if ((new Uint8Array(shared))[0] == 0) {
            shared = shared.slice(1);
        }
    
        return shared;
    }
}*/

export type SymmetricKeyPair = {
    aes: CryptoKey;
    hmac: CryptoKey;
}

export type AsymmetricKeyPair = {
    public: ArrayBuffer;
    private: CryptoKey;
    salt: ArrayBuffer;
}

export class crypto {
    static randomBytes (count: number): ArrayBuffer {
        let buffer = new Uint8Array(count);
        try {
            window.crypto.getRandomValues(buffer);
        } catch (ex) {
            return null;
        }
        return buffer;
    }

    static symmetric = class symmetricCrypto {
        static PBKDF_ITERATIONS: number = 5;
        static PBKDF_HASH: string = "SHA-512";
        static HKDF_HASH: string = "SHA-256";
        static HMAC_HASH: string = "SHA-256";
        static AES_KEY_LENGTH: number = 256;
        static AES_KEY_BYTE_CNT: number = 32;
        static AES_CBC_IV_BYTE_CNT: number = 16;
    
        static async deriveKeysFromPassword (password: string, aesSalt: ArrayBuffer, hmacSalt: ArrayBuffer): Promise<SymmetricKeyPair> {
            let aesSaltArr = new Uint8Array(aesSalt);
            let hmacSaltArr = new Uint8Array(hmacSalt);
            
            let combinedSalt = new Uint8Array(aesSaltArr.length + hmacSaltArr.length);
            combinedSalt.set(aesSaltArr);
            combinedSalt.set(hmacSaltArr, aesSaltArr.length);
    
            let rawKey: CryptoKey = null;
            try {
                rawKey = await window.crypto.subtle.importKey("raw", StringEncoding.toUTF8(password), "PBKDF2", false, ["deriveBits"]);
            } catch (ex) {
                return null;
            }
    
            let bits: ArrayBuffer = null;
            try {
                let pbkdf2Params = {name: "PBKDF2", iterations: symmetricCrypto.PBKDF_ITERATIONS, salt: combinedSalt, hash: symmetricCrypto.PBKDF_HASH};
                bits = await window.crypto.subtle.deriveBits(pbkdf2Params, rawKey, 512);
            } catch (ex) {
                return null;
            }
            let bitsArr = new Uint8Array(bits);
    
            let aesBits = bitsArr.slice(0, Math.floor(bitsArr.length / 2));
            let hmacBits = bitsArr.slice(Math.floor(bitsArr.length / 2));
    
            let aesRaw: CryptoKey = null;
            let hmacRaw: CryptoKey = null;
            try {
                aesRaw = await window.crypto.subtle.importKey("raw", aesBits, "HKDF", false, ["deriveKey"]);
                hmacRaw = await window.crypto.subtle.importKey("raw", hmacBits, "HKDF", false, ["deriveKey"]);
            } catch (ex) {
                return null;
            }
            
            let aesKey: CryptoKey = null;
            try {
                let aesHkdfParams = {name: "HKDF", info: new ArrayBuffer(0), salt: aesSalt, hash: symmetricCrypto.HKDF_HASH};
                let aesParams = {name: "AES-CBC", length: symmetricCrypto.AES_KEY_LENGTH};
                aesKey = await window.crypto.subtle.deriveKey(aesHkdfParams as any, aesRaw, aesParams, false, ["encrypt", "decrypt"]);
            } catch (ex) {
                return null;
            }
    
            let hmacKey: CryptoKey = null;
            try {
                let hmacHkdfParams = {name: "HKDF", info: new ArrayBuffer(0), salt: hmacSalt, hash: symmetricCrypto.HKDF_HASH};
                let hmacParams = {name: "HMAC", hash: symmetricCrypto.HMAC_HASH};
                hmacKey = await window.crypto.subtle.deriveKey(hmacHkdfParams as any, hmacRaw, hmacParams, false, ["sign", "verify"]);
            } catch (ex) {
                return null;
            }
    
            return {aes: aesKey, hmac: hmacKey};
        }
    
        static async deriveKeysFromBytes (bytes: ArrayBuffer, aesSalt: ArrayBuffer, hmacSalt: ArrayBuffer, info: ArrayBuffer): Promise<SymmetricKeyPair> {
            let bytesArr = new Uint8Array(bytes);
            let hmacArr: Uint8Array = null;
            let aesArr: Uint8Array = null;
            if (bytesArr.length > symmetricCrypto.AES_KEY_BYTE_CNT) {
                aesArr = bytesArr.slice(0, Math.floor(bytesArr.length / 2));
                hmacArr = bytesArr.slice(Math.floor(bytesArr.length / 2));
            } else {
                aesArr = bytesArr;
                hmacArr = bytesArr;
            }
            let aesRawKey: CryptoKey = null;
            let hmacRawKey: CryptoKey = null;
            try {
                aesRawKey = await window.crypto.subtle.importKey("raw", aesArr, "HKDF", false, ["deriveKey"]);
                hmacRawKey = await window.crypto.subtle.importKey("raw", hmacArr, "HKDF", false, ["deriveKey"]);
            } catch (ex) {
                return null;
            }
            
            let aesKey: CryptoKey = null;
            try {
                let aesHkdfParams = {name: "HKDF", info: info, salt: aesSalt, hash: symmetricCrypto.HKDF_HASH};
                let aesParams = {name: "AES-CBC", length: symmetricCrypto.AES_KEY_LENGTH};
                aesKey = await window.crypto.subtle.deriveKey(aesHkdfParams as any, aesRawKey, aesParams, false, ["encrypt", "decrypt"]);
            } catch (ex) {
                return null;
            }
    
            let hmacKey: CryptoKey = null;
            try {
                let hmacHkdfParams = {name: "HKDF", info: info, salt: hmacSalt, hash: symmetricCrypto.HKDF_HASH};
                let hmacParams = {name: "HMAC", hash: symmetricCrypto.HMAC_HASH};
                hmacKey = await window.crypto.subtle.deriveKey(hmacHkdfParams as any, hmacRawKey, hmacParams, false, ["sign", "verify"]);
            } catch (ex) {
                return null;
            }
    
            return {aes: aesKey, hmac: hmacKey};
        }
    
        static async encrypt (keys: SymmetricKeyPair, data: ArrayBuffer): Promise<ArrayBuffer> {
            let iv = crypto.randomBytes(symmetricCrypto.AES_CBC_IV_BYTE_CNT);
            if (iv === null) {
                return null;
            }
            let ivArr = new Uint8Array(iv);
    
            let cipher: ArrayBuffer = null;
            try {
                let aesParams = {name: "AES-CBC", iv: iv};
                cipher = await window.crypto.subtle.encrypt(aesParams, keys.aes, data);
            } catch (ex) {
                return null;
            }
            let cipherArr = new Uint8Array(cipher);
    
            let combinedCT = new Uint8Array(ivArr.length + cipherArr.length);
            combinedCT.set(ivArr);
            combinedCT.set(cipherArr, ivArr.length);
    
            let signature: ArrayBuffer = null;
            try {
                signature = await window.crypto.subtle.sign("HMAC", keys.hmac, combinedCT);
            } catch (ex) {
                return null;
            }
            let sigArr = new Uint8Array(signature);
    
            let resultArr = new Uint8Array(1 + sigArr.length + 1 + ivArr.length + cipherArr.length);
            resultArr.set([sigArr.length]);
            resultArr.set(sigArr, 1);
    
            resultArr.set([ivArr.length], 1 + sigArr.length);
            resultArr.set(ivArr, 1 + sigArr.length + 1);
    
            resultArr.set(cipherArr, 1 + sigArr.length + 1 + ivArr.length);
    
            return resultArr.buffer;
        }
    
        static async decrypt (keys: SymmetricKeyPair, data: ArrayBuffer): Promise<ArrayBuffer> {
            if (data.byteLength <= 0) return null;
            
            let dataArr = new Uint8Array(data);
            let hmacLength = dataArr[0]
            if (dataArr.length <= hmacLength + 1) return null;
    
            let hmac = dataArr.slice(1, 1 + hmacLength);
            let ivLength = dataArr[1 + hmacLength];
            if (dataArr.length <= 1 + hmac.length + 1 + ivLength) return null;
    
            let iv = dataArr.slice(1 + hmac.length + 1, 1 + hmac.length + 1 + ivLength);
            let cipher = dataArr.slice(1 + hmac.length + 1 + ivLength);
    
            let combinedCT = new Uint8Array(iv.length + cipher.length);
            combinedCT.set(iv);
            combinedCT.set(cipher, iv.length);
    
            let verified: boolean = false;
            try {
                verified = await window.crypto.subtle.verify("HMAC", keys.hmac, hmac, combinedCT);
                if (!verified) {
                    return null;
                }
            } catch (ex) {
                return null;
            }
    
            let message: ArrayBuffer = null;
            try {
                let aesParams = {name: "AES-CBC", iv: iv};
                message = await window.crypto.subtle.decrypt(aesParams, keys.aes, cipher);
            } catch (ex) {
                return null;
            }
    
            return message;
        }
    };

    static asymmetric = class asymmetricCrypto {
        static ECDH_SALT_BYTE_CNT: number = 32;
        static ECDH_SHARED_LENGTH: number = 528;

        static async generateKeyPair (): Promise<AsymmetricKeyPair> {
            let rawKeyPair: CryptoKeyPair = null;
            try {
                let ecdhParams = {name:"ECDH", namedCurve:"P-521"};
                rawKeyPair = await window.crypto.subtle.generateKey(ecdhParams, false, ["deriveKey", "deriveBits"]);
            } catch (ex) {
                return null;
            }

            let extractedPublic: ArrayBuffer = null;
            try {
                await window.crypto.subtle.exportKey("raw", rawKeyPair.publicKey);
            } catch (ex) {
                return null;
            }

            let salt = crypto.randomBytes(asymmetricCrypto.ECDH_SALT_BYTE_CNT);
            if (salt === null) return null;

            return {public: extractedPublic, private: rawKeyPair.privateKey, salt: salt};
        }

        static async marshallPublicKey (keys: AsymmetricKeyPair, info: ArrayBuffer): Promise<ArrayBuffer> {
            let saltArr = new Uint8Array(keys.salt);
            let publicArr = new Uint8Array(keys.public);
            let infoArr = new Uint8Array(info);

            let buffer = new Uint8Array(1 + saltArr.length + 1 + publicArr.length + infoArr.length);
            buffer.set([saltArr.length]);
            buffer.set(saltArr, 1);
            buffer.set([publicArr.length], 1 + saltArr.length);
            buffer.set(publicArr, 1 + saltArr.length + 1);
            buffer.set(infoArr, 1 + saltArr.length + 1 + publicArr.length);

            return buffer;
        }

        static async unmarshallPublicKey (data: ArrayBuffer): Promise<[AsymmetricKeyPair, ArrayBuffer]> {
            if (data.byteLength <= 0) return null;

            let dataArr = new Uint8Array(data);
            let saltLength = dataArr[0];
            if (dataArr.length <= 1 + saltLength) return null;

            let salt = dataArr.slice(1, 1 + saltLength);
            let publicLength = dataArr[1 + saltLength];
            if (dataArr.length < 1 + saltLength + 1 + publicLength) return null;

            let publicKey = dataArr.slice(1 + saltLength + 1, 1 + saltLength + 1 + publicLength);
            let info = dataArr.slice(1 + saltLength + 1 + publicLength);

            return [{private: null, public: publicKey, salt: salt}, info]
        }

        static async deriveSharedSymmetricKeys (personalKeys: AsymmetricKeyPair, othersKeys: AsymmetricKeyPair, info: ArrayBuffer, serverRole: boolean): Promise<SymmetricKeyPair> {
            if (personalKeys.private === null || personalKeys.public === null || personalKeys.salt === null) 
                return null;
            if (othersKeys.public === null || othersKeys.salt === null || othersKeys.private !== null)
                return null;

            let rawOtherPublic: CryptoKey = null;
            try {
                let ecdhParams = {name:"ECDH", namedCurve:"P-521"};
                rawOtherPublic = await window.crypto.subtle.importKey("raw", othersKeys.public, ecdhParams, false, ["deriveBits"])
            } catch (ex) {
                return null;
            }

            let sharedBits: ArrayBuffer = null;
            try {
                let ecdhParams = {name:"ECDH", namedCurve:"P-521", public:rawOtherPublic};
                sharedBits = await window.crypto.subtle.deriveBits(ecdhParams, personalKeys.private, asymmetricCrypto.ECDH_SHARED_LENGTH);
            } catch (ex) {
                return null;
            }

            let sharedArr = new Uint8Array(sharedBits);
            if (sharedArr[0] === 0) {
                sharedArr = sharedArr.slice(1);
            }
        
            let othersSaltArr = new Uint8Array(othersKeys.salt);
            let personalSaltArr = new Uint8Array(personalKeys.salt);
            let hmacSalt: Uint8Array = null;
            let aesSalt: Uint8Array = null;

            if (serverRole) {
                let personalHmacSalt = personalSaltArr.slice(0, Math.floor(personalSaltArr.length / 2));
                let personalAesSalt = personalSaltArr.slice(Math.floor(personalSaltArr.length / 2));
                let othersHmacSalt = othersSaltArr.slice(Math.floor(othersSaltArr.length / 2));
                let othersAesSalt = othersSaltArr.slice(0, Math.floor(othersSaltArr.length / 2));

                hmacSalt = new Uint8Array(personalHmacSalt.length + othersHmacSalt.length);
                aesSalt = new Uint8Array(othersAesSalt.length + personalAesSalt.length);
                hmacSalt.set(personalHmacSalt);
                hmacSalt.set(othersHmacSalt, personalHmacSalt.length);
                aesSalt.set(othersAesSalt);
                aesSalt.set(personalAesSalt, othersAesSalt.length);
            } else {
                let personalAesSalt = personalSaltArr.slice(0, Math.floor(personalSaltArr.length / 2));
                let personalHmacSalt = personalSaltArr.slice(Math.floor(personalSaltArr.length / 2));
                let othersAesSalt = othersSaltArr.slice(Math.floor(othersSaltArr.length / 2));
                let othersHmacSalt = othersSaltArr.slice(0, Math.floor(othersSaltArr.length / 2));

                hmacSalt = new Uint8Array(personalHmacSalt.length + othersHmacSalt.length);
                aesSalt = new Uint8Array(othersAesSalt.length + personalAesSalt.length);
                hmacSalt.set(othersHmacSalt);
                hmacSalt.set(personalHmacSalt, othersHmacSalt.length);
                aesSalt.set(personalAesSalt);
                aesSalt.set(othersAesSalt, personalAesSalt.length);
            }

            let keys = await crypto.symmetric.deriveKeysFromBytes(sharedArr.buffer, aesSalt.buffer, hmacSalt.buffer, info);
            return keys;
        }
    };
}