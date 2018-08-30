namespace PassManager {
    class Base64 {
        static serlialize (buffer: ArrayBuffer): string {
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

        static parse (b64: string): ArrayBuffer {
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

    class StringEncoding {
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
        //static toUTF16 (str: string): ArrayBuffer {}
        static toUTF32 (str: string): ArrayBuffer {
            let result = new Uint32Array(str.length);
            for (let i = 0; i < str.length; ++i) {
                result[i] = str.codePointAt(i);
            }
            return result.buffer;
        }
    }

    class Crypto {
        static randomBytes (count:number): ArrayBuffer {
            let bytes = new Uint8Array(count);
            crypto.getRandomValues(bytes);
            return bytes.buffer;
        }

        master: CryptoKey = null;
        salt: ArrayBuffer = null;

        initialized: boolean = false;

        AESKey: CryptoKey = null;
        HMACKey: CryptoKey = null;

        constructor (raw: string)
        constructor (raw: string, salt?: ArrayBuffer) {
            if (salt === undefined)
                salt = Crypto.randomBytes(16);
            this.salt = salt;
            
            let te = new TextEncoder();
            console.log(te.encode(raw));
            console.log(StringEncoding.toUTF8(raw));
            //this.master = await crypto.importKey("raw", TextEncoder())
        }
    }
}