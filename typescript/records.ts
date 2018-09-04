import { StringEncoding } from "./build/core.js";

export class UserSettings {

}

interface URLP  {
    allowSubdomains: boolean;
    allowLongerPaths: boolean;
    domainPattern: string[];
    pathPattern: string[];
}

function isURLP (arg: any): arg is URLP {
    if (!("allowSubdomains" in arg) || typeof arg.allowSubdomains != "boolean")
        return false;
    if (!("allowLongerPaths" in arg) || typeof arg.allowLongerPaths != "boolean")
        return false;
    if (!("domainPattern" in arg) || !Array.isArray(arg.domainPattern) || (arg.domainPattern.length && typeof arg.domainPattern[0] != "string"))
        return false;
    if (!("pathPattern" in arg) || !Array.isArray(arg.pathPattern) || (arg.pathPattern.length && typeof arg.pathPattern[0] != "string"))
        return false;
    return true;
}

interface PMRecord {
    urlFilter: URLP;
    appID: string;
    tag: string;
    identity: string;
    password: string;
    formData: object;
}

function isPMRecord (arg: any): arg is PMRecord {
    if (!("urlFilter" in arg) || !isURLP(arg.urlFilter))
        return false;
    if (!("appID" in arg) || typeof arg.appID != "string")
        return false;
    if (!("tag" in arg) || typeof arg.tag != "string")
        return false;
    if (!("identity" in arg) || typeof arg.identity != "string")
        return false;
    if (!("password" in arg) || typeof arg.password != "string")
        return false;
    if (!("formData" in arg) || typeof arg.formData != "object")
        return false;
    return true;
}

export class URLPattern {
    internal: URLP;
    constructor (arg?: URLP) {
        if (typeof arg == "undefined" || !isURLP(arg))
            arg = {"allowSubdomains":false, "allowLongerPaths": false, "domainPattern":[], "pathPattern":[]};
        this.internal = arg;            
    }

    toObject (): URLP {
        return this.internal;
    }
    fromObject (obj: URLP): void {
        this.internal = obj;
    }

    setDomain (allowSubs: boolean, pattern: string[]): void {
        this.internal.allowSubdomains = allowSubs;
        this.internal.domainPattern = pattern;
    }

    setPath (allowLonger: boolean, pattern: string[]): void {
        this.internal.allowLongerPaths = allowLonger;
        this.internal.pathPattern = pattern;
    }

    testURL (url: string): boolean {
        let U;
        try {
            U = new URL(url);
        } catch (ex) {
            return false;
        }
        let domArr = U.hostname.split(".");
        let pathArr = U.pathname.split("/");

        if (this.internal.allowSubdomains) {
            if (this.internal.domainPattern.length > domArr.length)
                return false;
        } else {
            if (this.internal.domainPattern.length != domArr.length)
                return false;
        }
        for (let i = 1; i <= this.internal.domainPattern.length; ++i) {
            let j = this.internal.domainPattern.length - i;
            let k = domArr.length - i;
            if (this.internal.domainPattern[j] == "*")
                continue;
            if (this.internal.domainPattern[j] != domArr[k])
                return false;
        }

        if (this.internal.allowLongerPaths) {
            if (this.internal.pathPattern.length > pathArr.length)
                return false;
        } else {
            if (this.internal.pathPattern.length != pathArr.length)
                return false;
        }

        for (let i = 0; i < this.internal.pathPattern.length; ++i) {
            if (this.internal.pathPattern[i] == "*")
                continue;
            if (this.internal.pathPattern[i] != pathArr[i])
                return false;
        }

        return true;
    }

    suggestedPattern (url: string): URLP {
        let result;
        let U;
        U = new URL(url);
        result.allowLongerPaths = true;
        result.pathPattern = [];
        let domArr = U.hostname.split(".");
        if (domArr.length > 2) {
            result.domainPattern = domArr;
            result.domainPattern[0] = "*";
            result.allowSubdomains = false;
        } else {
            result.domainPattern = domArr;
            result.allowSubdomains = true;
        }

        return result;
    }
}

export class PasswordRecords {
    records: PMRecord[];

    constructor () {
        this.records = [];
    }
    toObject (): PMRecord[] {
        return this.records;
    }
    fromObject (obj: PMRecord[]): void {
        this.records = obj;
    }

    getRecords (url: string): PMRecord[] {
        return this.records.filter(r => new URLPattern(r.urlFilter).testURL(url));
    }
    setRecord (record: PMRecord): void {
        this.records.push(record);
    }

    toFile (): ArrayBuffer {
        let tmp = JSON.stringify(this.records);
        return StringEncoding.toUTF8(tmp);
    }

    fromFile (file: ArrayBuffer): void {
        let tmp = StringEncoding.fromUTF8(file);
        let obj = JSON.parse(tmp);
        if (!Array.isArray(obj))
            throw "Invalid Password Record File";
        this.fromObject(obj.filter(r => isPMRecord(r)));
    }
}