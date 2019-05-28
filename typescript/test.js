import * as Utils from "./build/utils.js"

async function init() {
	let plaintext = "The quick brown fox jumped over the lazy dog.";
	console.log("Plaintext:", plaintext);
	let ptbuffer = Utils.StringEncoding.toUTF8(plaintext);
	console.log("Plaintext buffer:", Utils.BufferEncoding.toBase64(ptbuffer));

	let password  = "password";
	let passsalt  = Utils.Crypto.RandomBytes(256 / 8);

	let rawkey = await Utils.Crypto.StringToRawKey(password);
	password = null;
	let keypair = await Utils.Crypto.RawKeyToSymmetricKeyPair(rawkey, passsalt);
	passsalt = null;
	
	let ctbuffer = await Utils.Crypto.SymmetricEncrypt(keypair, ptbuffer);
	console.log("Ciphertext buffer: ", Utils.BufferEncoding.toBase64(ctbuffer));

	let dptbuffer = await Utils.Crypto.SymmetricDecrypt(keypair, ctbuffer);
	console.log("Decrypted Buffer:", Utils.BufferEncoding.toBase64(dptbuffer));

	console.log("Decrypted text:", Utils.StringEncoding.fromUTF8(dptbuffer));
}


init();

window.utils = Utils;