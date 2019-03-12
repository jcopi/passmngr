import * as Utils from "./utils.js"

module PassMNGR {
    class Constants {
        static PASSWORD_BLOCK_SIZE  = (256/8);
        static BYTES_PER_CHARACTER  = (32 /8);

        static NUMBERS_DICTIONARY   = "0123456789";
        static SYMBOLS_DICTIONARY   = "`,./;'[]\\=-<>?:\"{}|+_)(*&^%$#@!~)";
        static UPPERCASE_DICTIONARY = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        static LOWERCASE_DICTIONARY = "abcdefghijklmnopqrstuvwxyz";
    }

    class Login {
        domain  : Domain;

        identity: string;
        blockID : number;
        length  : number;
        dictID  : number;

        constructor (identity: string) {

        }

        IdentityMatches (ident: string): boolean {
            return this.identity == ident;
        }
    }

    class Domain {
        manager   : Core;

        isDomain  : boolean;
        identifier: string;
        tags      : string[];
        formData  : Map<string, string>;
        logins    : Login[];

        constructor (ident: string, isDomain: boolean = true) {
            this.isDomain = isDomain;
            this.identifier = ident;
            this.tags = [];
            this.formData = new Map();
            this.logins = [];
        }

        GetLogins (identity?: string): Login[] {
            if (identity === undefined) {
                return this.logins.slice();
            }

            return this.logins.filter(v => v.IdentityMatches(identity));
        }

        GetLogin (identity?: string): Login {
            let result: Login = null;
            
            if (identity === undefined && this.logins.length > 0) {
                result = this.logins[0];
            } else {
                let i = 0;
                while (i < this.logins.length && result == null) {
                    if (this.logins[i].IdentityMatches(identity)) {
                        result = this.logins[i];
                    }
                }
            }

            return result;
        }

        RemoveLogin (identity?: string): Login {
            
        }

        AddLogin (obj: Login) {

        }
    }

    class Core {

    }
}