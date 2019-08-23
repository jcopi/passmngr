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

declare global {
    interface Uint8Array {
        setBit(index: number, value: number): void;
        getBit(index: number): number;
    }
}

Uint8Array.prototype.setBit = function (index: number, value: number): void {
    let i = index >> 3;
    let j = 7 - (index & 0b111);
    value = value != 0 ? 1 : 0;

    if (i < this.length) {
        let num = 1 << j;
        this[i] &= ~num;
        this[i] |= (value << j) & num;
    }

    return;
}

Uint8Array.prototype.getBit = function (index: number): number {
    let i = index >> 3;
    let j = 7 - (index & 0b111);
    let value = 0;

    if (i < this.length) {
        value = (this[i] & (1 << j)) >> j;
    }

    return value;
}

type entropyEncodeModel = {
    decode: Uint8Array;
    encode: Map<number, number>;
}

export class compress {
    static jsonEntropyModel: entropyEncodeModel = {
        decode: new Uint8Array(stringEncoding.toUTF8("{:\",}/e sltrano-")),
        encode: new Map(["{",":","\"",",","}","/","e"," ","s","l","t","r","a","n","o","-"].map((v, i) => [v.charCodeAt(0),i]))
    };

    
    static entropyEncode (model: entropyEncodeModel, data: ArrayBuffer): ArrayBuffer {
        let input = new Uint8Array(data);
        let result = new Uint8Array(Math.ceil((9/8) * input.length));
        let j = 0;
        
        for (let i = 0; i < input.length; i++) {
            let replacement = model.encode.get(input[i]);
            if (replacement !== undefined) {
                result.setBit(j++, 1);
                result.setBit(j++, (replacement & 0b1000) >> 3);
                result.setBit(j++, (replacement & 0b0100) >> 2);
                result.setBit(j++, (replacement & 0b0010) >> 1);
                result.setBit(j++, replacement & 0b0001);
            } else {
                result.setBit(j++, 0);
                result.setBit(j++, (input[i] & 0b10000000) >> 7);
                result.setBit(j++, (input[i] & 0b01000000) >> 6);
                result.setBit(j++, (input[i] & 0b00100000) >> 5);
                result.setBit(j++, (input[i] & 0b00010000) >> 4);
                result.setBit(j++, (input[i] & 0b00001000) >> 3);
                result.setBit(j++, (input[i] & 0b00000100) >> 2);
                result.setBit(j++, (input[i] & 0b00000010) >> 1);
                result.setBit(j++,  input[i] & 0b00000001);
            }
        }

        return result.slice(0, (j >> 3) + Math.min(1, j & 0b111)).buffer;
    };

    static entropyDecode (model: entropyEncodeModel, data: ArrayBuffer): ArrayBuffer {
        let input = new Uint8Array(data);
        let result = new Uint8Array(Math.ceil((8/5) * input.length));
        let j = 0;

        for (let i = 0; i < input.length << 3;) {
            if (input.getBit(i++) === 1) {
                let num = 0;
                num |= input.getBit(i++) << 3;
                num |= input.getBit(i++) << 2;
                num |= input.getBit(i++) << 1;
                num |= input.getBit(i++);

                result[j++] = model.decode[num];
            } else {
                if (i + 8 > input.length << 3) {
                    break;
                }

                let num = 0;
                num |= input.getBit(i++) << 7;
                num |= input.getBit(i++) << 6;
                num |= input.getBit(i++) << 5;
                num |= input.getBit(i++) << 4;
                num |= input.getBit(i++) << 3;
                num |= input.getBit(i++) << 2;
                num |= input.getBit(i++) << 1;
                num |= input.getBit(i++);

                result[j++] = num;
            }
        }

        return result.slice(0, j).buffer;
    }
}

export namespace passmngr {
    export enum calg {
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

    export enum cryptoAlgos {
        AES_CBC_256_HMAC_SHA256 = calg.AES | calg.CBC | calg.HMAC | calg.SHA256,
        AES_CBC_256_HMAC_SHA384 = calg.AES | calg.CBC | calg.HMAC | calg.SHA384,
        AES_CBC_256_HMAC_SHA512 = calg.AES | calg.CBC | calg.HMAC | calg.SHA512,
        AES_GCM_256_HMAC_SHA256 = calg.AES | calg.GCM | calg.HMAC | calg.SHA256,
        AES_GCM_256_HMAC_SHA384 = calg.AES | calg.GCM | calg.HMAC | calg.SHA384,
        AES_GCM_256_HMAC_SHA512 = calg.AES | calg.GCM | calg.HMAC | calg.SHA512,
        ECDHE_P521 = calg.ECDHE | calg.P521,
        ECDHE_P384 = calg.ECDHE | calg.P384,
        ECDSA_P521_SHA256 = calg.ECDSA | calg.P521 | calg.SHA256,
        ECDSA_P521_SHA384 = calg.ECDSA | calg.P521 | calg.SHA384,
        ECDSA_P521_SHA512 = calg.ECDSA | calg.P521 | calg.SHA512,
        ECDSA_P384_SHA256 = calg.ECDSA | calg.P384 | calg.SHA256,
        ECDSA_P384_SHA384 = calg.ECDSA | calg.P384 | calg.SHA384,
        ECDSA_P384_SHA512 = calg.ECDSA | calg.P384 | calg.SHA512
    }

    export type cryptoSymKeySet = {
        algo: cryptoAlgos;
        sym: {
            encrypt: CryptoKey;
            sign: CryptoKey;
        };
        salt: ArrayBuffer;
    }

    export type cryptoAsymKeySet = {
        algo: cryptoAlgos;
        asym: {
            public: CryptoKey;
            private: CryptoKey;
        };
        salt: ArrayBuffer;
    }

    export type cryptoKeySet = cryptoSymKeySet | cryptoAsymKeySet;

    export function isSymmetric(keySet: cryptoKeySet): keySet is cryptoSymKeySet {
        return (keySet as cryptoSymKeySet).sym !== undefined;
    }

<<<<<<< HEAD
    class crypto {
=======
    export class crypto {

>>>>>>> 32656f7180733246170318c6d1a51d9d38d545ba
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
                for (let i = 0; i < cryptoPrimitives.smallPrimes.length; ++i) {
                    u32[1] = cryptoPrimitives.smallPrimes[i];
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
                const small_primes = cryptoPrimitives.smallPrimes;
                const indices = cryptoPrimitives.primeIndices;
        
                const L = 30;
                const N = cryptoPrimitives.smallPrimes.length;
                // If n is small enough, search in small_primes
                if (n <= cryptoPrimitives.smallPrimes[N-1]) {
                    return cryptoPrimitives.smallPrimes[cryptoPrimitives.lowerBound(cryptoPrimitives.smallPrimes, n)];
                }
                // Else n > largest small_primes
                // Start searching list of potential primes: L * k0 + indices[in]
                const M = cryptoPrimitives.primeIndices.length;
                // Select first potential prime >= n
                //   Known a-priori n >= L
                let k0 = (n / L) | 0; // Coerce to Uint32
                let inn = cryptoPrimitives.lowerBound(cryptoPrimitives.primeIndices, n - k0 * L);
                n = L * k0 + cryptoPrimitives.primeIndices[inn];
                while (!cryptoPrimitives.isPrime(n))
                {
                    if (++inn == M)
                    {
                        ++k0;
                        inn = 0;
                    }
                    n = L * k0 + cryptoPrimitives.primeIndices[inn];
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
            static ecdheBitsToDerive: number = 256;

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

            static async keyExchangeMarshallPublic (keys: cryptoKeySet): Promise<ArrayBuffer> {
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
                let keyLength = ((dataArr[2 + saltLength] << 8) | dataArr[3 + saltLength]);
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