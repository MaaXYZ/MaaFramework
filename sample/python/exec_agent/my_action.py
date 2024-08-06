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

# 表示当前 action 已经执行完成, 执行成功; 后续无法继续使用SyncContext
print(json.dumps({
    "return": True
}))
