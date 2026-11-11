#!/usr/bin/env python3

# This implementation locks using Argon2id for key derivation and
# AES-256-GCM for encryption, which is a very strong enterprise-grade
# configuration.
#
# this configuration requires 128 MB of RAM, which makes high
# parallelization attacks on GPUs infeasable. Argon2id is also highly
# resistant to cache-timing attacks, which Scrypt is not.
#
# this is not significantly more secure in practice, however it is
# more future-proof than the Scrypt+Fernet configuration of the other
# file.

import os
import sys
import base64
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives.kdf.argon2 import Argon2id
import secrets
import string

memory_cost = 131072  # 128 MB

def _derive_key(passphrase: str, salt: bytes) -> bytes:
    """
    Derives a 32-byte URL-safe base64 key from a passprhase and a salt.
    """
    kdf = Argon2id(
        salt=salt,
        length=32,            # 32 bytes = 256-bit key
        iterations=3,
        lanes=4,
        memory_cost=memory_cost,   # 128 MB
        ad=None,
        secret=None,
    )
    return kdf.derive(passphrase.encode("utf-8"))

def save_secret(secret: str, passphrase: str, filename: str):
    """Encrypts a secret string and saves it to a file using a passphrase."""
    # one rule for using AES-GCM -- every time you ever write the secret file,
    # you MUST regenerate a new random nonce. Never, ever reuse a nonce: this
    # is a catastrophic failure mode of AES-GCM.
    salt = os.urandom(16)
    nonce = os.urandom(12)
    key = _derive_key(passphrase, salt)
    aesgcm = AESGCM(key)
    encrypted_data = aesgcm.encrypt(nonce, secret.encode(), None)
    try:
        # make the file writable if it exists
        os.chmod(filename, 0o600)
    except:
        # but ignore errors if it does not.
        pass
    with open(filename, 'wb') as file:
        os.chmod(filename, 0o400)
        file.write(salt + nonce + encrypted_data)
    print(f"Secret successfully secured in {filename}.")

def load_secret(passphrase: str, filename: str) -> (str | None, str | None):
    """Loads and decrypts a secret string from a file using a PIN.
    returns: (None, passphrase), or (error string, None)"""
    if not os.path.exists(filename):
        return (f"File {filename} not found.", None)
    with open(filename, 'rb') as file:
        file_data = file.read()
    salt = file_data[:16]
    nonce = file_data[16:28]
    encrypted_data = file_data[28:]
    key = _derive_key(passphrase, salt)
    aesgcm = AESGCM(key)
    try:
        decrypted_data = aesgcm.decrypt(nonce, encrypted_data, None)
    except:
        return (f'FAILED DECRYPT: is pin incorrect?', None)
    return (None, decrypted_data.decode('utf-8'))

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
            err, secret = load_secret(args[3], args[2])
            if secret:
                print(f'the secret is: {secret}')
            else:
                print(f'Error: {err}')
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
