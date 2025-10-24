import json
import sys

def validate_json_syntax(json_string):
    try:
        json.loads(json_string)
        return True, "Valid JSON"
    except json.JSONDecodeError as e:
        return False, f"Invalid JSON: {e}"
    
total = 0
false = 0

with open(sys.argv[1]) as f:
    content = f.readlines()

    for line in content:
        is_valid, message = validate_json_syntax(line)
        total += 1
        if not is_valid:
            false += 1
            print(line)
            print(message)

print(f"{sys.argv[1]} // Total: {total} | Failed: {false}")