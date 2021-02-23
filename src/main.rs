use std::env;

use std::error::Error;

use std::io;
use std::io::prelude::*;
use std::io::File;
use std::io::SeekFrom;

use sodiumoxide;
use sodiumoxide::crypto;
use sodiumoxide::utils;
use sodiumoxide::randombytes;

const PLAINTEXT_BUFFER_SIZE: usize = 1024;
const CIPHERTEXT_BUFFER_SIZE: usize = PLAINTEXT_BUFFER_SIZE + crypto::secretstream::xchacha20poly1305::ABYTES;
const SALT_SIZE: usize = crypto::pwhash::argon2id13::SALTBYTES;
const KEY_SIZE: usize = crypto::secretstream::xchacha20poly1305::KEYBYTES

fn main() {
    println!("Hello, world!");
}

enum KVState {
    Locked,
    Unlocked
}

struct EncryptedKV {
    file_name: String,
    file: File,
    user_salt: [u8; SALT_SIZE]
    user_key: [u8; KEY_SIZE],
    main_key: [u8; KEY_SIZE],
    pt_buffer: [u8; PLAINTEXT_BUFFER_SIZE],
    pt_overflow: [u8; PLAINTEXT_BUFFER_SIZE],
    ct_buffer: [u8; CIPHERTEXT_BUFFER_SIZE],
    state: KVState
}

impl EncryptedKV {
    fn new(file_name: String) -> Result<EncryptedKV, Error> {
        let ekv = EncryptedKV {
            file_name: file_name,
            file: None,
            user_salt: [0; SALT_SIZE],
            user_key: [0; KEY_SIZE],
            main_key: [0; KEY_SIZE],
            pt_buffer: [0; PLAINTEXT_BUFFER_SIZE],
            pt_overflow: [0; PLAINTEXT_BUFFER_SIZE],
            ct_buffer: [0; CIPHERTEXT_BUFFER_SIZE],
            state: KVState::Locked
        }

        utils::mlock(&mut ekv.user_key).unwrap();
        utils::mlock(&mut ekv.main_key).unwrap();
        utils::mlock(&mut ekv.pt_buffer).unwrap();
        utils::mlock(&mut ekv.pt_overflow).unwrap();

        return Ok(ekv);
    }

    fn unlock(&mut self) -> Result<(),Error> {
        return Ok()
    }

    fn lock(&mut self) -> Result<(),Error> {
        return Ok()
    }

    fn get(&mut self, key: String) -> Result<Option<String>, Error> {
        return Ok(Some(""))
    }

    fn set(&mut self, key: String, value: String) -> Result<Error> {

    }
}