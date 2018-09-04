export class UserSettings {

}

interface URLP  {
    allowSubdomains: false;
    allowLongerPaths: false;
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
    identity: string;
    password: string;
    formData: object;
}

function isPMRecord (arg: any): arg is PMRecord {
    if (!("urlFilter" in arg) || !isURLP(arg.urlFilter))
        return false;
    if (!("appID" in arg) || typeof arg.appID != "string")
        return false;
    if (!("identity" in arg) || typeof arg.identity != "string")
        return false;
    if (!("password" in arg) || typeof arg.password != "string")
        return false;
    if (!("formData" in arg) || typeof arg.formData != "object")
        return false;
    return true;
}