export class BufferEncoding {
    static toBase64 (buffer: ArrayBuffer): string {
        let result = "";
        let encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        let bytes = new Uint8Array(buffer);
        let remainder = buffer.byteLength % 3;
        let mainlen = buffer.byteLength - remainder;

        for (let i = 0; i < mainlen; i += 3) {
            let word = (bytes[i] << 8) | (bytes[i+1] << 8) | bytes[i+2];
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

export class Crypto {
    static randomBytes (count:number): ArrayBuffer {
        let bytes = crypto.getRandomValues(new Uint8Array(count));
        return bytes.buffer;
    }

    static BASE_64_PASSWORD_DICT: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static BASE_98_PASSWORD_DICT: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

    static STANDARD_BIT_LENGTH: number = 256;
    static STANDARD_BLOCK_LENGTH: number = 16;
    static STANDARD_GCM_NONCE_LENGTH: number = 16;
    static STANDARD_HASH_ALGO: string = "SHA-256";
    static STANDARD_HASH_LENGTH: number = 32;
    static STANDARD_KDF_ITER_COUNT: number = 65537 * Math.pow(2, (new Date().getFullYear()) - 2012);

    master: CryptoKey = null;
    salt: ArrayBuffer = null;

    initialized: boolean = false;

    AESKey: CryptoKey = null;
    HMACKey: CryptoKey = null;

    constructor () {
        this.initialized = false;
    }

    async initKeys (raw: string, salt?: ArrayBuffer): Promise<void> {
        if (salt === undefined)
            salt = Crypto.randomBytes(Crypto.STANDARD_BLOCK_LENGTH);
        this.salt = salt;

        this.master = await crypto.subtle.importKey(
            "raw",
            StringEncoding.toUTF8(raw),
            "PBKDF2",
            false,
            ["deriveKey", "deriveBits"]
        );

        this.AESKey = await crypto.subtle.deriveKey(
            {"name":"PBKDF2", "iterations":Crypto.STANDARD_KDF_ITER_COUNT, "salt": this.salt, "hash":Crypto.STANDARD_HASH_ALGO},
            this.master,
            {"name":"AES-GCM", "length":Crypto.STANDARD_BIT_LENGTH},
            false,
            ["encrypt", "decrypt"]
        );

        this.HMACKey = await crypto.subtle.deriveKey(
            {"name":"PBKDF2", "iterations":991, "salt":this.salt, "hash":Crypto.STANDARD_HASH_ALGO},
            this.master,
            {"name":"HMAC", "hash":Crypto.STANDARD_HASH_ALGO},
            false,
            ["sign", "verify"]
        );

        this.initialized = true;
    }

    async encrypt (plain: ArrayBuffer): Promise<ArrayBuffer> {
        if (!this.initialized)
            throw "Crypto keys not initialized call initKeys first.";

        let iv = Crypto.randomBytes(Crypto.STANDARD_GCM_NONCE_LENGTH);
        let ct = await crypto.subtle.encrypt(
            {"name":"AES-GCM", "iv":iv, "tagLength":128},
            this.AESKey,
            plain
        );

        let result = new Uint8Array(iv.byteLength + ct.byteLength);
        result.set(new Uint8Array(iv), 0);
        result.set(new Uint8Array(ct), iv.byteLength);

        return result.buffer;
    }
    async decrypt (cipher: ArrayBuffer): Promise<ArrayBuffer> {
        if (!this.initialized)
            throw "Crypto keys not initialized call initKeys first.";

        let iv = cipher.slice(0,Crypto.STANDARD_GCM_NONCE_LENGTH);
        let ct = cipher.slice(Crypto.STANDARD_GCM_NONCE_LENGTH);
        let plain = await crypto.subtle.decrypt(
            {"name":"AES-GCM", "iv":iv, "tagLength":128},
            this.AESKey,
            ct
        );

        return plain;
    }
    async sign (message: ArrayBuffer): Promise<ArrayBuffer> {
        if (!this.initialized)
            throw "Crypto keys not initialized call initKeys first.";

        let result = new Uint8Array(message.byteLength + Crypto.STANDARD_HASH_LENGTH);
        result.set(new Uint8Array(message), Crypto.STANDARD_HASH_LENGTH);
        let signature = await crypto.subtle.sign(
            "HMAC",
            this.HMACKey,
            message
        );

        result.set(new Uint8Array(signature), 0);
        return result.buffer;
    }
    async verify (signed: ArrayBuffer): Promise<[boolean, ArrayBuffer]> {
        if (!this.initialized)
            throw "Crypto keys not initialized call initKeys first.";

        let signature = signed.slice(0, Crypto.STANDARD_HASH_LENGTH);
        let message = signed.slice(Crypto.STANDARD_HASH_LENGTH);
        let valid = await crypto.subtle.verify(
            "HMAC",
            this.HMACKey,
            signature,
            message
        );
        if (valid)
            return [true, message];

        return [false, null];
    }
}

