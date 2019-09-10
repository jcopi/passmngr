import {passmngr} from "./build/utils.js"

async function symmetricTest (keys) {
	let toEncrypt = "The quick brown fox jumps over the lazy dog.";
	try {
		let encrypted = await passmngr.crypto.symmetric.encrypt(keys, utils.stringEncoding.toUTF8(toEncrypt));
		let decrypted = await passmngr.crypto.symmetric.decrypt(keys, encrypted);

		if (toEncrypt !== passmngr.stringEncoding.fromUTF8(decrypted)) {
			return false;
		}
	} catch (ex) {
		return false;
	}

	try {
		let encrypted = await passmngr.crypto.symmetric.encrypt(keys, passmngr.stringEncoding.toUTF8(toEncrypt));
		// Modifying the ciphertext should result in a null return on decryption
		let encArr = new Uint8Array(encrypted);
		encArr[Math.floor(Math.random() * encArr.length)] ^= 0b00010000;
		encrypted = encArr.buffer;
		
		let decrypted = await passmngr.crypto.symmetric.decrypt(keys, encrypted);

		if (decrypted !== null) {
			return false;
		}
	} catch (ex) {
		return false;
	}

	return true;
}

async function asymSigningTest (keys) {
	let toSign = passmngr.stringEncoding.toUTF8("The quick brown fox jumps over the lazy dog.");
	try {
		let signature = await passmngr.crypto.asymmetric.sign(keys, toSign);
		let verified = await passmngr.crypto.asymmetric.verify(keys, signature, toSign);

		if (!verified) {
			return false;
		}
	} catch (ex) {
		return false;
	}

	try {
		let signature = await passmngr.crypto.asymmetric.sign(keys, toSign);
		let modifiedToSign = passmngr.stringEncoding.toUTF8("The quick brovn fox jumps over the lazy dog.");
		let verified = await passmngr.crypto.asymmetric.verify(keys, signature, modifiedToSign);

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
		let aPublicMessage = await passmngr.crypto.asymmetric.keyExchangeMarshallPublic(aliceKeys);
		let bPublicMessage = await passmngr.crypto.asymmetric.keyExchangeMarshallPublic(bobKeys);
		let aKnowledgeOfB = await passmngr.crypto.asymmetric.keyExchangeUnmarshallPublic(aliceKeys.algo, bPublicMessage);
		let bKnowledgeOfA = await passmngr.crypto.asymmetric.keyExchangeUnmarshallPublic(bobKeys.algo, aPublicMessage);
		
		aComputedKeys = await passmngr.crypto.asymmetric.keyExchangeDeriveSymmetricKeys(passmngr.cryptoAlgos.AES_CBC_256_HMAC_SHA384, aliceKeys, aKnowledgeOfB);
		bComputedKeys = await passmngr.crypto.asymmetric.keyExchangeDeriveSymmetricKeys(passmngr.cryptoAlgos.AES_CBC_256_HMAC_SHA384, bobKeys, bKnowledgeOfA);
	} catch (ex) {
		return false;
	}

	try {
		let toEncrypt = "The quick brown fox jumped over the lazy dog.";
		let encrypted = await passmngr.crypto.symmetric.encrypt(aComputedKeys, utils.stringEncoding.toUTF8(toEncrypt));
		let decrypted = await passmngr.crypto.symmetric.decrypt(bComputedKeys, encrypted);

		if (toEncrypt !== passmngr.stringEncoding.fromUTF8(decrypted)) {
			return false;
		}
	} catch (ex) {
		return false;
	}

	return true;
}

function entropyEncodeTest (obj) {
	let plainString = JSON.stringify(obj);
	let plainBuffer = passmngr.stringEncoding.toUTF8(plainString);

	let encoded = passmngr.compress.entropyEncode(passmngr.compress.jsonEntropyModel, plainBuffer);
	let decoded = passmngr.compress.entropyDecode(passmngr.compress.jsonEntropyModel, encoded);

	let decodedString = passmngr.stringEncoding.fromUTF8(decoded);

	if (decodedString === plainString) {
		console.log("ENTROPY ENCODING: PASS");
		console.log("                : " + (100 - (100 * (encoded.byteLength / plainBuffer.byteLength))).toFixed(2) + "%");
	}
}

async function test () {
	console.log("Testing crypto library");
	for (let k in passmngr.cryptoAlgos) {
		let a = passmngr.cryptoAlgos[k];
		if (!Number.isInteger(a)) continue;

		if (a & passmngr.calg.AES) {
			// Perform symmetric encryption tests
			// Keygen from password -> encrypt -> decrypt -> encrypt & modify -> fail to decrypt
			// Keygen from random   -> encrypt -> decrypt -> encrypt & modify -> fail to decrypt
			let salt = passmngr.crypto.primitives.randomBits(passmngr.crypto.keygen.defaultAESLength);
			let keys = await passmngr.crypto.keygen.fromPassword("ThisIsAPasswordToTest", a, salt, 10);
			let fromPwd = await symmetricTest(keys);

			keys = await passmngr.crypto.keygen.fromRandom(a);
			let fromRnd = await symmetricTest(keys);

			console.log(passmngr.cryptoAlgos[a] + " [Password] : " + (fromPwd ? "PASS" : "FAIL"));
			console.log("                       " + " [Random]   : " + (fromRnd ? "PASS" : "FAIL"));
		} else if (a & passmngr.calg.ECDSA) {
			// Perform asymmetric signing tests
			// Keygen from random -> sign -> verify -> sign & modify -> fail to verify
			let keys = await passmngr.crypto.keygen.fromRandom(a);

			let fromRnd = await asymSigningTest(keys);

			console.log(passmngr.cryptoAlgos[a] + " [Random] : " + (fromRnd ? "PASS" : "FAIL"));
		} else if (a & passmngr.calg.ECDHE) {
			// Perform key exchange tests
			// Keygen from random -> verify 2 local entites derive the same symmetric keys
			let aliceKeys = await passmngr.crypto.keygen.fromRandom(a);
			let bobKeys = await passmngr.crypto.keygen.fromRandom(a);

			let fromRnd = await asymKeyExchangeTest(aliceKeys, bobKeys);

			console.log(passmngr.cryptoAlgos[a] + " [Random] : " + (fromRnd ? "PASS" : "FAIL"));
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
test();
window.utils = passmngr;