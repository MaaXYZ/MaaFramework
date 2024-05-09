import sys
import json

# for arg in sys.argv:
#     print(arg)

output = {
    "function": "Click",
    "sync_context": sys.argv[1],
    "x": 200,
    "y": 100,
}
print(json.dumps(output, ensure_ascii=False))

res = input()
print("maafw return:", res)
