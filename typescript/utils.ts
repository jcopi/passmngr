export class bufferEncoding {
    static toBase64 (buffer: ArrayBuffer): string {
        let result = "";
        let encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        let padding  = "=";

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
            result += encoding[(word & 0b1111110000000000) >> 10];
            result += encoding[(word & 0b0000001111110000) >>  4];
            result += encoding[(word & 0b0000000000001111) <<  2];
            result += padding;
        } else if (remainder == 1) {
            let word = bytes[mainlen];
            result += encoding[(word & 0b11111100) >> 2];
            result += encoding[(word & 0b00000011) << 4];
            result += padding;
            result += padding;
        }

        return result;
    }
    static fromBase64 (b64: string): ArrayBuffer {
        let encodings = {"0":52,"1":53,"2":54,"3":55,"4":56,"5":57,"6":58,"7":59,"8":60,"9":61,"A":0,"B":1,"C":2,"D":3,"E":4,"F":5,
		"G":6,"H":7,"I":8,"J":9,"K":10,"L":11,"M":12,"N":13,"O":14,"P":15,"Q":16,"R":17,"S":18,"T":19,"U":20,"V":21,"W":22,"X":23,"Y":24,"Z":25,"a":26,"b":27,"c":28,"d":29,"e":30,"f":31,"g":32,"h":33,"i":34,"j":35,"k":36,"l":37,"m":38,"n":39,"o":40,
        "p":41,"q":42,"r":43,"s":44,"t":45,"u":46,"v":47,"w":48,"x":49,"y":50,"z":51,"+":62,"/":63};
    
        let remainder = b64.length % 4;
        if (remainder > 0)
            throw "Invalid padding on base64 string";
        let lastChunkBCnt = (b64.endsWith("=") ? (b64.endsWith("==") ? 1 : 2) : 0);
        let mainlen = b64.length - (lastChunkBCnt === 0 ? 0 : 4);
        let nbytes = ((mainlen / 4) * 3) + lastChunkBCnt;
        let bytes = new Uint8Array(nbytes);

        let j = 0;
        for (let i = 0; i < mainlen; i += 4) {
            let word = (encodings[b64[i]] << 18) | (encodings[b64[i+1]] << 12) | (encodings[b64[i+2]] << 6) | encodings[b64[i+3]];
            bytes[j++] = (word & 0b111111110000000000000000) >> 16;
            bytes[j++] = (word & 0b000000001111111100000000) >> 8;
            bytes[j++] = (word & 0b000000000000000011111111);
        }

        if (lastChunkBCnt === 1) {
            let word = (encodings[b64[mainlen]] << 2) | (encodings[b64[mainlen+1]] >> 4);
            bytes[j++] = word;
        } else if (lastChunkBCnt === 2) {
            let word = (encodings[b64[mainlen]] << 10) | (encodings[b64[mainlen+1]] << 4) | (encodings[b64[mainlen+2]] >>2);
            bytes[j++] = (word & 0b1111111100000000) >> 8;
            bytes[j++] = (word & 0b0000000011111111);
        }

        return bytes.buffer;
    }
}

export type BERObject = {
    class: number;
    tag: number;
    tagName: string;
    structured: boolean;
    rawContents: ArrayBuffer;
    contents: BERObject[];
    length: number;
}

export class berEncoding {
    static TAG_BOOLEAN             : number = 1;
    static TAG_INTEGER             : number = 2;
    static TAG_BIT_STRING          : number = 3;
    static TAG_OCTET_STRING        : number = 4;
    static TAG_NULL                : number = 5;
    static TAG_OBJECT_IDENTIFIER   : number = 6;
    static TAG_OBJECT_DESCRIPTOR   : number = 7;
    static TAG_INSTANCE_OF_EXTERNAL: number = 8;
    static TAG_REAL                : number = 9;
    static TAG_ENUMERATED          : number = 10;
    static TAG_EMBEDDED_PDV        : number = 11;
    static TAG_UTF8_STRING         : number = 12;
    static TAG_RELATIVE_OID        : number = 13;
    static TAG_SEQUENCE_OF         : number = 16;
    static TAG_SET_OF              : number = 17;
    static TAG_NUMERIC_STRING      : number = 18;
    static TAG_PRINTABLE_STRING    : number = 19;
    static TAG_T61_STRING          : number = 20;
    static TAG_VIDEOTEX_STRING     : number = 21;
    static TAG_IA5_STRING          : number = 22;
    static TAG_UTC_TIME            : number = 23;
    static TAG_GENERALIZED_TIME    : number = 24;
    static TAG_GRAPHIC_STRING      : number = 25;
    static TAG_ISO646_STRING       : number = 26;
    static TAG_GENERAL_STRING      : number = 27;
    static TAG_UNIVERSAL_STRING    : number = 28;
    static TAG_CHARACTER_STRING    : number = 29;
    static TAG_BMP_STRING          : number = 30;

    static TAG_NAMES = {1: "BOOLEAN",2: "INTEGER",3: "BIT_STRING",4: "OCTET_STRING",5: "NULL",6: "OBJECT_IDENTIFIER",7: "OBJECT_DESCRIPTOR",8: "_INSTANCE_OF_EXTERNAL",9: "_REAL",10: "_ENUMERATED",11: "_EMBEDDED_PDV",12: "_UTF8_STRING",13: "_RELATIVE_OID",16: "_SEQUENCE_OF",17: "_SET_OF",18: "_NUMERIC_STRING",19: "_PRINTABLE_STRING",20: "_T61_STRING",21: "_VIDEOTEX_STRING",22: "_IA5_STRING",23: "_UTC_TIME",24: "_GENERALIZED_TIME",25: "_GRAPHIC_STRING",26: "_ISO646_STRING",27: "_GENERAL_STRING",28: "UNIVERSAL_STRING",29: "CHARACTER_STRING",30: "BMP_STRING"}

    static fromBer (data: ArrayBuffer): BERObject[] {
        let dataArr = new Uint8Array(data);
        let result: BERObject[] = [];

        let i = 0; // a buffer could contain many BER encoded objects
        while (i < dataArr.length) {
            // The first 2 bits of the first byte are an objects' class
            let cls = (dataArr[i] & 0b11000000) >> 6;
            // The 3rd bit indicated whether the data is structured
            let structured = (dataArr[i] & 0b00100000) === 0b00100000;
            // The next 5+ bytes represent the object's tag
            let tag = (dataArr[i] & 0b00011111);
            if (tag === 0b00011111) {
                // If the first 5 tag bits are all 1 the tag is made up of the following bytes
                // until a byte with a leading 0 is found. 
                tag = 0;
                for (i++; i < dataArr.length; i++) {
                    tag *= 128;
                    tag += Number(dataArr[i] & 0b01111111);
                    if ((dataArr[i] & 0b10000000) !== 0b10000000) {
                        i++;
                        break;
                    }
                }
                if (i >= dataArr.length) throw "Invalid BER format not enough bytes";
                if (!Number.isSafeInteger(tag)) throw "Tag too large";
            } else {
                i++;
            }

            let length = dataArr[i];
            if (length === 0b10000000) {
                // The length is unknown, so it must be found while parsing the contents
                // it will be set to '-1' here and dealt with later
                length = -1;
                i++;
            } else if (length > 128) {
                let lenBytes = (length & 0b01111111);
                length = 0;
                for (i++; i < dataArr.length && lenBytes-- > 0; i++) {
                    length *= 256;
                    length += dataArr[i];
                }
                if (i >= dataArr.length) throw "Invalid BER format not enough bytes";
                if (!Number.isSafeInteger(length)) throw "Length too large"; 
            } else {
                i++;
            }

            let raw: Uint8Array = null;
            if (length <= 0) {
                // Content will end at the first set of adjacent 0 bytes, length is unknown.
                let cnt = [];
                while (i + 1 < dataArr.length) {
                    if (dataArr[i] === 0 && dataArr[i + 1] === 0) {
                        i += 2;
                        break;
                    } else {
                        cnt.push(dataArr[i]);
                        i++;
                    }
                }
                raw = new Uint8Array(cnt);
                length = raw.length;
            } else {
                raw = dataArr.slice(i, i + length);
                i += length;
            }

            let content = null;
            if (structured) {
                content = berEncoding.fromBer(raw);
            }

            let item: BERObject = {class: cls, tag: tag, tagName:(tag in berEncoding.TAG_NAMES ? berEncoding.TAG_NAMES[tag] : "TAG_UNKNOWN"), structured: structured, rawContents: raw, contents: content, length: length};
            result.push(item);
        }



        return result;
    }
}

export class stringEncoding {
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

export class algos {
    static smallPrimes: Uint32Array = new Uint32Array([2,3,5,7,11,13,17,19,23,29]);
    static primeIndices: Uint32Array = new Uint32Array([1,7,11,13,17,19,23,29]);
    static isPrime (y: number): boolean {
        let u32 = new Uint32Array(4);
        u32[2] = y;

        // Check all of the small primes
        for (let i = 0; i < algos.smallPrimes.length; ++i) {
            u32[1] = algos.smallPrimes[i];
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

    static lowerBound (list: ArrayLike<any>, value: any): number {
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

    static nextPrime (n: number): number {
        const small_primes = algos.smallPrimes;
        const indices = algos.primeIndices;

        const L = 30;
        const N = algos.smallPrimes.length;
        // If n is small enough, search in small_primes
        if (n <= algos.smallPrimes[N-1]) {
            return algos.smallPrimes[algos.lowerBound(algos.smallPrimes, n)];
        }
        // Else n > largest small_primes
        // Start searching list of potential primes: L * k0 + indices[in]
        const M = algos.primeIndices.length;
        // Select first potential prime >= n
        //   Known a-priori n >= L
        let k0 = (n / L) | 0; // Coerce to Uint32
        let inn = algos.lowerBound(algos.primeIndices, n - k0 * L);
        n = L * k0 + algos.primeIndices[inn];
        while (!algos.isPrime(n))
        {
            if (++inn == M)
            {
                ++k0;
                inn = 0;
            }
            n = L * k0 + algos.primeIndices[inn];
        }
        return n;
    }
}

export type SymmetricKeyPair = {
    aes: CryptoKey;
    hmac: CryptoKey;
}

export type AsymmetricKeyPair = {
    public: CryptoKey;
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
        static PBKDF_ITERATIONS: number = algos.nextPrime(4194368);
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
                rawKey = await window.crypto.subtle.importKey("raw", stringEncoding.toUTF8(password), "PBKDF2", false, ["deriveBits"]);
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
                throw "1"
                return null;
            }
            
            let aesKey: CryptoKey = null;
            try {
                let aesHkdfParams = {name: "HKDF", info: info, salt: aesSalt, hash: symmetricCrypto.HKDF_HASH};
                let aesParams = {name: "AES-CBC", length: symmetricCrypto.AES_KEY_LENGTH};
                aesKey = await window.crypto.subtle.deriveKey(aesHkdfParams as any, aesRawKey, aesParams, false, ["encrypt", "decrypt"]);
            } catch (ex) {
                throw "2"
                return null;
            }
    
            let hmacKey: CryptoKey = null;
            try {
                let hmacHkdfParams = {name: "HKDF", info: info, salt: hmacSalt, hash: symmetricCrypto.HKDF_HASH};
                let hmacParams = {name: "HMAC", hash: symmetricCrypto.HMAC_HASH};
                hmacKey = await window.crypto.subtle.deriveKey(hmacHkdfParams as any, hmacRawKey, hmacParams, false, ["sign", "verify"]);
            } catch (ex) {
                throw "3"
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

        static async sign (keys: SymmetricKeyPair, data: ArrayBuffer): Promise<ArrayBuffer> {
            let signature: ArrayBuffer = null;
            try {
                signature = await window.crypto.subtle.sign("HMAC", keys.hmac, data);
            } catch (ex) {
                return null;
            }
            
            return signature;
        }

        static async verify (keys: SymmetricKeyPair, data: ArrayBuffer, signature: ArrayBuffer): Promise<boolean> {
            let verified: boolean = false;
            try {
                verified = await window.crypto.subtle.verify("HMAC", keys.hmac, signature, data);
            } catch (ex) {
                return false;
            }

            return verified;
        }
    };

    static asymmetric = class asymmetricCrypto {
        static ecdh = class ecdhCrypto {
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

                let salt = crypto.randomBytes(ecdhCrypto.ECDH_SALT_BYTE_CNT);
                if (salt === null) return null;

                return {public: rawKeyPair.publicKey, private: rawKeyPair.privateKey, salt: salt};
            }

            static async marshallPublicKey (keys: AsymmetricKeyPair, info: ArrayBuffer): Promise<ArrayBuffer> {
                let extractedPublic: ArrayBuffer = null;
                try {
                    extractedPublic = await window.crypto.subtle.exportKey("raw", keys.public);
                } catch (ex) {
                    return null;
                }
                
                let saltArr = new Uint8Array(keys.salt);
                let publicArr = new Uint8Array(extractedPublic);
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

                let rawPublic: CryptoKey = null;
                try {
                    let ecdhParams = {name:"ECDH", namedCurve:"P-521"};
                    rawPublic = await window.crypto.subtle.importKey("raw", publicKey, ecdhParams, false, ["deriveBits"])
                } catch (ex) {
                    return null;
                }

                return [{private: null, public: rawPublic, salt: salt}, info]
            }

            static async deriveSharedSymmetricKeys (personalKeys: AsymmetricKeyPair, othersKeys: AsymmetricKeyPair, info: ArrayBuffer, serverRole: boolean): Promise<SymmetricKeyPair> {
                if (personalKeys.private === null || personalKeys.public === null || personalKeys.salt === null) 
                    return null;
                if (othersKeys.public === null || othersKeys.salt === null || othersKeys.private !== null)
                    return null;

                let sharedBits: ArrayBuffer = null;
                try {
                    let ecdhParams = {name:"ECDH", namedCurve:"P-521", public:othersKeys.public};
                    sharedBits = await window.crypto.subtle.deriveBits(ecdhParams, personalKeys.private, ecdhCrypto.ECDH_SHARED_LENGTH);
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
        }

        static rsa = class rsaCrypto {
            static RSA_SIGNATURE_HASH: string = "SHA-512";
            static RSA_SALT_BYTE_CNT: number = 64;

            static async generateKeyPair (): Promise<AsymmetricKeyPair> {
                let rawKeyPair: CryptoKeyPair = null;
                try {
                    let rsaPssParams = {name: "RSA-PSS", modulusLength: 4096, publicExponent: new Uint8Array([1,0,1]), hash: rsaCrypto.RSA_SIGNATURE_HASH};
                    rawKeyPair = await window.crypto.subtle.generateKey(rsaPssParams, false, ["sign", "verify"]);
                } catch (ex) {
                    return null;
                }

                return {public: rawKeyPair.publicKey, private: rawKeyPair.privateKey, salt: null};
            }

            static async sign (keys: AsymmetricKeyPair, data: ArrayBuffer): Promise<ArrayBuffer> {
                let signature: ArrayBuffer = null;
                try {
                    let rsaPssParams = {name: "RSA-PSS", saltLength: rsaCrypto.RSA_SALT_BYTE_CNT};
                    signature = await window.crypto.subtle.sign(rsaPssParams, keys.private, data);
                } catch (ex) {
                    return null;
                }

                return signature;
            }

            static async verify (keys: AsymmetricKeyPair, data: ArrayBuffer, signature: ArrayBuffer): Promise<boolean> {
                let verified: boolean = false;
                try {
                    let rsaPssParams = {name: "RSA-PSS", saltLength: rsaCrypto.RSA_SALT_BYTE_CNT};
                    verified = await window.crypto.subtle.verify(rsaPssParams, keys.public, signature, data);
                } catch (ex) {
                    return false;
                }

                return verified;
            }
        }
    };
}

export namespace passmngr {
    enum calg {
        AES    = 0b00000001,
        CBC    = 0b00000010,
        GCM    = 0b00000100,
        CTR    = 0b00001000,
        HMAC   = 0b00010000,
        SHA256 = 0b00100000,
        SHA384 = 0b01000000,
        SHA512 = 0b10000000,

        ECDSA  = 0b00000001 << 8,
        P384   = 0b00000010 << 8,
        P521   = 0b00000100 << 8,
        ECDHE  = 0b00001000 << 8
    }

    enum cryptoAlgos {
        AES_CBC_256_HMAC_SHA256 = calg.AES | calg.CBC | calg.HMAC | calg.SHA256,
        AES_CBC_256_HMAC_SHA384 = calg.AES | calg.CBC | calg.HMAC | calg.SHA384,
        AES_CBC_256_HMAC_SHA512 = calg.AES | calg.CBC | calg.HMAC | calg.SHA512,
        AES_GCM_256_HMAC_SHA256 = calg.AES | calg.GCM | calg.HMAC | calg.SHA256,
        AES_GCM_256_HMAC_SHA384 = calg.AES | calg.GCM | calg.HMAC | calg.SHA384,
        AES_GCM_256_HMAC_SHA512 = calg.AES | calg.GCM | calg.HMAC | calg.SHA512,
        ECDHE_P521 = calg.ECDHE | calg.P384,
        ECDHE_P384 = calg.ECDHE | calg.P384,
        ECDSA_P521_SHA256 = calg.ECDSA | calg.P521 | calg.SHA256,
        ECDSA_P521_SHA384 = calg.ECDSA | calg.P521 | calg.SHA384,
        ECDSA_P521_SHA512 = calg.ECDSA | calg.P521 | calg.SHA512,
        ECDSA_P384_SHA256 = calg.ECDSA | calg.P384 | calg.SHA256,
        ECDSA_P384_SHA384 = calg.ECDSA | calg.P384 | calg.SHA384,
        ECDSA_P384_SHA512 = calg.ECDSA | calg.P384 | calg.SHA512
    }

    type cryptoSymKeySet = {
        algo: cryptoAlgos;
        sym: {
            encrypt: CryptoKey;
            sign: CryptoKey;
        };
        salt: ArrayBuffer;
    }

    type cryptoAsymKeySet = {
        algo: cryptoAlgos;
        asym: {
            public: CryptoKey;
            private: CryptoKey;
        };
        salt: ArrayBuffer;
    }

    type cryptoKeySet = cryptoSymKeySet | cryptoAsymKeySet;

    function isSymmetric(keySet: cryptoKeySet): keySet is cryptoSymKeySet {
        return (keySet as cryptoSymKeySet).sym !== undefined;
    }

    class crypto {

        static primitives = class cryptoPrimitives {
            static randomBytes (count: number): ArrayBuffer {
                let buffer = new Uint8Array(count);
                try {
                    window.crypto.getRandomValues(buffer);
                } catch (ex) {
                    return null;
                }
                return buffer;
            }

            static randomBits (count: number): ArrayBuffer {
                return this.randomBytes(Math.ceil(count / 8));
            }

            static smallPrimes: Uint32Array = new Uint32Array([2,3,5,7,11,13,17,19,23,29]);
            static primeIndices: Uint32Array = new Uint32Array([1,7,11,13,17,19,23,29]);
            static isPrime (y: number): boolean {
                let u32 = new Uint32Array(4);
                u32[2] = y;

                // Check all of the small primes
                for (let i = 0; i < algos.smallPrimes.length; ++i) {
                    u32[1] = algos.smallPrimes[i];
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

            static lowerBound (list: ArrayLike<any>, value: any): number {
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
        
            static nextPrime (n: number): number {
                const small_primes = algos.smallPrimes;
                const indices = algos.primeIndices;
        
                const L = 30;
                const N = algos.smallPrimes.length;
                // If n is small enough, search in small_primes
                if (n <= algos.smallPrimes[N-1]) {
                    return algos.smallPrimes[algos.lowerBound(algos.smallPrimes, n)];
                }
                // Else n > largest small_primes
                // Start searching list of potential primes: L * k0 + indices[in]
                const M = algos.primeIndices.length;
                // Select first potential prime >= n
                //   Known a-priori n >= L
                let k0 = (n / L) | 0; // Coerce to Uint32
                let inn = algos.lowerBound(algos.primeIndices, n - k0 * L);
                n = L * k0 + algos.primeIndices[inn];
                while (!algos.isPrime(n))
                {
                    if (++inn == M)
                    {
                        ++k0;
                        inn = 0;
                    }
                    n = L * k0 + algos.primeIndices[inn];
                }
                return n;
            }
        };

        static keygen = class keygenCrypto {
            static defaultPBKDF2Iterations: number = crypto.primitives.nextPrime(4194368);
            static defaultPBKDF2Hash: string = "SHA-512";
            static defaultHKDFHash: string = "SHA-256";
            static defaultAESLength: number = 256;
            static defaultSymBitLength: number = 512;
            static defaultEcdhSaltBitLength: number = 256;

            static async fromBits (algo: cryptoAlgos, bits: ArrayBuffer, salt: ArrayBuffer): Promise<cryptoKeySet> {
                if ((algo & calg.AES) && (algo & calg.HMAC)) {
                    // Symmetric encryption
                    let aesParams: AesKeyGenParams = null
                    if (algo & calg.CBC) {
                        aesParams = {name: "AES-CBC", length: keygenCrypto.defaultAESLength};
                    } else if (algo & calg.GCM) {
                        aesParams = {name: "AES-GCM", length: keygenCrypto.defaultAESLength};
                    } else {
                        return null;
                    }

                    let hmacParams: HmacKeyGenParams = null;
                    if (algo & calg.SHA256) {
                        hmacParams = {name: "HMAC", hash: "SHA-256"};
                    } else if (algo & calg.SHA384) {
                        hmacParams = {name: "HMAC", hash: "SHA-384"};
                    } else if (algo & calg.SHA512) {
                        hmacParams = {name: "HMAC", hash: "SHA-512"};
                    } else {
                        return null;
                    }

                    let saltArray = new Uint8Array(salt)
                    let aesSalt = saltArray.slice(0, saltArray.length >> 1);
                    let hmacSalt = saltArray.slice(saltArray.length >> 1);

                    let bitsArray = new Uint8Array(bits);
                    let aesBits = bitsArray.slice(0, bitsArray.length >> 1);
                    let hmacBits = bitsArray.slice(bitsArray.length >> 1);

                    let aesKey: CryptoKey = null;
                    let hmacKey: CryptoKey = null;

                    try {
                        let aesHkdfParams = {name: "HKDF", info: new ArrayBuffer(0), salt: aesSalt, hash: keygenCrypto.defaultHKDFHash};
                        let hmacHkdfParams = {name: "HKDF", info: new ArrayBuffer(0), salt: hmacSalt, hash: keygenCrypto.defaultHKDFHash};

                        let aesRawKg =  window.crypto.subtle.importKey("raw", aesBits, "HKDF", false, ["deriveKey"]);
                        let hmacRawKg =  window.crypto.subtle.importKey("raw", hmacBits, "HKDF", false, ["deriveKey"]);
                        let aesRaw = await aesRawKg;
                        let hmacRaw = await hmacRawKg;

                        let aesKg = window.crypto.subtle.deriveKey(aesHkdfParams as any, aesRaw, aesParams, false, ["encrypt", "decrypt"]);
                        let hmacKg = window.crypto.subtle.deriveKey(hmacHkdfParams as any, hmacRaw, hmacParams, false, ["sign", "verify"]);
                        aesKey = await aesKg;
                        hmacKey = await hmacKg;
                    } catch (ex) {
                        return null;
                    }

                    return {algo: algo, sym: {encrypt: aesKey, sign: hmacKey}, salt: null};
                } else {
                    return null;
                }
            }

            static async fromPassword (password: string, algo: cryptoAlgos, salt: ArrayBuffer, iteration: number = keygenCrypto.defaultPBKDF2Iterations): Promise<cryptoKeySet> {
                let rawKey: CryptoKey = null;
                let keyBits: ArrayBuffer = null;
                try {
                    rawKey = await window.crypto.subtle.importKey("raw", stringEncoding.toUTF8(password), "PBKDF2", false, ["deriveBits"]);
                    let pbkdf2Params = {name: "PBKDF2", iterations: this.defaultPBKDF2Iterations, salt: salt, hash: keygenCrypto.defaultPBKDF2Hash};
                    keyBits = await window.crypto.subtle.deriveBits(pbkdf2Params, rawKey, keygenCrypto.defaultSymBitLength)
                } catch (ex) {
                    keyBits = null;
                    return null;
                }
                
                return await keygenCrypto.fromBits(algo, keyBits, salt);
            }

            static async fromRandom (algo: cryptoAlgos): Promise<cryptoKeySet> {
                if (algo & calg.AES) {
                    let keyBits = crypto.primitives.randomBits(keygenCrypto.defaultSymBitLength);
                    let salt = crypto.primitives.randomBits(keygenCrypto.defaultSymBitLength);

                    return await keygenCrypto.fromBits(algo, keyBits, salt);
                } else if (algo & calg.ECDHE) {
                    let ecdhParams: EcKeyGenParams = null;
                    if (algo & calg.P384) {
                        ecdhParams = {name: "ECDH", namedCurve: "P-384"};
                    } else if (algo & calg.P521) {
                        ecdhParams = {name: "ECDH", namedCurve: "P-521"};
                    } else {
                        return null;
                    }

                    let rawKeyPair: CryptoKeyPair = null;
                    try {
                        rawKeyPair = await window.crypto.subtle.generateKey(ecdhParams, false, ["deriveKey", "deriveBits"])
                    } catch (ex) {
                        return null;
                    }

                    let salt = crypto.primitives.randomBits(keygenCrypto.defaultEcdhSaltBitLength);
                    if (salt === null) {
                        return null;
                    }

                    return {algo: algo, asym: {public: rawKeyPair.publicKey, private: rawKeyPair.privateKey}, salt: salt};
                } else if (algo & calg.ECDSA) {
                    let ecdsaParams: EcKeyGenParams = null;
                    if (algo & calg.P384) {
                        ecdsaParams = {name: "ECDSA", namedCurve: "P-384"};
                    } else if (algo & calg.P521) {
                        ecdsaParams = {name: "ECDSA", namedCurve: "P-521"};
                    } else {
                        return null;
                    }

                    let rawKeyPair: CryptoKeyPair = null;
                    try {
                        rawKeyPair = await window.crypto.subtle.generateKey(ecdsaParams, false, ["sign", "verify"])
                    } catch (ex) {
                        return null;
                    }

                    return {algo: algo, asym: {public: rawKeyPair.publicKey, private: rawKeyPair.privateKey}, salt: null};
                } else {
                    return null;
                }
            }
        };

        static symmetric = class symmetricCrypto {
            static aesCbcIvBitLength: number = 128;
            static aesGcmNonceBitLength: number = 96;
            static aesCbcIvByteLength: number = 16;
            static aesGcmNonceByteLength: number = 12;
            static aesGcmTagBitLength: number = 128;
            static sha256ByteLength: number = 32;
            static sha384ByteLength: number = 48;
            static sha512ByteLength: number = 64;

            static async encrypt (keys: cryptoKeySet, data: ArrayBuffer, iv?: ArrayBuffer): Promise<ArrayBuffer> {
                if (!isSymmetric(keys)) {
                    return null;
                }
                
                let cipherText: ArrayBuffer = null;
                let aesParams: AesCbcParams | AesGcmParams = null;

                if (keys.algo & calg.CBC) {
                    // CBC mode will require the addition of an HMAC signature
                    if (iv === undefined) {
                        iv = crypto.primitives.randomBits(symmetricCrypto.aesCbcIvBitLength);
                        if (iv === null) {
                            return null;
                        }
                    }

                    
                    aesParams = {name: "AES-CBC", iv:iv};
                } else if (keys.algo & calg.GCM) {
                    if (iv === undefined) {
                        iv = crypto.primitives.randomBits(symmetricCrypto.aesGcmNonceBitLength);
                        if (iv === null) {
                            return null;
                        }
                    }

                    aesParams = {name: "AES-GCM", iv: iv, tagLength: symmetricCrypto.aesGcmTagBitLength};
                }
                try {
                    cipherText = await window.crypto.subtle.encrypt(aesParams, keys.sym.encrypt, data);
                } catch (ex) {
                    return null;
                }

                let combined = new Uint8Array(iv.byteLength + cipherText.byteLength);
                combined.set(new Uint8Array(iv));
                combined.set(new Uint8Array(cipherText), iv.byteLength);

                let signature = await symmetricCrypto.sign(keys, combined.buffer);
                if (signature === null) {
                    return null;
                }

                let result = new Uint8Array(signature.byteLength + combined.length);
                result.set(new Uint8Array(signature));
                result.set(combined, signature.byteLength);

                return result;
            }

            static async decrypt (keys: cryptoKeySet, data: ArrayBuffer): Promise<ArrayBuffer> {
                if (!isSymmetric(keys)) {
                    return null;
                }

                let signature: ArrayBuffer = null;
                let iv: ArrayBuffer = null;
                let cipherText: ArrayBuffer = null;

                let aesParams: AesCbcParams | AesGcmParams = null;

                if (keys.algo & calg.SHA256) {
                    signature = data.slice(0, symmetricCrypto.sha256ByteLength);
                } else if (keys.algo & calg.SHA384) {
                    signature = data.slice(0, symmetricCrypto.sha384ByteLength);
                } else if (keys.algo & calg.SHA512) {
                    signature = data.slice(0, symmetricCrypto.sha512ByteLength);
                } else {
                    return null;
                }

                if (keys.algo & calg.CBC) {
                    iv = data.slice(signature.byteLength, signature.byteLength + symmetricCrypto.aesCbcIvByteLength);
                    aesParams = {name: "AES-CBC", iv: iv};
                } else if (keys.algo & calg.GCM) {
                    iv = data.slice(signature.byteLength, signature.byteLength + symmetricCrypto.aesGcmNonceByteLength);
                    aesParams = {name: "AES-GCM", iv: iv, tagLength: symmetricCrypto.aesGcmTagBitLength};
                } else {
                    return null;
                }

                cipherText = data.slice(signature.byteLength + iv.byteLength);
                let combined = new Uint8Array(iv.byteLength + cipherText.byteLength);
                combined.set(new Uint8Array(iv));
                combined.set(new Uint8Array(cipherText), iv.byteLength);

                if (await symmetricCrypto.verify(keys, signature, combined.buffer) !== true) {
                    return null;
                }

                let plainText: ArrayBuffer = null;
                try {
                    plainText = await window.crypto.subtle.decrypt(aesParams, keys.sym.encrypt, cipherText);
                } catch (ex) {
                    return null;
                }

                return plainText;
            }

            static async sign (keys: cryptoKeySet, data: ArrayBuffer): Promise<ArrayBuffer> {
                if (!isSymmetric(keys)) {
                    return null;
                }
                
                let signature: ArrayBuffer = null;
                try {
                    signature = await window.crypto.subtle.sign("HMAC", keys.sym.sign, data);
                } catch (ex) {
                    return null;
                }

                return signature;
            }

            static async verify (keys: cryptoKeySet, signature: ArrayBuffer, data: ArrayBuffer): Promise<boolean> {
                if (!isSymmetric(keys)) {
                    return null;
                }

                let verified: boolean = false;
                try {
                    verified = await window.crypto.subtle.verify("HMAC", keys.sym.sign, signature, data);
                } catch (ex) {
                    return false;
                }

                return verified;
            }
        };

        static asymmetric = class asymmetricCrypto {
            static ecdheBitsToDerive: number = 528;

            static async sign (keys: cryptoKeySet, data: ArrayBuffer): Promise<ArrayBuffer> {
                if (isSymmetric(keys)) {
                    return null;
                }

                let ecdsaParams: EcdsaParams = null;
                if (keys.algo & calg.SHA256) {
                    ecdsaParams = {name: "ECDSA", hash: "SHA-256"};
                } else if (keys.algo & calg.SHA384) {
                    ecdsaParams = {name: "ECDSA", hash: "SHA-384"};
                } else if (keys.algo & calg.SHA512) {
                    ecdsaParams = {name: "ECDSA", hash: "SHA-512"};
                }

                let signature: ArrayBuffer = null;
                try {
                    signature = await window.crypto.subtle.sign(ecdsaParams, keys.asym.private, data);
                } catch (ex) {
                    return null;
                }

                return signature;
            }

            static async verify (keys: cryptoKeySet, signature: ArrayBuffer, data: ArrayBuffer): Promise<boolean> {
                if (isSymmetric(keys)) {
                    return null;
                }

                let ecdsaParams: EcdsaParams = null;
                if (keys.algo & calg.SHA256) {
                    ecdsaParams = {name: "ECDSA", hash: "SHA-256"};
                } else if (keys.algo & calg.SHA384) {
                    ecdsaParams = {name: "ECDSA", hash: "SHA-384"};
                } else if (keys.algo & calg.SHA512) {
                    ecdsaParams = {name: "ECDSA", hash: "SHA-512"};
                }

                let verified: boolean = false;
                try {
                    verified = await window.crypto.subtle.verify(ecdsaParams, keys.asym.public, signature, data);
                } catch (ex) {
                    return false;
                }

                return verified;
            }

            static async keyExchangeMarshshallPublic (keys: cryptoKeySet): Promise<ArrayBuffer> {
                if (isSymmetric(keys)) {
                    return null;
                }

                if (keys.salt === null) {
                    keys.salt = new ArrayBuffer(0);
                }
                
                let extracted: ArrayBuffer = null;
                try {
                    extracted = await window.crypto.subtle.exportKey("raw", keys.asym.public);
                } catch (ex) {
                    return null;
                }

                let marshallBuffer = new Uint8Array(extracted.byteLength + keys.salt.byteLength + 4);
                marshallBuffer.set([(keys.salt.byteLength & 0xFF00) >> 8, keys.salt.byteLength & 0x00FF], 0);
                marshallBuffer.set(new Uint8Array(keys.salt), 2);
                marshallBuffer.set([(extracted.byteLength & 0xFF00) >> 8, extracted.byteLength & 0x00FF], 2 + keys.salt.byteLength);
                marshallBuffer.set(new Uint8Array(extracted), keys.salt.byteLength + 4);

                return marshallBuffer.buffer;
            }

            static async keyExchangeUnmarshallPublic (algo: cryptoAlgos, data: ArrayBuffer): Promise<cryptoKeySet> {
                if (!(algo & calg.ECDHE)) {
                    return null;
                }
                
                let dataArr = new Uint8Array(data);
                if (dataArr.length <= 2) {
                    return null;
                }
                let saltLength = ((dataArr[0] << 8) | dataArr[1]);
                if (dataArr.length < saltLength + 2) {
                    return null;
                }
                let salt = data.slice(2, 2 + saltLength);
                if (dataArr.length < saltLength + 4) {
                    return null;
                }
                let keyLength = ((dataArr[2 + saltLength] >> 8) | dataArr[3 + saltLength]);
                if (dataArr.length < saltLength + keyLength + 4) {
                    return null;
                }
                let key = data.slice(saltLength + 4, saltLength + keyLength + 4);

                let pubKey: CryptoKey = null;
                let ecdheKeyParams: EcKeyImportParams = null;
                if (algo & calg.P384) {
                    ecdheKeyParams = {name: "ECDH", namedCurve: "P-384"};
                } else if  (algo & calg.P521) {
                    ecdheKeyParams = {name: "ECDH", namedCurve: "P-521"};
                }

                try {
                    pubKey = await window.crypto.subtle.importKey("raw", key, ecdheKeyParams, false, ["deriveBits"]);
                } catch (ex) {
                    return null;
                }

                return {algo: algo, asym: {private: null, public: pubKey}, salt: salt};
            }

            static async keyExchangeDeriveSymmetricKeys (algo: cryptoAlgos, selfKeys: cryptoKeySet, otherKeys: cryptoKeySet): Promise<cryptoKeySet> {
                if (isSymmetric(selfKeys) || isSymmetric(otherKeys) || selfKeys.algo != otherKeys.algo || !(selfKeys.algo & calg.ECDHE)) {
                    return null;
                }

                if (!(algo & calg.AES) || !(algo & calg.HMAC)) {
                    return null;
                }

                let selfSaltArr = new Uint8Array(selfKeys.salt);
                let otherSaltArr = new Uint8Array(otherKeys.salt);
                let selfSaltFirst = true;
                for (let i = 0; i < Math.min(selfSaltArr.length, otherSaltArr.length); i++) {
                    if (selfSaltArr[i] > otherSaltArr[i]) {
                        selfSaltFirst = true;
                        break;
                    } else if (selfSaltArr[i] < otherSaltArr[i]) {
                        selfSaltFirst = false;
                        break;
                    }
                }


                let aesSalt: ArrayBuffer = null;
                let hmacSalt: ArrayBuffer = null;
                if (selfSaltFirst) {
                    aesSalt = selfKeys.salt;
                    hmacSalt = otherKeys.salt;
                } else {
                    aesSalt = otherKeys.salt;
                    hmacSalt = selfKeys.salt;
                }

                let combinedSalt = new Uint8Array(aesSalt.byteLength + hmacSalt.byteLength);
                combinedSalt.set(new Uint8Array(aesSalt));
                combinedSalt.set(new Uint8Array(hmacSalt), aesSalt.byteLength);
                
                let derivedBits: ArrayBuffer = null;
                let ecdheParams: EcdhKeyDeriveParams = null;
                ecdheParams = {name: "ECDH", public: otherKeys.asym.public};
                try {
                    derivedBits = await window.crypto.subtle.deriveBits(ecdheParams, selfKeys.asym.private, asymmetricCrypto.ecdheBitsToDerive);
                } catch (ex) {
                    return null;
                }

                return crypto.keygen.fromBits(algo, derivedBits, combinedSalt.buffer);
            }
        };
    }
}