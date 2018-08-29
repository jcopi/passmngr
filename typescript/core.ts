interface pw_record {
    user: string,
    pass: string
};

class pw_manager
{
    static buffer_to_b64 (buffer: ArrayBuffer): string
    {
        let result = "";
        let encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        let bytes = new Uint8Array(buffer);
        let nbytes = buffer.byteLength;
        let remainder = nbytes % 3;
        let mainlen = nbytes - remainder;

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
    static b64_to_buffer (b64: string): ArrayBuffer
    {
        let encodings = {"0":52,"1":53,"2":54,"3":55,"4":56,"5":57,"6":58,"7":59,"8":60,"9":61,"A":0,"B":1,"C":2,"D":3,"E":4,"F":5,
			"G":6,"H":7,"I":8,"J":9,"K":10,"L":11,"M":12,"N":13,"O":14,"P":15,"Q":16,"R":17,"S":18,"T":19,"U":20,"V":21,"W":22,"X":23,"Y":24,"Z":25,"a":26,"b":27,"c":28,"d":29,"e":30,"f":31,"g":32,"h":33,"i":34,"j":35,"k":36,"l":37,"m":38,"n":39,"o":40,
            "p":41,"q":42,"r":43,"s":44,"t":45,"u":46,"v":47,"w":48,"x":49,"y":50,"z":51,"+":62,"/":63};
        
        let remainder = b64.length % 4;
        if (remainder != 2 && remainder != 3)
            throw "Invalid padding on  b64 string";
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

    static str_to_buffer (str: string): ArrayBuffer
    {
        let chars = new Uint16Array(str.length);
        for (let i = str.length; i--; chars[i] = str.charCodeAt(i));
        return chars.buffer;
    }
    static buffer_to_str (buffer: ArrayBuffer): string
    {
        let chars = new Uint16Array(buffer);
        let str = "";
        chars.forEach(v => str += String.fromCharCode(v));
        return str;
    }

    static gen_random (nbytes: number): ArrayBuffer
    {
        let bytes = new Uint8Array(nbytes);
        crypto.getRandomValues(bytes);
        return bytes.buffer;
    }

    master: CryptoKey;
    aeskey: CryptoKey;
    hmackey: CryptoKey;
    salt: ArrayBuffer;
    local: ArrayBuffer;

    async set_keys (master: string, salt: ArrayBuffer): Promise<void>
    {
        this.salt = salt;
        
        this.master = await crypto.subtle.importKey(
            "raw",
            pw_manager.str_to_buffer(master),
            "PBKDF2",
            false,
            ["deriveKey", "deriveBits"]
        );

        this.aeskey = await crypto.subtle.deriveKey(
            {"name":"PBKDF2", "iterations":1007, "salt": this.salt, "hash":"SHA-256"},
            this.master,
            {"name":"AES-CBC", "length":256},
            false,
            ["encrypt", "decrypt"]
        );

        this.hmackey = await crypto.subtle.deriveKey(
            {"name":"PBKDF2", "iterations":1007, "salt": this.salt, "hash":"SHA-256"},
            this.master,
            {"name":"HMAC", "hash":"SHA-256"},
            false,
            ["sign", "verify"]
        );
    }

    async sign_message (message: ArrayBuffer): Promise<ArrayBuffer>
    {
        return await crypto.subtle.sign(
            "HMAC",
            this.hmackey,
            message
        );
    }
    async verify_message (message: ArrayBuffer, signature: ArrayBuffer): Promise<boolean>
    {
        return await crypto.subtle.verify(
            "HMAC",
            this.hmackey,
            signature,
            message
        );
    }
    async encrypt_message (plain: ArrayBuffer): Promise<ArrayBuffer>
    {
        let iv = pw_manager.gen_random(16);
        let ct = await crypto.subtle.encrypt(
            {"name":"AES-CBC", "length":256, "iv":iv},
            this.aeskey,
            plain
        );
        let result = new Uint8Array(iv.byteLength + ct.byteLength);
        result.set(new Uint8Array(iv), 0);
        result.set(new Uint8Array(ct), iv.byteLength);

        return result.buffer
    }
    async decrypt_message (cipher: ArrayBuffer): Promise<ArrayBuffer>
    {
        let iv = cipher.slice(0, 16);
        let ct = cipher.slice(16);

        let result = crypto.subtle.decrypt(
            {"name":"AES-CBC", "length":256, "iv":iv},
            this.aeskey,
            ct
        );

        return result;
    }

    async load_record_file (file: ArrayBuffer, signature: ArrayBuffer): Promise<Map<string, pw_record[]>>
    {
        let valid = await this.verify_message(file, signature);
        if (!valid)
            throw "File corrupted";
        let plain = await this.decrypt_message(file);
        let plain_str = pw_manager.buffer_to_b64(plain);
        let result: Map<string, pw_record[]> = new Map();
        try {
            let obj = JSON.parse(plain_str);
            let carr: pw_record[];
            for (let key in obj) {
                if (Array.isArray(obj[key])) {
                    carr = [];
                    obj[key].forEach(v => {
                        if (v.hasOwnProperty("user") && v.hasOwnProperty("pass") && typeof v.user === "string" && typeof v.pass === "string") {
                            carr.push({
                                "user": v.user,
                                "pass":v.pass
                            });
                        }
                    });
                    result.set(key, carr);
                }
            }
        } catch (ex) {
            throw "Invalid file";
        }

        return result;
    }
    async save_record_file (records: Map<string, pw_record[]>): Promise<[ArrayBuffer, ArrayBuffer]>
    {
        let obj = {};
        for (let [k,v] of records) {
            obj[k] = v;
        }
        let str = JSON.stringify(obj);
        let plain = pw_manager.str_to_buffer(str);

        let cipher = await this.encrypt_message(plain);
        let signature = await this.sign_message(cipher);

        return [cipher, signature];
    }
}