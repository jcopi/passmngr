import * as Utils from "./build/utils.js"

async function DoPBKDF2 (string) {
	let password  = string;
	let passsalt  = Utils.Crypto.RandomBytes(256 / 8);
	let rawkey = await Utils.Crypto.StringToRawKey(password);
	let keypair = await Utils.Crypto.RawKeyToSymmetricKeyPair(rawkey, passsalt);
	keypair !== null && false;
}

async function init() {
	/*console.log("PERFORMANCE TESTING PBKDF2");
	let list = ["password", "abcdefghijklmnop", "12345678", "a bad password", "batterystaplehorsecorrect", "!@testT3sTtESt123"]
	let times = [];
	for (let i = 0; i < list.length; ++i) {
		let start = Date.now();
		let x = await DoPBKDF2(list[i]);
		times.push(Date.now() - start);
	}
	console.log(times.map((v,i) => {return [list[i].length, v]}));*/

	console.log("TESTING SYMMETRIC ENCRYPTION")

	let plaintext = "The quick brown fox jumped over the lazy dog.";
	console.log("Plaintext:", plaintext);
	let ptbuffer = Utils.StringEncoding.toUTF8(plaintext);

	let password  = "password";
	let passsalt  = Utils.Crypto.RandomBytes(256 / 8);
	let rawkey = await Utils.Crypto.StringToRawKey(password);
	let keypair = await Utils.Crypto.RawKeyToSymmetricKeyPair(rawkey, passsalt);
	let ctbuffer = await Utils.Crypto.SymmetricEncrypt(keypair, ptbuffer);
	console.log("Ciphertext: ", Utils.StringEncoding.fromUTF8(ctbuffer));

	let dptbuffer = await Utils.Crypto.SymmetricDecrypt(keypair, ctbuffer);
	console.log("Plaintext:", Utils.StringEncoding.fromUTF8(dptbuffer));

	console.log("TESTING ASYMMETRIC ENCRYPTION");
	let [apublic, aprivate] = await Utils.Crypto.GenerateAsymmetricKeyPair();
	let [bpublic, bprivate] = await Utils.Crypto.GenerateAsymmetricKeyPair();
	let asalt = Utils.Crypto.RandomBytes(Utils.Crypto.gcmNonceLength);
	let bsalt = Utils.Crypto.RandomBytes(Utils.Crypto.gcmNonceLength);

	let apacket = Utils.Channel.PackPublicKey(apublic, asalt);
	let bpacket = Utils.Channel.PackPublicKey(bpublic, bsalt);

	console.log("A's Public Key Packet: ", Utils.BufferEncoding.toBase64(apacket));
	console.log("B's Public Key Packet: ", Utils.BufferEncoding.toBase64(bpacket));

	// Process B's Public Key packet as A
	let [abpublic, absalt] = Utils.Channel.UnpackPublicKey(bpacket);
    let abothsalt = new Uint8Array(asalt.byteLength + absalt.byteLength);
    abothsalt.set(new Uint8Array(asalt)); abothsalt.set(new Uint8Array(absalt), asalt.byteLength);

    let awrappedKey = await Utils.Crypto.PublicBytesToRawKey(abpublic);
	let asharedBytes = await Utils.Crypto.DeriveSharedSecret(awrappedKey, aprivate);
	console.log("A's computed shared secret: ", Utils.BufferEncoding.toBase64(asharedBytes));
	let start = Date.now();
	let asymkeys = await Utils.Crypto.DerivedKeyToSymmetricKeyPair(asharedBytes, abothsalt.buffer, new ArrayBuffer(0));
	console.log("A's HKDF Took " + (Date.now() - start).toFixed(2) + "ms");
	// Process A's Public Key packet as B
	let [bapublic, basalt] = Utils.Channel.UnpackPublicKey(apacket);
    let bbothsalt = new Uint8Array(basalt.byteLength + bsalt.byteLength);
    bbothsalt.set(new Uint8Array(basalt)); bbothsalt.set(new Uint8Array(bsalt), basalt.byteLength);

    let bwrappedKey = await Utils.Crypto.PublicBytesToRawKey(bapublic);
	let bsharedBytes = await Utils.Crypto.DeriveSharedSecret(bwrappedKey, bprivate);
	console.log("B's computed shared secret: ", Utils.BufferEncoding.toBase64(bsharedBytes));
	start = Date.now();
	let bsymkeys = await Utils.Crypto.DerivedKeyToSymmetricKeyPair(bsharedBytes, bbothsalt.buffer, new ArrayBuffer(0));
	console.log("B's HKDF Took " + (Date.now() - start).toFixed(2) + "ms");

	// Encrypt plain text with a's sym key, decrypt with b's sym key
	let abplain = "Encrypt plain text with a's sym key, decrypt with b's sym key";
	console.log("Plaintext: ", abplain);
	let abctbuf = await Utils.Crypto.SymmetricEncrypt(asymkeys, Utils.StringEncoding.toUTF8(abplain));
	console.log("Ciphertext: ", Utils.StringEncoding.fromUTF8(abctbuf));
	let baplain = await Utils.Crypto.SymmetricDecrypt(bsymkeys, abctbuf);
	console.log("Plaintext: ", Utils.StringEncoding.fromUTF8(baplain));

	// Encrypt plain text with b's sym key, decrypt with a's sym key
	baplain = "Encrypt plain text with b's sym key, decrypt with a's sym key";
	console.log("Plaintext: ", baplain);
	let bactbuf = await Utils.Crypto.SymmetricEncrypt(bsymkeys, Utils.StringEncoding.toUTF8(baplain));
	console.log("Ciphertext: ", Utils.StringEncoding.fromUTF8(bactbuf));
	abplain = await Utils.Crypto.SymmetricDecrypt(asymkeys, bactbuf);
	console.log("Plaintext: ", Utils.StringEncoding.fromUTF8(abplain));
}


init();
window.utils = Utils;