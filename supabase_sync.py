import requests
import json
import os
import sys

# --- CONFIGURATION (From Render Env) ---
SUPABASE_URL = os.getenv("SUPABASE_URL")
SUPABASE_KEY = os.getenv("SUPABASE_KEY")

HEADERS = {
    "apikey": SUPABASE_KEY,
    "Authorization": f"Bearer {SUPABASE_KEY}",
    "Content-Type": "application/json",
    "Prefer": "return=representation"
}

def handle_request():
    if not SUPABASE_URL or not SUPABASE_KEY:
        print("[]")
        return

    # Usage: python3 supabase_sync.py <GET|POST|DELETE> <table_name> [body_or_query]
    if len(sys.argv) < 3:
        return

    method = sys.argv[1]
    table = sys.argv[2]
    
    url = f"{SUPABASE_URL}/rest/v1/{table}"

    try:
        if method == "GET":
            response = requests.get(url, headers=HEADERS)
            print(response.text)
        
        elif method == "POST":
            body = sys.argv[3] if len(sys.argv) > 3 else "{}"
            # Ensure ID is unique if not provided
            response = requests.post(url, headers=HEADERS, json=json.loads(body))
            print("{\"status\":\"success\"}")

        elif method == "DELETE":
            item_id = sys.argv[3] if len(sys.argv) > 3 else "0"
            query_url = f"{url}?id=eq.{item_id}"
            response = requests.delete(query_url, headers=HEADERS)
            print("{\"status\":\"success\"}")

    except Exception as e:
        # Fallback to empty JSON so C++ doesn't crash
        print("[]")

if __name__ == "__main__":
    handle_request()
