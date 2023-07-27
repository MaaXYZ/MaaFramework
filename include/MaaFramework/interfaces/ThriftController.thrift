namespace cpp ThriftController

struct Point {
  1: i32 x,
  2: i32 y,
}

struct ClickParam {
  1: Point point,
}

struct SwipeStep {
  1: Point point,
  2: i32 delay,
}

struct SwipeParam {
  1: list<SwipeStep> steps,
}
struct PressKeyParam {
  1: i32 keycode,
}

struct Size {
  1: i32 width,
  2: i32 height,
}

struct CustomImage {
  1: Size size,
  2: i32 type,
  3: binary data,
}

service ThriftController {
  bool set_option(1: string key, 2: string value),

  bool connect(),
  bool click(1: ClickParam param),
  bool swipe(1: SwipeParam param),
  bool press_key(1: PressKeyParam param),

  bool start_game(1: string activity),
  bool stop_game(1: string activity),

  Size get_resolution(),

  string get_uuid(),
  CustomImage screencap(),
}
