export interface password_t {
    domain: string;
    appID: string;
    tag: string;
    identity: string;
    password: string;
    formData: object;
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
            if (!Passwords.domainMatch(r.domain, url))
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

    }
    deleteAllAppID (id: string): void {

    }
    filterAppID (id: string, fn: (p: password_t) => boolean): void {

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