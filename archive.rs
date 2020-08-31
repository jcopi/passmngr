use std;
use std::mem;
use std::io;
use std::fs::OpenOptions;
use std::collections::HashMap;

enum ArchiveMode {
    Read,
    Write,
}

enum ArchiveError {
    OpenFailed,
    ReadFailed,
    WriteFailed,
    AllocationFailed,
    InvalidOperation,
    MalformedFile,
    ItemNotFound,
    EndOfItem,
}

struct Archive {
    file:  File,
    mode:  ArchiveMode,
    items: HashMap<String, (i64, u64, u64)>
}

impl Archive {
    fn new(file_name: String, mode: ArchiveMode) -> Result<Archive, ArchiveError> {
        let archive: Archive;
        archive.mode = mode;
        archive.items = HashMap<String, (u64,u64,u64)>::new();
        let opts = match mode {
            ArchiveMode::Read => OpenOptions::new().read(true).create(false),
            ArchiveMode::Write => OpenOptions::new().write(true).create(true),
        }
        let file_result = opts.open(file_name);
        if file_result.is_err() {
            return Result<Archive, ArchiveError>::Err(ArchiveError::OpenFailed);
        }
        archive.file = file_result.unwrap();
        Result<Archive, ArchiveError>::Ok(archive)
    }

    fn load_index(&mut self) -> Result<(), ArchiveError> {
        if self.mode != ArchiveMode::Read {
            return Result<(), ArchiveError>::Err(InvalidOperation);
        }

        if self.file.seek(io::SeekFrom::End(-mem::size_of<u64>())).is_err() {
            return Result<(), ArchiveError>::Err(ReadFailed);
        }

        let mut buf: [u8; mem::size_of<u64>()];
        if self.file.read_exact(&mut buf).is_err() {
            return Result<(), ArchiveError>::Err(ReadFailed);
        }
        let index_size = u64::from_be_bytes(buf);
        if index_size > i64::MAX {
            return Result<(), ArchiveError>::Err(MalformedFile);
        }
        if self.file.seek(io::SeekFrom::Current(-index_size)).is_err() {
            return Result<(), ArchiveError>::Err(ReadFailed);
        }

        let mut buf = vec![u8; index_size];
        let i: u64 = 0;
        if self.file.read_exact(&mut buf).is_err() {
            return Result<(), ArchiveError>::Err(ReadFailed);
        }

        while (i + mem::size_of<u16>() < index_size) {
            let slc = buf[i..mem::size_of<u16>()];
            let name_length = u16::from_be_bytes(slc);
            i += mem::size_of<u16>();

            if i + name_length + (2 * mem::size_of<u64>()) > index_size {
                return Result<(), ArchiveError>::Err(MalformedFile);
            }

            let slc = buf[i..i + name_length];
            let name_result = str::from_utf8(&slc);
            if name_result.is_err() {
                return Result<(), ArchiveError>::Err(MalformedFile);
            }
            let name = name_result.unwrap();
            i += name_length;

            let slc = buf[i..i + mem::size_of<u64>()];
            let item_start = u64::from_be_bytes(slc);
            i += mem::size_of<u64>();

            let slc = buf[i..i + mem::size_of<u64>()];
            let item_length = u64::from_be_bytes(slc);
            i += mem::size_of<u64>();

            self.items.insert(name, (-1, item_start, item_length));
        }
        
        return Result<(), ArchiveError>::Ok(());
    }
}
