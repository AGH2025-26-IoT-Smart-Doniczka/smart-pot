import os
from passlib.hash import argon2

def hash_password(password: str):
    salt = os.urandom(16)
    salt_hex = salt.hex()

    hashed = argon2.using(salt=salt).hash(password)
    return hashed, salt_hex

def verify_password(password: str, hashed: str, salt_hex: str) -> bool:
    return argon2.using(salt=salt_hex).verify(password, hashed)
    