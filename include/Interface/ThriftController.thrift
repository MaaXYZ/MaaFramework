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

struct InputTextParam {
  1: string text,
}

struct AppParam {
  1: string entry,
}

struct Size {
  1: i32 width,
  2: i32 height,
}

struct CustomImage {
  1: binary png_data,
}

service ThriftController {
  bool connect(),

  string request_uuid(),
  Size request_resolution(),

  bool start_app(1: AppParam param),
  bool stop_app(1: AppParam param),

  CustomImage screencap(),

  bool click(1: ClickParam param),
  bool swipe(1: SwipeParam param),
  
  bool touch_down(1: TouchParam param),
  bool touch_move(1: TouchParam param),
  bool touch_up(1: TouchParam param),

  bool press_key(1: PressKeyParam param),
  bool input_text(1: InputTextParam param),
}
