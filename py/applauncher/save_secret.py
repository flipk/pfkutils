#!/usr/bin/env python3

import os
import sys
import base64
from cryptography.hazmat.primitives.kdf.scrypt import Scrypt
from cryptography.fernet import Fernet, InvalidToken
import secrets
import string

def _derive_key(pin: str, salt: bytes) -> bytes:
    """
    Derives a 32-byte URL-safe base64 key from a PIN and a salt.
    """
    # Scrypt parameters tuned to be intentionally SLOW.
    # N=2**20 means this will take roughly ~1 second to compute on a modern CPU.
    # If a 6-digit PIN takes 1 second per guess, an attacker needs ~11.5 days to check all 1 million.
    kdf = Scrypt(
        salt=salt,
        length=32,
        n=2**20, # CPU/Memory cost (increase to make it slower, decrease to make it faster)
        r=8,     # Block size
        p=1,     # Parallelization
    )
    key = kdf.derive(pin.encode('utf-8'))
    return base64.urlsafe_b64encode(key)

def save_secret(secret: str, pin: str, filename: str):
    """Encrypts a secret string and saves it to a file using a PIN."""
    # 1. Generate a random 16-byte salt
    salt = os.urandom(16)
    
    # 2. Derive the encryption key from the PIN + Salt
    key = _derive_key(pin, salt)
    
    # 3. Encrypt the data
    f = Fernet(key)
    encrypted_data = f.encrypt(secret.encode('utf-8'))

    # 4. Save salt + encrypted data together. 
    # The salt isn't a secret, but it is required to re-derive the key later.
    with open(filename, 'wb') as file:
        file.write(salt + encrypted_data)
    print(f"Secret successfully secured in {filename}.")

def load_secret(pin: str, filename: str) -> str:
    """Loads and decrypts a secret string from a file using a PIN."""
    if not os.path.exists(filename):
        raise Exception("File not found.")

    with open(filename, 'rb') as file:
        file_data = file.read()

    # 1. Extract the first 16 bytes for the salt, the rest is the encrypted data
    salt = file_data[:16]
    encrypted_data = file_data[16:]

    # 2. Derive the exact same key using the provided PIN + extracted salt
    key = _derive_key(pin, salt)
    f = Fernet(key)

    # 3. Attempt to decrypt
    decrypted_data = f.decrypt(encrypted_data)
    return decrypted_data.decode('utf-8')

def generate_totp_secret(length=32):
    """
    Generates a 32-character Base32-compliant TOTP secret.
    Base32 alphabet: A-Z and 2-7.
    """
    # Standard Base32 alphabet used by TOTP apps
    base32_chars = string.ascii_uppercase + "234567"
    
    # Securely pick random characters from the alphabet
    secret = ''.join(secrets.choice(base32_chars) for _ in range(length))
    
    return secret

def _test_usage():
    print('usage: save_secret.py save secret.bin pin secret\n'
          '       save_secret.py load secret.bin pin\n'
          '       save_secret.py gen\n')
    exit(1)

def _test_main(args):
    if len(args) == 1:
        _test_usage()
    if args[1] == 'save' and len(args) == 5:
        save_secret(args[4], args[3], args[2])
    elif args[1] == 'load' and len(args) == 4:
        try:
            secret = load_secret(args[3], args[2])
            print(f'the secret is: {secret}')
        except:
            print('incorrect PIN or corrupted data.')
    elif args[1] == 'gen':
        secret = generate_totp_secret(32)
        print(f'new secret:  {secret}')
    else:
        _test_usage()
    return 0

# --- Example Usage ---
if __name__ == "__main__":
    exit(_test_main(sys.argv))
