import * as utils from "./build/utils.js"

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

function entropyEncodeTest (obj) {
	let plainString = JSON.stringify(obj);
	let plainBuffer = utils.stringEncoding.toUTF8(plainString);

	let encoded = utils.compress.entropyEncode(utils.compress.jsonEntropyModel, plainBuffer);
	let decoded = utils.compress.entropyDecode(utils.compress.jsonEntropyModel, encoded);

	let decodedString = utils.stringEncoding.fromUTF8(decoded);

	if (decodedString === plainString) {
		console.log("ENTROPY ENCODING: PASS");
		console.log("                : " + (100 - (100 * (encoded.byteLength / plainBuffer.byteLength))).toFixed(2) + "%");
	}
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


let x = {
    "compilerOptions":{
        "target"     : "es6",
        "lib"        : ["dom", "es6"],
        "declaration": true,
        "outDir"     : "./build"
    },
    "files": [
        "./utils.ts"
    ]
};
let y = {
	"options": {"failByDrop": false},
	"outdir": "./reports/clients",
	"servers": [
		 {"agent": "ReadAllWriteMessage", "url": "ws://localhost:9000/m", "options": {"version": 18}},
		 {"agent": "ReadAllWritePreparedMessage", "url": "ws://localhost:9000/p", "options": {"version": 18}},
		 {"agent": "ReadAllWrite", "url": "ws://localhost:9000/r", "options": {"version": 18}},
		 {"agent": "CopyFull", "url": "ws://localhost:9000/f", "options": {"version": 18}},
		 {"agent": "CopyWriterOnly", "url": "ws://localhost:9000/c", "options": {"version": 18}}
	 ],
	"cases": ["*"],
	"exclude-cases": [],
	"exclude-agent-cases": {}
 };
entropyEncodeTest(x);
entropyEncodeTest(y);

// init();
//test();
window.utils = utils;