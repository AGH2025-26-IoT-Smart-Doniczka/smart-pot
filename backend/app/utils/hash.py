import os
from passlib.hash import argon2


def hash_password(password: str):
    salt = os.urandom(16)

    hashed = argon2.using(salt=salt).hash(password)
    return hashed
