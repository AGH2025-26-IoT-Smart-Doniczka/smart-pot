import psycopg2
import os


def get_connection():
    return psycopg2.connect(
        host=os.getenv("DB_HOST", "postgres"),
        database=os.getenv("DB_NAME", "smartpot"),
        user=os.getenv("DB_USER", "smartpot"),
        password=os.getenv("DB_PASSWORD", "smartpotpass"),
    )
