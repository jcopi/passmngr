interface bi_core {
    buffer: Uint32Array;
    length: number;
    negative: boolean;
}

const BI_SHIFT: number = 24;
const BI_MASK: number  = 0xFFFFFF;
const BI_BASE: number  = BI_MASK + 1;

function _abs_comp (a: bi_core, b:bi_core): number {
    if (a.length > b.length) return 1;
    if (b.length > a.length) return -1;
    let i = a.length;
    while (i-- && a[i] == b[i]);
    if (i < 0) return 0;
    if (a[i] > b[i]) return 1;
    return -1;
}

function _add_pp (a: bi_core, b:bi_core): bi_core {
    if (a.length < b.length)
        return _add_pp(b,a);
    let capacity = Math.max(a.length + 1, a.buffer.length);
    let result = {
        "buffer": new Uint32Array(capacity),
        "length":0,
        "negative":false
    };

    result.buffer.set(a.buffer, 0);

    let carry = 0;
    let op    = 0;
    let i     = 0;
    while (i < b.length) {
        op = a.buffer[i] + b.buffer[i] + carry;
        result.buffer[i] = op & BI_MASK;
        carry = op >> BI_SHIFT;
        ++i;
    }

    while (carry > 0 && i < a.length) {
        op = a.buffer[i] + carry;
        result.buffer[i] = op & BI_MASK;
        carry = op >> BI_SHIFT;
        ++i;
    }

    if (carry > 0) {
        result.buffer[i] = carry;
        ++i;
    }

    result.length = Math.max(a.length, i);

    return result;
}

function _subt_pp (a: bi_core, b: bi_core): bi_core  {
    // The assumption of a > b is done in the calling function
    let capacity = Math.max(a.length, a.buffer.length);
    let result = {
        "buffer": new Uint32Array(capacity),
        "length":0,
        "negative":false
    };

    result.buffer.set(a.buffer, 0);

    let borrow = 0;
    let op     = 0;
    let i      = 0;
    while (i < b.length) {
        op = a.buffer[i] - borrow - b.buffer[i];
        if (op < 0) {
            op += BI_BASE;
            borrow = 1;
        } else {
            borrow = 0;
        }
        result.buffer[i] = op;
        ++i;
    }

    while (borrow > 0 && i < a.length) {
        op = a.buffer[i] - borrow;
        if (op < 0) {
            op += BI_BASE;
            borrow = 1;
        } else {
            borrow = 0;
        }
        result.buffer[i] = op;
        ++i;
    }

    for (i = a.length; i--;) {
        if (result.buffer[i] != 0)
            break;
    }
    result.length = i;

    return result;
}

function bi_add (a: bi_core, b: bi_core): bi_core {
    let result: bi_core;
    if (a.negative && b.negative) {
        result = _add_pp(a,b);
        result.negative = true;
    } else if (a.negative) {
        result = bi_subtract(b,a);
    } else if (b.negative) {
        result = bi_subtract(a,b);
    } else {
        result = _add_pp(a,b);
    }

    return result;
}

function bi_subtract (a: bi_core, b: bi_core): bi_core {
    let result: bi_core;
    let cmp = _abs_comp(a,b);
    if (a.negative && b.negative) { // b - a
        if (cmp >= 0) {
            result = _subt_pp(a,b);
            result.negative = true;
        } else {
            result = _subt_pp(b,a);
        }
    } else if (a.negative) {
        result = _add_pp(a,b);
        result.negative = true;
    } else if (b.negative) {
        result = _add_pp(a,b);
    } else {
        if (cmp >= 0) {
            result = _subt_pp(a,b);
        } else {
            result = _subt_pp(b,a);
            result.negative = true;
        }
    }

    return result;
}

