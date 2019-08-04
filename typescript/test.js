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
	console.log("TESTING BER DECODE");
	let pem = "-----BEGIN CERTIFICATE-----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-----END CERTIFICATE-----";
	pem = pem.replace(/\-+(BEGIN|END)\s*CERTIFICATE\-+/g, "");
	let pemArr = utils.bufferEncoding.fromBase64(pem);
	let pemObj = utils.berEncoding.fromBer(pemArr)
	console.log(pemArr, pemObj);

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

	console.log("TESTING ASYMMETRIC ECDH KEY EXCHANGE");

	console.log("Generating ECC keys.");
	let aliceKeys = await utils.crypto.asymmetric.ecdh.generateKeyPair();
	let bobKeys = await utils.crypto.asymmetric.ecdh.generateKeyPair();

	console.log("Exchanging public keys and salts.");
	let alicePublicMessage = await utils.crypto.asymmetric.ecdh.marshallPublicKey(aliceKeys, new ArrayBuffer(0));
	let bobPublicMessage = await utils.crypto.asymmetric.ecdh.marshallPublicKey(bobKeys, new ArrayBuffer(0));
	let [aliceKnowledgeOfBob, ax] = await utils.crypto.asymmetric.ecdh.unmarshallPublicKey(bobPublicMessage);
	let [bobKnowledgeOfAlice, bx] = await utils.crypto.asymmetric.ecdh.unmarshallPublicKey(alicePublicMessage);

	console.log("Computing shared symmetric keys.");
	let aliceComputedKeys = await utils.crypto.asymmetric.ecdh.deriveSharedSymmetricKeys(aliceKeys, aliceKnowledgeOfBob, new ArrayBuffer(0), false);
	let bobComputedKeys = await utils.crypto.asymmetric.ecdh.deriveSharedSymmetricKeys(bobKeys, bobKnowledgeOfAlice, new ArrayBuffer(0), true);

	let aliceSecretMessage = "abcdefghijklmnopqrstuvwxyz0123456789";
	let bobSecretMessage = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	console.log("Alice encrypting message: " + aliceSecretMessage);
	let aliceEncryptedMessage = await utils.crypto.symmetric.encrypt(aliceComputedKeys, utils.stringEncoding.toUTF8(aliceSecretMessage));
	let bobDecryptedMessage = await utils.crypto.symmetric.decrypt(bobComputedKeys, aliceEncryptedMessage);
	console.log("Bob decrypted as: " + utils.stringEncoding.fromUTF8(bobDecryptedMessage));

	console.log("Bob encrypting message: " + bobSecretMessage);
	let bobEncryptedMessage = await utils.crypto.symmetric.encrypt(bobComputedKeys, utils.stringEncoding.toUTF8(bobSecretMessage));
	let aliceDecryptedMessage = await utils.crypto.symmetric.decrypt(aliceComputedKeys, bobEncryptedMessage);
	console.log("Alice decrypted as: " + utils.stringEncoding.fromUTF8(aliceDecryptedMessage));
}

async function symmetricTest (keys) {
	let toEncrypt = "The quick brown fox jumps over the lazy dog.";
	try {
		let encrypted = await utils.passmngr.crypto.symmetric.encrypt(keys, utils.stringEncoding.toUTF8(toEncrypt));
		let decrypted = await utils.passmngr.crypto.symmetric.decrypt(keys, encrypted);

		if (toEncrypt !== utils.stringEncoding.fromUTF8(decrypted)) {
			return false;
		}
	} catch (ex) {
		return false;
	}

	try {
		let encrypted = await utils.passmngr.crypto.symmetric.encrypt(keys, utils.stringEncoding.toUTF8(toEncrypt));
		// Modifying the ciphertext should result in a null return on decryption
		let encArr = new Uint8Array(encrypted);
		encArr[Math.floor(Math.random() * encArr.length)] ^= 0b00010000;
		encrypted = encArr.buffer;
		
		let decrypted = await utils.passmngr.crypto.symmetric.decrypt(keys, encrypted);

		if (decrypted !== null) {
			return false;
		}
	} catch (ex) {
		return false;
	}

	return true;
}

async function asymSigningTest (keys) {
	let toSign = utils.stringEncoding.toUTF8("The quick brown fox jumps over the lazy dog.");
	try {
		let signature = await utils.passmngr.crypto.asymmetric.sign(keys, toSign);
		let verified = await utils.passmngr.crypto.asymmetric.verify(keys, signature, toSign);

		if (!verified) {
			return false;
		}
	} catch (ex) {
		return false;
	}

	try {
		let signature = await utils.passmngr.crypto.asymmetric.sign(keys, toSign);
		let modifiedToSign = utils.stringEncoding.toUTF8("The quick brovn fox jumps over the lazy dog.");
		let verified = await utils.passmngr.crypto.asymmetric.verify(keys, signature, modifiedToSign);

		if (verified) {
			return false;
		}
	} catch (ex) {
		return false;
	}

	return true;
}

async function asymKeyExchangeTest (aliceKeys, bobKeys) {
	
	let aComputedKeys, bComputedKeys;
	try {
		let aPublicMessage = await utils.passmngr.crypto.asymmetric.keyExchangeMarshallPublic(aliceKeys);
		let bPublicMessage = await utils.passmngr.crypto.asymmetric.keyExchangeMarshallPublic(bobKeys);
		let aKnowledgeOfB = await utils.passmngr.crypto.asymmetric.keyExchangeUnmarshallPublic(aliceKeys.algo, bPublicMessage);
		let bKnowledgeOfA = await utils.passmngr.crypto.asymmetric.keyExchangeUnmarshallPublic(bobKeys.algo, aPublicMessage);
		
		aComputedKeys = await utils.passmngr.crypto.asymmetric.keyExchangeDeriveSymmetricKeys(utils.passmngr.cryptoAlgos.AES_CBC_256_HMAC_SHA384, aliceKeys, aKnowledgeOfB);
		bComputedKeys = await utils.passmngr.crypto.asymmetric.keyExchangeDeriveSymmetricKeys(utils.passmngr.cryptoAlgos.AES_CBC_256_HMAC_SHA384, bobKeys, bKnowledgeOfA);
	} catch (ex) {
		return false;
	}

	try {
		let toEncrypt = "The quick brown fox jumped over the lazy dog.";
		let encrypted = await utils.passmngr.crypto.symmetric.encrypt(aComputedKeys, utils.stringEncoding.toUTF8(toEncrypt));
		let decrypted = await utils.passmngr.crypto.symmetric.decrypt(bComputedKeys, encrypted);

		if (toEncrypt !== utils.stringEncoding.fromUTF8(decrypted)) {
			return false;
		}
	} catch (ex) {
		return false;
	}

	return true;
}

async function test () {
	console.log("Testing crypto library");
	for (let k in utils.passmngr.cryptoAlgos) {
		let a = utils.passmngr.cryptoAlgos[k];
		if (!Number.isInteger(a)) continue;

		if (a & utils.passmngr.calg.AES) {
			// Perform symmetric encryption tests
			// Keygen from password -> encrypt -> decrypt -> encrypt & modify -> fail to decrypt
			// Keygen from random   -> encrypt -> decrypt -> encrypt & modify -> fail to decrypt
			let salt = utils.passmngr.crypto.primitives.randomBits(utils.passmngr.crypto.keygen.defaultAESLength);
			let keys = await utils.passmngr.crypto.keygen.fromPassword("ThisIsAPasswordToTest", a, salt, 10);
			let fromPwd = await symmetricTest(keys);

			keys = await utils.passmngr.crypto.keygen.fromRandom(a);
			let fromRnd = await symmetricTest(keys);

			console.log(utils.passmngr.cryptoAlgos[a] + " [Password] : " + (fromPwd ? "PASS" : "FAIL"));
			console.log("                       " + " [Random]   : " + (fromRnd ? "PASS" : "FAIL"));
		} else if (a & utils.passmngr.calg.ECDSA) {
			// Perform asymmetric signing tests
			// Keygen from random -> sign -> verify -> sign & modify -> fail to verify
			let keys = await utils.passmngr.crypto.keygen.fromRandom(a);

			let fromRnd = await asymSigningTest(keys);

			console.log(utils.passmngr.cryptoAlgos[a] + " [Random] : " + (fromRnd ? "PASS" : "FAIL"));
		} else if (a & utils.passmngr.calg.ECDHE) {
			// Perform key exchange tests
			// Keygen from random -> verify 2 local entites derive the same symmetric keys
			let aliceKeys = await utils.passmngr.crypto.keygen.fromRandom(a);
			let bobKeys = await utils.passmngr.crypto.keygen.fromRandom(a);

			let fromRnd = await asymKeyExchangeTest(aliceKeys, bobKeys);

			console.log(utils.passmngr.cryptoAlgos[a] + " [Random] : " + (fromRnd ? "PASS" : "FAIL"));
		}
	}
}

// init();
test();
window.utils = utils;