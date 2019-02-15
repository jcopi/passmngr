import * as Core from "./core.js";

export interface password_t {
    identity  : string;
    blockID   : number;
    charLength: number;
    dictID    : number | string;
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
    crypto: Core.Crypto;

}