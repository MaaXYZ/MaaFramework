namespace cpp ThriftController

struct Point {
  1: i32 x,
  2: i32 y,
}

struct ClickParam {
  1: Point point,
}

struct SwipeParam {
  1: Point point1,
  2: Point point2,
  3: i32 duration,
}

struct TouchParam {
  1: i32 contact,
  2: Point point,
  3: i32 pressure,
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
  
  bool touch_down(1: TouchParam param),
  bool touch_move(1: TouchParam param),
  bool touch_up(1: TouchParam param),

  bool press_key(1: PressKeyParam param),

  bool start_game(1: string activity),
  bool stop_game(1: string activity),

  Size get_resolution(),

  string get_uuid(),
  CustomImage screencap(),
}
