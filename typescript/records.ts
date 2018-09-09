import { StringEncoding } from "./core";

export interface password_t {
    domain: string;
    appID: string;
    tag: string;
    identity: string;
    password: string;
    formData: object;
}
function isPassword (arg: any): arg is password_t {
    if (!("domain" in arg ) || typeof arg.domain != "string")
        return false;
    if (!("appID" in arg ) || typeof arg.appID != "string")
        return false;
    if (!("tag" in arg ) || typeof arg.tag != "string")
        return false;
    if (!("identity" in arg ) || typeof arg.identity != "string")
        return false;
    if (!("password" in arg ) || typeof arg.password != "string")
        return false;
    if (!("formData" in arg ) || typeof arg.formData != "object")
        return false;
    return true;
}

export class Passwords {
    internal: password_t[];

    hasURL (url: string): boolean {
        for (let i = 0; i < this.internal.length; i++) {
            if (Passwords.domainMatch(this.internal[i].domain, url))
                return true;
        }
        return false;
    }
    getURL (url: string): password_t[] {
        return this.internal.filter(r => Passwords.domainMatch(r.domain, url));
    }
    deleteAllURL (url: string): void {
        this.internal = this.internal.filter(r => !Passwords.domainMatch(r.domain, url));
    }
    filterURL (url: string, fn: (p: password_t) => boolean): void {
        this.internal = this.internal.filter(r => {
            if (Passwords.domainMatch(r.domain, url))
                return fn(r);
            return true;
        })
    }

    hasAppID (id: string): boolean {
        for (let i = 0; i < this.internal.length; i++) {
            if (this.internal[i].appID == id)
                return true;
        }
        return false;
    }
    getAppID (id: string): password_t[] {
        return this.internal.filter(r => r.appID == id);
    }
    deleteAllAppID (id: string): void {
        this.internal = this.internal.filter(r => r.appID != id);
    }
    filterAppID (id: string, fn: (p: password_t) => boolean): void {
        this.internal = this.internal.filter(r => {
            if (r.appID == id)
                return fn(r);
            return true;
        })
    }

    toFile (): ArrayBuffer {
        let str = JSON.stringify(this.internal);
        return StringEncoding.toUTF8(str);
    }
    fromFile (buffer: ArrayBuffer): void {
        let str = StringEncoding.fromUTF8(buffer);
        let obj = JSON.parse(str);
        if (!Array.isArray(obj))
            throw "";
        this.internal = [];
        obj.forEach(v => {
            if (isPassword(v))
                this.internal.push(v);
        });
    }

    static domainMatch (pattern: string, url: string): boolean {
        let urlo = new URL(url);
        let domain = urlo.hostname.split(".");
        let patts = pattern.split(".");
        if (patts.length == 0)
            return false;
        if (domain.length < patts.length)
            return false;
        
        let lengthConstrained = true;
        while (patts.length) {
            let p = patts.pop();
            let d = domain.pop();
            if (p.length > 1 && p[0] == "*") {
                lengthConstrained = false;
                p = p.substr(1);
            } else {
                lengthConstrained = true;
            }
            if (p == "*")
                continue;
            if (p != d)
                return false;
        }

        if (lengthConstrained && patts.length != domain.length)
            return false;
        return true;
    }
}