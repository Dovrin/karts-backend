import requests
import json
import os
import sys

# --- CONFIGURATION (From Render Env) ---
SUPABASE_URL = os.getenv("SUPABASE_URL")
SUPABASE_KEY = os.getenv("SUPABASE_KEY")

# Remove trailing slash if present
if SUPABASE_URL and SUPABASE_URL.endswith('/'):
    SUPABASE_URL = SUPABASE_URL[:-1]

HEADERS = {
    "apikey": SUPABASE_KEY,
    "Authorization": f"Bearer {SUPABASE_KEY}",
    "Content-Type": "application/json",
    "Prefer": "return=representation"
}

def log(msg):
    """Log to stderr so it shows up in Render logs"""
    sys.stderr.write(f"[Python Bridge] {msg}\n")

def handle_request():
    if not SUPABASE_URL or not SUPABASE_KEY:
        log("ERROR: SUPABASE_URL or SUPABASE_KEY missing in environment variables.")
        print("[]")
        return

    if len(sys.argv) < 3:
        log("ERROR: Missing arguments. Use: sync.py <METHOD> <TABLE> [BODY/ID]")
        print("[]")
        return

    method = sys.argv[1]
    table = sys.argv[2]
    
    url = f"{SUPABASE_URL}/rest/v1/{table}"

    try:
        if method == "GET":
            log(f"Fetching from {table}...")
            response = requests.get(url, headers=HEADERS)
            if response.status_code == 200:
                print(response.text)
            else:
                log(f"GET Failed: {response.status_code} - {response.text}")
                print("[]")
        
        elif method == "POST":
            # Read JSON body from stdin to avoid quote issues in shell
            body_str = sys.stdin.read()
            log(f"Posting to {table}...")
            response = requests.post(url, headers=HEADERS, json=json.loads(body_str))
            if response.status_code in [200, 201]:
                print("{\"status\":\"success\"}")
            else:
                log(f"POST Failed: {response.status_code} - {response.text}")
                print("{\"status\":\"error\"}")

        elif method == "DELETE":
            item_id = sys.argv[3] if len(sys.argv) > 3 else "0"
            log(f"Deleting from {table} where id={item_id}...")
            query_url = f"{url}?id=eq.{item_id}"
            response = requests.delete(query_url, headers=HEADERS)
            if response.status_code in [200, 204]:
                print("{\"status\":\"success\"}")
            else:
                log(f"DELETE Failed: {response.status_code} - {response.text}")
                print("{\"status\":\"error\"}")

    except Exception as e:
        log(f"EXCEPTION: {str(e)}")
        print("[]")

if __name__ == "__main__":
    handle_request()
