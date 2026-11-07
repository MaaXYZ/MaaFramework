"""Exercise the real framework C ABI against a deterministic XDB1 server."""
import ctypes as C
import json
from pathlib import Path
import sys
import tempfile

root = Path(sys.argv[1])
sys.path.insert(0, str(root / '3rdparty/XDeviceBridge/tests/support'))
from fake_server import FakeServer


class InterruptedVideoServer(FakeServer):
    frame_unavailable = False

    def handle_call(self, name, params):
        if name == 'video.frame' and self.frame_unavailable:
            with self.lock:
                self.calls.append((name, params))
            raise ValueError('No newer frame within the requested timeout')
        return super().handle_call(name, params)


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

create = function(fw, 'MaaIOSControllerCreate', P, [C.c_char_p, C.c_uint64, I, C.c_char_p])
destroy = function(fw, 'MaaControllerDestroy', None, [P])
wait = function(fw, 'MaaControllerWait', I, [P, Q])
set_option = function(fw, 'MaaControllerSetOption', B, [P, I, P, C.c_uint64])
posts = {}
for suffix, args in [('Connection', []), ('Screencap', []), ('Click', [I,I]), ('InputText', [C.c_char_p]), ('KeyDown',[I]), ('KeyUp',[I]), ('TouchDown',[I,I,I,I]), ('TouchMove',[I,I,I,I]), ('TouchUp',[I]), ('StartApp',[C.c_char_p]), ('StopApp',[C.c_char_p])]:
    posts[suffix] = function(fw, 'MaaControllerPost' + suffix, Q, [P] + args)
image_create = function(fw, 'MaaImageBufferCreate', P, [])
image_destroy = function(fw, 'MaaImageBufferDestroy', None, [P])
cached = function(fw, 'MaaControllerCachedImage', B, [P,P])
width = function(fw, 'MaaImageBufferWidth', I, [P])
height = function(fw, 'MaaImageBufferHeight', I, [P])
pixels = function(fw, 'MaaImageBufferGetRawData', P, [P])
list_create = function(tk, 'MaaToolkitIOSDeviceListCreate', P, [])
list_destroy = function(tk, 'MaaToolkitIOSDeviceListDestroy', None, [P])
find = function(tk, 'MaaToolkitIOSDeviceFindWithOptions', B, [I,C.c_char_p,P,P,P])
size = function(tk, 'MaaToolkitIOSDeviceListSize', C.c_uint64, [P])
resource_create = function(fw, 'MaaResourceCreate', P, [])
resource_destroy = function(fw, 'MaaResourceDestroy', None, [P])
resource_bundle = function(fw, 'MaaResourcePostBundle', Q, [P, C.c_char_p])
resource_wait = function(fw, 'MaaResourceWait', I, [P, Q])
tasker_create = function(fw, 'MaaTaskerCreate', P, [])
tasker_destroy = function(fw, 'MaaTaskerDestroy', None, [P])
bind_resource = function(fw, 'MaaTaskerBindResource', B, [P, P])
bind_controller = function(fw, 'MaaTaskerBindController', B, [P, P])
post_task = function(fw, 'MaaTaskerPostTask', Q, [P, C.c_char_p, C.c_char_p])
tasker_wait = function(fw, 'MaaTaskerWait', I, [P, Q])


def multiswipe_pipeline(ctrl, server):
    with tempfile.TemporaryDirectory(prefix='maa-ios-multiswipe-') as directory:
        pipeline = Path(directory) / 'pipeline'
        pipeline.mkdir()
        node = dict(recognition='DirectHit', action='MultiSwipe', pre_delay=0, post_delay=0,
                    swipes=[dict(begin=[1, 1, 1, 1], end=[2, 1, 1, 1], duration=40, contact=i) for i in range(5)])
        (pipeline / 'multitouch.json').write_text(json.dumps(dict(MultiTouchRegression=node)))
        resource, tasker = resource_create(), tasker_create()
        try:
            assert resource_wait(resource, resource_bundle(resource, directory.encode())) == 3000
            assert bind_resource(tasker, resource) and bind_controller(tasker, ctrl)
            before = len(server.calls)
            assert tasker_wait(tasker, post_task(tasker, b'MultiTouchRegression', b'{}')) == 3000
            active, maximum = set(), 0
            for method, params in server.calls[before:]:
                if method != 'input.touch':
                    continue
                if params['down']:
                    active.add(params['contact'])
                else:
                    active.discard(params['contact'])
                maximum = max(maximum, len(active))
            assert maximum == 5 and not active, 'existing MultiSwipe must overlap five contacts and lift them all'
        finally:
            tasker_destroy(tasker)
            resource_destroy(resource)

with FakeServer() as server:
    config = json.dumps(server.config).encode()
    devices = list_create()
    try:
        assert find(0, config, None, None, devices) and size(devices) == 1
    finally:
        list_destroy(devices)
    assert not create(b'', 3, 0, config)
    assert not create(None, 3, 0, config)
    for method in (1, 2, 3):
        server.no_video = method == 3  # Both must fall back after video-start failure.
        ctrl = create(b'mock-iphone', method, 2, config)
        assert ctrl
        try:
            raw = B(1)
            assert set_option(ctrl, 3, C.byref(raw), C.sizeof(raw))
            def action(name, *args):
                result = posts[name](ctrl, *args)
                assert result
                return wait(ctrl, result)
            assert action('Connection') == 3000
            if method != 2:
                started = [p for name, p in server.calls if name == 'video.start'][-1]
                assert started['hardware_decode'] is False
            assert action('Screencap') == 3000
            image = image_create()
            try:
                assert cached(ctrl, image)
                assert (width(image), height(image)) == (5, 3), 'crop/rotation/PNG geometry mismatch'
                assert C.string_at(pixels(image), 3) == bytes([9,41,93]), 'BGR pixel conversion mismatch'
            finally:
                image_destroy(image)
            assert action('Click', 2, 1) == 3000
            for contact in range(5):
                assert action('TouchDown', contact, 1, 1, 0) == 3000
            assert action('TouchDown', 0, 1, 1, 0) == 4000
            assert action('TouchMove', 4, 2, 1, 0) == 3000
            assert action('TouchUp', 2) == 3000
            assert action('TouchMove', 2, 2, 1, 0) == 4000
            assert action('TouchMove', 3, 2, 1, 0) == 3000
            assert action('TouchDown', 5, 1, 1, 0) == 4000
            assert action('TouchDown', -1, 1, 1, 0) == 4000
            for contact in (0, 1, 3, 4):
                assert action('TouchUp', contact) == 3000
            touch_events = [p for name, p in server.calls if name == 'input.touch']
            assert {p['contact'] for p in touch_events if p['down']} == set(range(5))
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
                multiswipe_pipeline(ctrl, server)
        finally:
            destroy(ctrl)
    assert not {'pair', 'pairing.import', 'ddi.prepare'} & set(server.methods())
    assert 'connection.close' in server.methods()
    opens = [p for name, p in server.calls if name == 'connection.open']
    assert all(p.get('access', 'control') == 'control' and p['port'] == 49152 for p in opens)

with InterruptedVideoServer() as server:
    config = json.dumps(server.config).encode()
    for method in (1, 3):
        before = len(server.calls)
        ctrl = create(b'mock-iphone', method, 2, config)
        assert ctrl
        try:
            assert wait(ctrl, posts['Connection'](ctrl)) == 3000
            assert wait(ctrl, posts['Screencap'](ctrl)) == 3000
            server.frame_unavailable = True
            expected = 4000 if method == 1 else 3000
            assert wait(ctrl, posts['Screencap'](ctrl)) == expected
            server.frame_unavailable = False
            assert wait(ctrl, posts['Screencap'](ctrl)) == 3000
            calls = server.calls[before:]
            starts = [params for name, params in calls if name == 'video.start']
            assert len(starts) == 1 and starts[0].get('restart_on_silence', True)
            assert not any(name == 'video.stop' for name, _ in calls)
            assert sum(name == 'connection.open' for name, _ in calls) == 1
            assert sum(name == 'screenshot' for name, _ in calls) == (method == 3)
        finally:
            server.frame_unavailable = False
            destroy(ctrl)
with FakeServer() as server:
    for options in ({'wifi_port': 0}, {'wifi_port': 65536}, {'wifi_port': 49152.5},
                    {'wifi_port': True}, {'wifi_address': 123}, {'wifi_address': 'a\0b'}, {'shared_control': 'true'},
                    {'max_bitrate': -1}, {'max_bitrate': 1.5}, {'max_bitrate': True},
                    {'max_bitrate': 2**64}, {'max_fps': 241}, {'max_fps': 1.5},
                    {'hardware_decode': 1}):
        config = json.dumps({**server.config, **options}).encode()
        ctrl = create(b'mock-iphone', 1, 2, config)
        assert ctrl
        try:
            assert wait(ctrl, posts['Connection'](ctrl)) == 4000
        finally:
            destroy(ctrl)
    assert 'connection.open' not in server.methods(), 'invalid options must fail before opening a session'

    server.shared_control = True
    config = json.dumps({**server.config, 'shared_control': True, 'wifi_address': '127.0.0.2', 'wifi_port': 49153,
                         'max_bitrate': 2**64 - 1, 'max_fps': 15, 'hardware_decode': True}).encode()
    ctrl = create(b'mock-iphone', 1, 2, config)
    try:
        assert wait(ctrl, posts['Connection'](ctrl)) == 3000
        opened = [p for name, p in server.calls if name == 'connection.open'][-1]
        assert opened['access'] == 'shared' and opened['wifi'] == '127.0.0.2' and opened['port'] == 49153
        started = [p for name, p in server.calls if name == 'video.start'][-1]
        assert started['max_bitrate'] == 2**64 - 1 and started['max_fps'] == 15 and started['hardware_decode'] is True
    finally:
        destroy(ctrl)

print('PASS: native xdb discovery, Stream/PNG/fallback, geometry, input, app control and ownership')

