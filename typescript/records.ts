import * as Core from "./utils.js";

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

    static NUMBERS   = "0123456789";
    static SYMBOLS   = "`,./;'[]\\=-<>?:\"{}|+_)(*&^%$#@!~)";
    static UPPERCASE = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static LOWERCASE = "abcdefghijklmnopqrstuvwxyz";

    static DictCountRestriction (dict: string, count: number, input: string): boolean {
        input.split("").forEach((v) => {
            if (dict.includes(v)) {
                count--;
            }
        });

        return count <= 0;
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

    async CreateDomainRecord (): Promise<boolean> {
        return true;
    }

    async RemoveDomainRecord (): Promise<boolean> {
        return true;
    }

    async GetDomainRecord (): Promise<PassMNGR.DomainRecord> {
        return true;
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

    async NewRecord (length: number, dictionary: string, restrictions?:((string) => boolean)[]): record_t {
        if (restrictions === undefined) {
            restrictions = [];
        }

        let byteCount  = length * PassMNGR.BYTES_PER_CHARACTER;
        let blockCount = Math.ceil(byteCount / PassMNGR.PASSWORD_BLOCK_SIZE);
        let u32ByteCnt = 4 * (blockCount / PassMNGR.PASSWORD_BLOCK_SIZE);

        let randBlock   = Core.Crypto.randomBytes(u32ByteCnt);
        let addressable = new Uint32Array(randBlock);
        let passwordStr = PassMNGR.GeneratePassword(addressable, dictionary);

        passwordStr = passwordStr.substr(length);

        let passes = restrictions.reduce((a, b) => {
            return a && b(passwordStr);
        }, true);

        if (passes) {
            let record: record_t = {
                domain   : "",
                appID    : "",
                tags     : [],
                passwords: null,
                formData : null
            };

            let password: password_t = {
                identity  : "",
                blockID   : 0,
                charLength: 0,
                dictID    : 0
            };

            return passwordStr;
        } else {
            return this.NewRecord(length, dictionary, restrictions);
        }
    }
}