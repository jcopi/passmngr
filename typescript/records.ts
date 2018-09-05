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

interface password_t {
    domain: string;
    appID: string;
    tag: string;
    identity: string;
    password: string;
    formData: object;
}

class Passwords {
    internal: password_t[];

    hasEntriesByURL(url: string): boolean {
        let urlO = new URL(url);
        let domain = urlO.hostname.split(".").slice(-2).join(".");

        let result = false;
        for (let i = 0; !result && i < this.internal.length; ++i) 
            result = (domain == this.internal[i].domain);
        return result;
    }
    getEntriesByURL(url: string): password_t[] {
        let urlO = new URL(url);
        let domain = urlO.hostname.split(".").slice(-2).join(".");

        return this.internal.filter(r => (r.domain == domain));
    }
    editEntriesByURL(fn: (p: password_t) => password_t, url: string): void {
        let urlO = new URL(url);
        let domain = urlO.hostname.split(".").slice(-2).join(".");

        this.internal.forEach((r, i, a) => {
            if (r.domain == domain)
                a[i] = fn(r);
        });
    }
    removeAllEntriesOfURL(url: string): void {
        let urlO = new URL(url);
        let domain = urlO.hostname.split(".").slice(-2).join(".");

        this.internal = this.internal.filter(r => (r.domain != domain));
    }

    filterEntriesOfURL(fn: (p: password_t) => boolean, url: string): void {
        let urlO = new URL(url);
        let domain = urlO.hostname.split(".").slice(-2).join(".");

        this.internal = this.internal.filter(r => (r.domain != domain || fn(r)));
    }

    hasEntriesByAppID(id: string): boolean {
        let result = false;
        for (let i = 0; !result && i < this.internal.length; ++i) 
            result = (id == this.internal[i].appID);
        return result;
    }
    getEntriesByAppID(id: string): password_t[] {
        return this.internal.filter(r => (r.appID == id));
    }
    editEntriesByAppID(fn: (p: password_t) => password_t, id: string): void {
        this.internal.forEach((r, i, a) => {
            if (r.appID == id)
                a[i] = fn(r);
        });
    }
    removeAllEntriesOfAppID(id: string): void {
        this.internal = this.internal.filter(r => (r.appID != id));
    }
    filterEntriesOfAppID(fn: (p: password_t) => boolean, id: string): void {
        this.internal = this.internal.filter(r => (r.appID != id || fn(r)));
    }

    newEntry(value: password_t): void {}

    static recordFromObject (o: object): password_t {
        let result = {"domain":"", "appID":"", "tag":"default", "identity":"", "password":"", "formData":{}};

        for (let key in result) {
            if (key in o && typeof o[key] == typeof result[key])
                result[key] = o[key];
        }

        return result;
    }
}
