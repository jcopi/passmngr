import * as utils from "./build/utils.js"

async function DoPBKDF2 (string) {
	let password = string;
	let aessalt  = utils.crypto.randomBytes(256 / 8);
	let hmacsalt = utils.crypto.randomBytes(256 / 8);
	let keypair = await utils.crypto.symmetric.deriveKeysFromPassword(password, aessalt, hmacsalt);
	keypair !== null && false;
	return true;
}

async function init() {
	/*console.log("PERFORMANCE TESTING PBKDF2");
	let list = ["password", "abcdefghijklmnop", "12345678", "a bad password", "batterystaplehorsecorrect", "!@testT3sTtESt123"]
	let times = [];
	for (let i = 0; i < list.length; ++i) {
		let start = Date.now();
		//let x = await DoPBKDF2(list[i]);
		//if (!x) break;
		times.push(Date.now() - start);
	}
	console.log(times.map((v,i) => {return [list[i].length, v]}));*/

	console.log("TESTING SYMMETRIC ENCRYPTION")

	let plaintext = "The quick brown fox jumped over the lazy dog.";
	console.log("Plaintext:", plaintext);
	let ptbuffer = utils.stringEncoding.toUTF8(plaintext);

	let password = "password";
	let aessalt  = utils.crypto.randomBytes(256 / 8);
	let hmacsalt = utils.crypto.randomBytes(256 / 8);
	let keypair  = await utils.crypto.symmetric.deriveKeysFromPassword(password, aessalt, hmacsalt);
	let ctbuffer = await utils.crypto.symmetric.encrypt(keypair, ptbuffer);
	console.log("Ciphertext: ", utils.stringEncoding.fromUTF8(ctbuffer));

	let dptbuffer = await utils.crypto.symmetric.decrypt(keypair, ctbuffer);
	console.log("Plaintext:", utils.stringEncoding.fromUTF8(dptbuffer));

	console.log("TESTING ASYMMETRIC ENCRYPTION");

	console.log("Generating ECC keys.");
	let aliceKeys = await utils.crypto.asymmetric.generateKeyPair();
	let bobKeys = await utils.crypto.asymmetric.generateKeyPair();

	console.log("Exchanging public keys and salts.");
	let alicePublicMessage = await utils.crypto.asymmetric.marshallPublicKey(aliceKeys, new ArrayBuffer(0));
	let bobPublicMessage = await utils.crypto.asymmetric.marshallPublicKey(bobKeys, new ArrayBuffer(0));
	let [aliceKnowledgeOfBob, ax] = await utils.crypto.asymmetric.unmarshallPublicKey(bobPublicMessage);
	let [bobKnowledgeOfAlice, bx] = await utils.crypto.asymmetric.unmarshallPublicKey(alicePublicMessage);

	console.log("Computing shared symmetric keys.");
	let aliceComputedKeys = await utils.crypto.asymmetric.deriveSharedSymmetricKeys(aliceKeys, aliceKnowledgeOfBob, new ArrayBuffer(0), false);
	let bobComputedKeys = await utils.crypto.asymmetric.deriveSharedSymmetricKeys(bobKeys, bobKnowledgeOfAlice, new ArrayBuffer(0), true);

	let aliceSecretMessage = "abcdefghijklmnopqrstuvwxyz0123456789";
	let bobSecretMessage = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	console.log("Alice encrypting message: " + aliceSecretMessage);
	console.log(aliceComputedKeys);
	let aliceEncryptedMessage = await utils.crypto.symmetric.encrypt(aliceComputedKeys, utils.stringEncoding.toUTF8(aliceSecretMessage));
	let bobDecryptedMessage = await utils.crypto.symmetric.decrypt(bobComputedKeys, aliceEncryptedMessage);
	console.log("Bob decrypted as: " + utils.stringEncoding.fromUTF8(bobDecryptedMessage));

	console.log("Bob encrypting message: " + bobSecretMessage);
	let bobEncryptedMessage = await utils.crypto.symmetric.encrypt(bobComputedKeys, utils.stringEncoding.toUTF8(bobSecretMessage));
	let aliceDecryptedMessage = await utils.crypto.symmetric.decrypt(aliceComputedKeys, bobEncryptedMessage);
	console.log("Alice decrypted as: " + utils.stringEncoding.fromUTF8(aliceDecryptedMessage));
}

init();
window.utils = utils;