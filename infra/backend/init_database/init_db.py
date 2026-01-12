import time
import psycopg2
from psycopg2 import OperationalError
from queries import queries

MAX_RETRIES = 3
RETRY_DELAY = 5
attempt = 0
conn = None

while attempt < MAX_RETRIES:
    try:
        conn = psycopg2.connect(
            host="postgres",
            database="smartpot",
            user="smartpot",
            password="smartpotpass"
        )
        print("Connected to PostgreSQL")
        break
    except OperationalError as e:
        attempt += 1
        print(f"Failed to connect (attempt {attempt}/{MAX_RETRIES}): {e}")
        if attempt < MAX_RETRIES:
            print(f"Waiting {RETRY_DELAY} seconds before retrying...")
            time.sleep(RETRY_DELAY)
        else:
            print("Could not connect after 3 attempts. Exiting.")
            exit(1)

cursor = conn.cursor()

for query in queries:
    cursor.execute(query)

conn.commit()
conn.close()
print("Tables created successfully")
