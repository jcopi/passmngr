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
	let ex1 = new Uint8Array([0x04, 0x05, 0x12, 0x34, 0x56, 0x78, 0x90]);
	console.log(ex1, utils.berEncoding.fromBer(ex1.buffer));
	let ex2 = new Uint8Array([0xdf, 0x82, 0x02, 0x05, 0x12, 0x34, 0x56, 0x78, 0x90]);
	console.log(ex2, utils.berEncoding.fromBer(ex2.buffer));
	let pem = "-----BEGIN CERTIFICATE-----\
	MIIEuTCCA6GgAwIBAgIQQBrEZCGzEyEDDrvkEhrFHTANBgkqhkiG9w0BAQsFADCB\
	vTELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL\
	ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwOCBWZXJp\
	U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MTgwNgYDVQQDEy9W\
	ZXJpU2lnbiBVbml2ZXJzYWwgUm9vdCBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eTAe\
	Fw0wODA0MDIwMDAwMDBaFw0zNzEyMDEyMzU5NTlaMIG9MQswCQYDVQQGEwJVUzEX\
	MBUGA1UEChMOVmVyaVNpZ24sIEluYy4xHzAdBgNVBAsTFlZlcmlTaWduIFRydXN0\
	IE5ldHdvcmsxOjA4BgNVBAsTMShjKSAyMDA4IFZlcmlTaWduLCBJbmMuIC0gRm9y\
	IGF1dGhvcml6ZWQgdXNlIG9ubHkxODA2BgNVBAMTL1ZlcmlTaWduIFVuaXZlcnNh\
	bCBSb290IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MIIBIjANBgkqhkiG9w0BAQEF\
	AAOCAQ8AMIIBCgKCAQEAx2E3XrEBNNti1xWb/1hajCMj1mCOkdeQmIN65lgZOIzF\
	9uVkhbSicfvtvbnazU0AtMgtc6XHaXGVHzk8skQHnOgO+k1KxCHfKWGPMiJhgsWH\
	H26MfF8WIFFE0XBPV+rjHOPMee5Y2A7Cs0WTwCznmhcrewA3ekEzeOEz4vMQGn+H\
	LL729fdC4uW/h2KJXwBL38Xd5HVEMkE6HnFuacsLdUYI0crSK5XQz/u5QGtkjFdN\
	/BMReYTtXlT2NJ8IAfMQJQYXStrxHXpma5hgZqTZ79IugvHw7wnqRMkVauIDbjPT\
	rJ9VAMf2CGqUuV/c4DPxhGD5WycRtPwW8rtWaoAljQIDAQABo4GyMIGvMA8GA1Ud\
	EwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgEGMG0GCCsGAQUFBwEMBGEwX6FdoFsw\
	WTBXMFUWCWltYWdlL2dpZjAhMB8wBwYFKw4DAhoEFI/l0xqGrI2Oa8PPgGrUSBgs\
	exkuMCUWI2h0dHA6Ly9sb2dvLnZlcmlzaWduLmNvbS92c2xvZ28uZ2lmMB0GA1Ud\
	DgQWBBS2d/ppSEefUxLVwuoHMnYH0ZcHGTANBgkqhkiG9w0BAQsFAAOCAQEASvj4\
	sAPmLGd75JR3Y8xuTPl9Dg3cyLk1uXBPY/ok+myDjEedO2Pzmvl2MpWRsXe8rJq+\
	seQxIcaBlVZaDrHC1LGmWazxY8u4TB1ZkErvkBYoH1quEPuBUDgMbMzxPcP1Y+Oz\
	4yHJJDnp/RVmRvQbEdBNc6N9Rvk97ahfYtTxP/jgdFcrGJ2BtMQo2pSXpXDrrB2+\
	BxHw1dvd5Yzw1TKwg+ZX4o+/vqGqvz0dtdQ46tewXDpPaj+PwGZsY6rp2aQW9IHR\
	lRQOfc2VNNnSj3BzgXucfr2YYdhFh5iQxeuGMMY1v/D/w1WIg0vvBZIGcfK4mJO3\
	7M2CYfE45k+XmCpajQ==\
	-----END CERTIFICATE-----".replace(/[\n\r\s]+/g, "");
	pem = pem.replace(/\-+(BEGIN|END)\s*CERTIFICATE\-+/g, "");
	let pemArr = utils.bufferEncoding.fromBase64(pem);
	console.log(pemArr, utils.berEncoding.fromBer(pemArr));

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

init();
window.utils = utils;