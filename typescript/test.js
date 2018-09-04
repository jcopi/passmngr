import * as PassManager from "./build/core.js"
import { PasswordRecords } from "./records.js";

let pmCrypto = new PassManager.Crypto();
async function init () {
	let message = "This is my message. It's not very long";
	console.log("Original Message", message);
	let messageBuffer = PassManager.StringEncoding.toUTF8(message);
	console.log("Original message Buffer", PassManager.Base64.serialize(messageBuffer));
	await pmCrypto.initKeys("password");
	let encrypted = await pmCrypto.encrypt(messageBuffer);
	console.log("Encrypted Buffer", PassManager.Base64.serialize(encrypted));
	let signed = await pmCrypto.sign(encrypted);
	console.log("Signed Buffer", PassManager.Base64.serialize(signed));
	let vres = await pmCrypto.verify(signed);
	if (vres[0] == false) {
		console.log("Signature not verified");
		return;
	}
	console.log("Signature VERIFIED encrypted buffer", PassManager.Base64.serialize(vres[1]));
	let decrypted = await pmCrypto.decrypt(vres[1]);
	console.log("Decrypted Buffer", PassManager.Base64.serialize(decrypted));
	let Td = new TextDecoder();
	console.log("Original Message:", Td.decode(decrypted));
}

let PR = new PasswordRecords();


init();