# passmngr

Data will be organized into seperate files:
 - A `keys` file will hold encryption keys for decrypting other files. The keys file will have a salt attached somewhere in the meta data that will be used to create the master encryption key from the user password. This will be verified as associated data in the AEAD used to secure the keys file.
 - An encrypted `accounts` file will hold information on user accounts. This will include domains/app identifiers, usernames, passwords and other meta data to facilitate searching. 
 - An encrypted `settings` file will hold user preferences. password timeout settings, master password keygen parameters, etc. 

The initial MVP will not use libsodium allocations, this will bring the risk of sensitive data being paged to RAM, testing will be done to determine the best way to utilize the limited amount of non-paged ram available.