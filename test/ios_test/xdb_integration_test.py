"""Exercise the real framework C ABI against a deterministic XDB1 server."""
import ctypes as C
import json
from pathlib import Path
import sys

root = Path(sys.argv[1])
sys.path.insert(0, str(root / '3rdparty/XDeviceBridge/tests/support'))
from fake_server import FakeServer

fw = C.CDLL(sys.argv[2])
tk = C.CDLL(sys.argv[3])
P = C.c_void_p
B = C.c_uint8
I = C.c_int32
Q = C.c_int64

def function(lib, name, result, args):
    value = getattr(lib, name)
    value.restype, value.argtypes = result, args
    return value

create = function(fw, 'MaaIOSControllerCreate', P, [C.c_char_p, C.c_uint64, I, C.c_char_p, C.c_char_p])
destroy = function(fw, 'MaaControllerDestroy', None, [P])
wait = function(fw, 'MaaControllerWait', I, [P, Q])
set_option = function(fw, 'MaaControllerSetOption', B, [P, I, P, C.c_uint64])
posts = {}
for suffix, args in [('Connection', []), ('Screencap', []), ('Click', [I,I]), ('InputText', [C.c_char_p]), ('KeyDown',[I]), ('KeyUp',[I]), ('StartApp',[C.c_char_p]), ('StopApp',[C.c_char_p])]:
    posts[suffix] = function(fw, 'MaaControllerPost' + suffix, Q, [P] + args)
image_create = function(fw, 'MaaImageBufferCreate', P, [])
image_destroy = function(fw, 'MaaImageBufferDestroy', None, [P])
cached = function(fw, 'MaaControllerCachedImage', B, [P,P])
width = function(fw, 'MaaImageBufferWidth', I, [P])
height = function(fw, 'MaaImageBufferHeight', I, [P])
pixels = function(fw, 'MaaImageBufferGetRawData', P, [P])
list_create = function(tk, 'MaaToolkitIOSDeviceListCreate', P, [])
list_destroy = function(tk, 'MaaToolkitIOSDeviceListDestroy', None, [P])
find = function(tk, 'MaaToolkitIOSDeviceFindWithOptions', B, [I,C.c_char_p,P])
size = function(tk, 'MaaToolkitIOSDeviceListSize', C.c_uint64, [P])

with FakeServer() as server:
    config = json.dumps(server.config).encode()
    devices = list_create()
    try:
        assert find(0, config, devices) and size(devices) == 1
    finally:
        list_destroy(devices)
    assert not create(b'', 3, 0, None, config)
    for method in (1, 2, 3):
        server.no_video = method == 3  # Both must fall back after video-start failure.
        ctrl = create(b'mock-iphone', method, 2, b'auto', config)
        assert ctrl
        try:
            raw = B(1)
            assert set_option(ctrl, 3, C.byref(raw), C.sizeof(raw))
            def action(name, *args):
                result = posts[name](ctrl, *args)
                assert result
                return wait(ctrl, result)
            assert action('Connection') == 3000
            assert action('Screencap') == 3000
            image = image_create()
            try:
                assert cached(ctrl, image)
                assert (width(image), height(image)) == (5, 3), 'crop/rotation/PNG geometry mismatch'
                assert C.string_at(pixels(image), 3) == bytes([9,41,93]), 'BGR pixel conversion mismatch'
            finally:
                image_destroy(image)
            assert action('Click', 2, 1) == 3000
            touches = [p for name,p in server.calls if name == 'input.touch' and p['down']]
            assert abs(touches[-1]['x'] - 1/3) < 1e-6 and abs(touches[-1]['y'] - .6) < 1e-6
            assert action('KeyDown', 113) == 3000
            assert action('KeyUp', 113) == 3000
            before = server.methods().count('input.text')
            assert action('InputText', '中文'.encode()) == 4000
            assert server.methods().count('input.text') == before
            assert action('InputText', b'abc123') == 3000
            assert action('StartApp', b'test.bundle') == 3000
            assert action('StopApp', b'test.bundle') == 3000
            if method == 1:
                server.bad_frame = True
                assert action('Screencap') == 4000, 'truncated BGRA frame accepted'
                server.bad_frame = False
        finally:
            destroy(ctrl)
    assert not {'pair', 'pairing.import', 'ddi.prepare'} & set(server.methods())
    assert 'connection.close' in server.methods()
print('PASS: native xdb discovery, Stream/PNG/fallback, geometry, input, app control and ownership')
