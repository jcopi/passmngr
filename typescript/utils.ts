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

export class Algos {
    static IsPrime (x: number): boolean {
        const small_primes = [2,3,5,7,11,13,17,19,23,29];
        const indices = [1,7,11,13,17,19,23,29];

        const N = small_primes.length;
        for (let i = 3; i < N; ++i)
        {
            const p = small_primes[i];
            const q = x / p;
            if (q < p)
                return true;
            if (x == q * p)
                return false;
        }
        for (let i = 31; true;)
        {
            let q = x / i;
            if (q < i)
                return true;
            if (x == q * i)
                return false;
            i += 4;

            q = x / i;
            i += 6;

            q = x / i;
            if (q < i)
                return true;
            if (x == q * i)
                return false;
            if (q < i)
                return true;
            if (x == q * i)
                return false;
            i += 2;

            q = x / i;
            if (q < i)
                return true;
            if (x == q * i)
                return false;
            i += 4;

            q = x / i;
            if (q < i)
                return true;
            if (x == q * i)
                return false;
            i += 2;

            q = x / i;
            if (q < i)
                return true;
            if (x == q * i)
                return false;
            i += 4;

            q = x / i;
            if (q < i)
                return true;
            if (x == q * i)
                return false;
            i += 6;

            q = x / i;
            if (q < i)
                return true;
            if (x == q * i)
                return false;
            i += 2;
        }
        return true;
    }

    static LowerBound (list: Array<any>, value: any): number {
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
        const small_primes = [2,3,5,7,11,13,17,19,23,29];
        const indices = [1,7,11,13,17,19,23,29];

        const L = 30;
        const N = small_primes.length;
        // If n is small enough, search in small_primes
        if (n <= small_primes[N-1]) {
            return small_primes[Algos.LowerBound(small_primes, n)];
        }
        // Else n > largest small_primes
        // Start searching list of potential primes: L * k0 + indices[in]
        const M = indices.length;
        // Select first potential prime >= n
        //   Known a-priori n >= L
        let k0 = n / L;
        let inn = Algos.LowerBound(indices, n - k0 * L);
        n = L * k0 + indices[inn];
        while (!Algos.IsPrime(n))
        {
            if (++inn == M)
            {
                ++k0;
                inn = 0;
            }
            n = L * k0 + indices[inn];
        }
        return n;
    }
}

export class Crypto {
    static kdfIterations: number = Algos.NextPrime(4194368);
    static standardHash: string = "SHA-256";
    static standardBitLength: number = 256;
    static gcmNonceLength: number = 16;
    static signatureLength: number = 32;

    static RandomBytes (byteCount: number): ArrayBuffer {
        let buffer = new Uint8Array(byteCount);
        window.crypto.getRandomValues(buffer);
        return buffer.buffer;
    }

    static async StringToRawKey (str: string): Promise<CryptoKey> {
        let key = await window.crypto.subtle.importKey("raw", StringEncoding.toUTF8(str), "PBKDF2", false, ["deriveKey", "deriveBits"]);
        return key;
    }

    static async RawKeyToSymmetricKeyPair (raw: CryptoKey, salt: ArrayBuffer): Promise<[CryptoKey,CryptoKey]> {
        let aes = await window.crypto.subtle.deriveKey({name:"PBKDF2", iterations:Crypto.kdfIterations, salt:salt, hash:Crypto.standardHash},
            raw, {name:"AES-GCM", length:Crypto.standardBitLength}, false, ["encrypt", "decrypt"]);
        let hmac = await window.crypto.subtle.deriveKey({name:"PBKDF2", iterations:Algos.NextPrime(Crypto.kdfIterations - 2048), salt:salt, hash:Crypto.standardHash},
            raw, {name:"HMAC", hash:Crypto.standardHash}, false, ["sign", "verify"]);

        return [aes, hmac];
    }

    static async DerivedKeyToSymmetricKeyPair (shared: ArrayBuffer, salt: ArrayBuffer, info: ArrayBuffer): Promise<[CryptoKey, CryptoKey]> {
        let raw = await window.crypto.subtle.importKey("raw", shared, "HKDF", true, ["deriveKey", "deriveBits"]);
    
        let aes = await window.crypto.subtle.deriveKey({name:"HKDF", hash:Crypto.standardHash, salt:salt, info:info},
            raw, {name:"AES-GCM", length:Crypto.standardBitLength}, false, ["encrypt", "decrypt"]);
        let hmac = await window.crypto.subtle.deriveKey({name:"HKDF", hash:Crypto.standardHash, salt:salt, info:info},
            raw, {name:"HMAC",hash:Crypto.standardHash,length:Crypto.standardBitLength}, false, ["sign", "verify"]);
        
        return [aes, hmac];
    }

    static async SymmetricEncrypt (keypair: [CryptoKey, CryptoKey], plaintext: ArrayBuffer): Promise<ArrayBuffer> {
        let aes = keypair[0];
        let hmac = keypair[1];
        try {
            let nonce = Crypto.RandomBytes(Crypto.gcmNonceLength);
            let cipher = await window.crypto.subtle.encrypt({name:"AES-GCM", iv:nonce, tagLength: Crypto.gcmNonceLength * 8},
                aes, plaintext);

            let temp = new Uint8Array(nonce.byteLength + cipher.byteLength);
            temp.set(new Uint8Array(nonce)); temp.set(new Uint8Array(cipher), nonce.byteLength);

            
            let signature = await window.crypto.subtle.sign("HMAC", hmac, temp.buffer);

            let result = new Uint8Array(signature.byteLength + temp.byteLength);
            result.set(new Uint8Array(signature)); result.set(temp, signature.byteLength);

            return result;
        } catch (ex) {
            throw "Encryption Error";
        }
    }

    static async SymmetricDecrypt (keypair: [CryptoKey, CryptoKey], ciphertext: ArrayBuffer): Promise<ArrayBuffer> {
        let aes = keypair[0];
        let hmac = keypair[1];

        let signature = ciphertext.slice(0, Crypto.signatureLength);
        let message = ciphertext.slice(Crypto.signatureLength);
        try {
            let valid = await window.crypto.subtle.verify("HMAC", hmac, signature, message);
            if (!valid) {
                throw "Decryption Error";
            }

            let nonce = message.slice(0, Crypto.gcmNonceLength);
            let cipher = message.slice(Crypto.gcmNonceLength);

            let plain = await window.crypto.subtle.decrypt({name:"AES-GCM", iv:nonce, tagLength:Crypto.gcmNonceLength * 8},
                aes, cipher);

            return plain;
        } catch (ex) {
            throw "Decryption Error";
        }
    }
}

