#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include <iostream>

#include "./MacOSTestGUI.h"

// 可滚动子画布视图（使用翻转坐标，原点在左上角）
@interface CanvasView : NSView
@end

@implementation CanvasView

- (BOOL)isFlipped
{
    return YES;
}

@end

// 自定义视图类，用于处理键盘和鼠标事件
@interface EventHandlingView : NSView {
    MacOSTestGUI* _guiInstance;
}
- (instancetype)initWithFrame:(NSRect)frameRect gui:(MacOSTestGUI*)gui;
// Action for bottom input field
- (void)inputFieldAction:(id)sender;
@end

@implementation EventHandlingView

- (instancetype)initWithFrame:(NSRect)frameRect gui:(MacOSTestGUI*)gui
{
    self = [super initWithFrame:frameRect];
    if (self) {
        _guiInstance = gui;
    }
    return self;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)scrollWheel:(NSEvent*)event
{
    _guiInstance->handleScrollEvent([event scrollingDeltaX], [event scrollingDeltaY]);
    // 将滚动事件传递给父视图（NSScrollView），实现实际滚动
    [super scrollWheel:event];
}

- (void)keyDown:(NSEvent*)event
{
    _guiInstance->handleKeyEvent(
        MacOSTestGUI::KeyDown, [event keyCode], [event modifierFlags], [[event characters] UTF8String],
        [[event charactersIgnoringModifiers] UTF8String]);
}
- (void)keyUp:(NSEvent*)event
{
    _guiInstance->handleKeyEvent(
        MacOSTestGUI::KeyUp, [event keyCode], [event modifierFlags], [[event characters] UTF8String],
        [[event charactersIgnoringModifiers] UTF8String]);
}

- (void)mouseDown:(NSEvent*)event
{
    NSPoint location = [event locationInWindow];
    _guiInstance->handleMouseEvent(
        MacOSTestGUI::MouseDown, MacOSTestGUI::Left, location.x, location.y, (unsigned long long)[event modifierFlags]);
}
- (void)mouseDragged:(NSEvent*)event
{
    NSPoint location = [event locationInWindow];
    _guiInstance->handleMouseEvent(
        MacOSTestGUI::MouseMove, MacOSTestGUI::Left, location.x, location.y, (unsigned long long)[event modifierFlags]);
}
- (void)mouseUp:(NSEvent*)event
{
    NSPoint location = [event locationInWindow];
    _guiInstance->handleMouseEvent(
        MacOSTestGUI::MouseUp, MacOSTestGUI::Left, location.x, location.y, (unsigned long long)[event modifierFlags]);
}
- (void)rightMouseDown:(NSEvent*)event
{
    NSPoint location = [event locationInWindow];
    _guiInstance->handleMouseEvent(
        MacOSTestGUI::MouseDown, MacOSTestGUI::Right, location.x, location.y, (unsigned long long)[event modifierFlags]);
}
- (void)rightMouseDragged:(NSEvent*)event
{
    NSPoint location = [event locationInWindow];
    _guiInstance->handleMouseEvent(
        MacOSTestGUI::MouseMove, MacOSTestGUI::Right, location.x, location.y, (unsigned long long)[event modifierFlags]);
}
- (void)rightMouseUp:(NSEvent*)event
{
    NSPoint location = [event locationInWindow];
    _guiInstance->handleMouseEvent(
        MacOSTestGUI::MouseUp, MacOSTestGUI::Right, location.x, location.y, (unsigned long long)[event modifierFlags]);
}
- (void)otherMouseDown:(NSEvent*)event
{
    NSPoint location = [event locationInWindow];
    _guiInstance->handleMouseEvent(
        MacOSTestGUI::MouseDown, MacOSTestGUI::Other, location.x, location.y, (unsigned long long)[event modifierFlags]);
}
- (void)otherMouseDragged:(NSEvent*)event
{
    NSPoint location = [event locationInWindow];
    _guiInstance->handleMouseEvent(
        MacOSTestGUI::MouseMove, MacOSTestGUI::Other, location.x, location.y, (unsigned long long)[event modifierFlags]);
}
- (void)otherMouseUp:(NSEvent*)event
{
    NSPoint location = [event locationInWindow];
    _guiInstance->handleMouseEvent(
        MacOSTestGUI::MouseUp, MacOSTestGUI::Other, location.x, location.y, (unsigned long long)[event modifierFlags]);
}

- (void)inputFieldAction:(id)sender
{
    NSString* s = [sender stringValue];
    std::string str = s ? std::string([s UTF8String]) : std::string();
    std::cout << "InputField: " << str << std::endl;
    // 清空输入框
    [sender setStringValue:@""];
}

@end

MacOSTestGUI::MacOSTestGUI(const std::string& windowTitle)
{
    // 获取当前键盘布局
    keyboardLayout = TISCopyCurrentKeyboardLayoutInputSource();
    if (keyboardLayout) {
        CFDataRef layoutData = (CFDataRef)TISGetInputSourceProperty(keyboardLayout, kTISPropertyUnicodeKeyLayoutData);
        if (layoutData) {
            keyboardLayoutData = (const UCKeyboardLayout*)CFDataGetBytePtr(layoutData);
        }
    }

    NSApplication* app = [NSApplication sharedApplication];

    // 设置应用为普通前台应用，否则无法接收键盘事件
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];

    // 创建窗口（稍微加高以容纳布局）
    NSRect frame = NSMakeRect(100, 100, 720, 720);
    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                              | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];

    [window setTitle:[NSString stringWithUTF8String:windowTitle.c_str()]];

    // 设置窗口为顶层窗口
    // [window setLevel:NSFloatingWindowLevel];

    // 创建自定义容器视图（支持事件处理）
    EventHandlingView* containerView = [[EventHandlingView alloc] initWithFrame:NSMakeRect(0, 0, 720, 720) gui:this];

    // 可滚动子画布
    NSScrollView* scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 40, 720, 680)];
    [scrollView setHasVerticalScroller:YES];
    [scrollView setHasHorizontalScroller:NO];
    [scrollView setBorderType:NSLineBorder];

    // 子画布（翻转坐标，原点在左上角）
    CanvasView* canvasView = [[CanvasView alloc] initWithFrame:NSMakeRect(0, 0, 1440, 1360)];
    [canvasView setWantsLayer:YES];
    [canvasView.layer setBackgroundColor:[[NSColor colorWithWhite:0.95 alpha:1.0] CGColor]];

    // 🐝：在画布顶部附近（canvas y=80），初始即可见
    NSTextField* bee = [[NSTextField alloc] initWithFrame:NSMakeRect(310, 290, 100, 100)];
    [bee setStringValue:@"🐝"];
    [bee setEditable:NO];
    [bee setBordered:NO];
    [bee setBackgroundColor:[NSColor clearColor]];
    [bee setAlignment:NSTextAlignmentCenter];
    [bee setFont:[NSFont systemFontOfSize:80]];
    [canvasView addSubview:bee];

    // 🦊：在画布下半部分（canvas y=530），需要向下滚动后可见
    NSTextField* fox = [[NSTextField alloc] initWithFrame:NSMakeRect(310, 970, 100, 100)];
    [fox setStringValue:@"🦊"];
    [fox setEditable:NO];
    [fox setBordered:NO];
    [fox setBackgroundColor:[NSColor clearColor]];
    [fox setAlignment:NSTextAlignmentCenter];
    [fox setFont:[NSFont systemFontOfSize:80]];
    [canvasView addSubview:fox];

    [scrollView setDocumentView:canvasView];
    // 滚动到顶部（flipped 视图的 (0,0) 即为顶部）
    [canvasView scrollPoint:NSMakePoint(0, 0)];
    [containerView addSubview:scrollView];

    // 底部可编辑文本输入框（按回车触发 action）
    NSTextField* inputField = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 10, 700, 24)];
    [inputField setPlaceholderString:@"在此输入并按回车提交"];
    [inputField setEditable:YES];
    [inputField setBezeled:YES];
    [inputField setBezelStyle:NSTextFieldRoundedBezel];
    [inputField setTarget:containerView];
    [inputField setAction:@selector(inputFieldAction:)];
    [containerView addSubview:inputField];

    [window setContentView:containerView];

    // 显示窗口
    [window makeKeyAndOrderFront:nil];
}

MacOSTestGUI::~MacOSTestGUI()
{
    // 释放键盘布局资源
    if (keyboardLayout) {
        CFRelease(keyboardLayout);
        keyboardLayout = nullptr;
        keyboardLayoutData = nullptr;
    }
}

void MacOSTestGUI::run()
{
    NSApplication* app = [NSApplication sharedApplication];
    [app run];
}

void MacOSTestGUI::handleKeyEvent(
    KbdEventType type, unsigned short keyCode, uint64_t modifiers, std::string characters, std::string charactersIgnoringModifiers)
{
    std::cout << "键盘";
    switch (type) {
    case KeyDown:
        std::cout << "按下";
        break;
    case KeyUp:
        std::cout << "释放";
        break;
    }
    std::cout << " - ";
    if (modifiers & NSEventModifierFlagCommand) {
        std::cout << "⌘ ";
    }
    if (modifiers & NSEventModifierFlagShift) {
        std::cout << "⇧ ";
    }
    if (modifiers & NSEventModifierFlagControl) {
        std::cout << "⌃ ";
    }
    if (modifiers & NSEventModifierFlagOption) {
        std::cout << "⌥ ";
    }
    if (modifiers & NSEventModifierFlagCapsLock) {
        std::cout << "⇪ ";
    }
    // 使用缓存的键盘布局获取按键名称
    std::cout << keyCodeToString(keyCode);

    std::cout << " KeyCode: " << keyCode;

    // 将单一的 '\r' 或 '\n' 规范化为 '\r\n'
    auto normalize_newlines = [](const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            unsigned char c = s[i];
            if (c == '\r') {
                if (i + 1 < s.size() && s[i + 1] == '\n') {
                    out += "\r\n";
                    ++i;
                }
                else {
                    out += "\r\n";
                }
            }
            else if (c == '\n') {
                out += "\r\n";
            }
            else {
                out.push_back(static_cast<char>(c));
            }
        }
        return out;
    };

    std::string display_chars = normalize_newlines(characters);

    std::cout << " Characters: '" << display_chars << "'" << std::endl;
}

void MacOSTestGUI::handleMouseEvent(MouseEventType type, MouseButton button, double x, double y, unsigned long long modifiers)
{
    std::cout << "鼠标";
    switch (type) {
    case MouseDown:
        std::cout << "按下";
        break;
    case MouseUp:
        std::cout << "释放";
        break;
    case MouseMove:
        std::cout << "移动";
        break;
    }

    std::cout << " - ";
    switch (button) {
    case Left:
        std::cout << "左键";
        break;
    case Right:
        std::cout << "右键";
        break;
    case Other:
        std::cout << "其他";
        break;
    }

    std::cout << " 坐标: (" << x << ", " << y << ")" << std::endl;
}

void MacOSTestGUI::handleScrollEvent(double dx, double dy)
{
    std::cout << "滚轮事件 dx=" << dx << " dy=" << dy << std::endl;
}

std::string MacOSTestGUI::keyCodeToString(unsigned short keyCode)
{
    if (!keyboardLayoutData) {
        return "";
    }

    // 特殊情况
    if (keyCode == kVK_Return) {
        return "⏎";
    }

    // 尝试获取按键名称
    UniChar unicodeString[4];
    UniCharCount actualStringLength = 0;
    UInt32 deadKeyState = 0;

    // 使用UCKeyTranslate获取按键对应的字符
    OSStatus status = UCKeyTranslate(
        keyboardLayoutData, keyCode, kUCKeyActionDisplay,
        0, // 无修饰符
        LMGetKbdType(), kUCKeyTranslateNoDeadKeysBit, &deadKeyState, sizeof(unicodeString) / sizeof(unicodeString[0]), &actualStringLength,
        unicodeString);

    if (status == noErr && actualStringLength > 0) {
        return std::string([NSString stringWithCharacters:unicodeString length:actualStringLength].UTF8String);
    }

    return "";
}
