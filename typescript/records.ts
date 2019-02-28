import * as Core from "./core.js";

export interface password_t {
    identity  : string;
    blockID   : number;
    charLength: number;
    dictID    : number;
};

export interface record_t {
    domain   : string;
    appID    : string;
    tags     : string[];
    passwords: password_t[];
    formData : Map<string, string>;
};

export class PassMNGR
{
    static DoesDomainMatch (domain: string, domainPattern: string): boolean {
        return false;
    }

    static GeneratePassword (buffer: ArrayBuffer, dictionary: string): string {
        let result      = "";
        let addressable = new Uint32Array(buffer);

        for (let i = 0; i < addressable.length; ++i) {
            result += dictionary[addressable[i] % dictionary.length];
        }

        return result;
    }

    static PASSWORD_BLOCK_SIZE = 256 / 8; // 256bit Blocks
    static BYTES_PER_CHARACTER = 32 / 8;  // Each character generated from a 32bit uint
    
    crypto      : Core.Crypto;
    passBlocks  : ArrayBuffer;
    recordBlocks: ArrayBuffer;
    records     : record_t[];
    dictionaries: string[];
    

    FindRecordsFromDomain (domain: string): record_t[] {
        return this.records.filter(PassMNGR.DoesDomainMatch.bind(null, domain));
    }

    FindRecordsFromAppID (appID: string): record_t[] {
        return this.records.filter((v) => v.appID === appID);
    }

    FindRecordsFromTags (tags: string[]): record_t[] {
        return this.records.filter((v) => {
            return tags.reduce((previous, tag) => {
                return previous && (v.tags.indexOf(tag) >= 0);
            }, true);
        });
    }

    async GetPassword (object: password_t): Promise<string> {
        let decrypted   = await this.crypto.decrypt(this.passBlocks);
        let addressable = new Uint32Array(decrypted);

        let startingBlk = object.blockID * PassMNGR.PASSWORD_BLOCK_SIZE;
        let byteCount   = object.charLength * PassMNGR.BYTES_PER_CHARACTER;
        let blockCount  = Math.ceil(byteCount / PassMNGR.PASSWORD_BLOCK_SIZE);

        let passwordBlk = addressable.slice(startingBlk, startingBlk + blockCount);
        let passwordStr = PassMNGR.GeneratePassword(passwordBlk.buffer, this.dictionaries[object.dictID]);

        if (passwordStr.length < object.charLength) {
            throw "Error getting password from file";
        }

        return passwordStr.substr(0, object.charLength);
    }

    
}