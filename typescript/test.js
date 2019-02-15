import * as Core    from "./build/core.js"
import * as Records from "./build/records.js"

let pmCrypto = new Core.Crypto();
async function init () {
    let start = performance.now();
	let message = "This is my message. It's not very long";
    console.log("Original Message: ", message);
    
	let messageBuffer = Core.StringEncoding.toUTF8(message);
    await pmCrypto.initKeys("password");
    let encrypted = await pmCrypto.encrypt(messageBuffer);
    
	console.log("Encrypted Buffer: ", Core.BufferEncoding.toBase64(encrypted));
	let signed = await pmCrypto.sign(encrypted);
	console.log("Signed Buffer:    ", Core.BufferEncoding.toBase64(signed));
	let vres = await pmCrypto.verify(signed);
	if (vres[0] == false) {
		console.log("Signature not verified");
		return;
	}
	let decrypted = await pmCrypto.decrypt(vres[1]);
	console.log("Decrypted Buffer", Core.BufferEncoding.toBase64(decrypted));
    console.log("Original Message:", Core.StringEncoding.fromUTF8(decrypted));
    console.log(performance.now() - start, "ms");

    console.log("PBKDF2 Rounds: ", Core.Crypto.STANDARD_KDF_ITER_COUNT);
}

init();