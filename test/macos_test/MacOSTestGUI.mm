#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include <iostream>

#include "./MacOSTestGUI.h"

// 自定义视图类，用于处理键盘和鼠标事件
@interface EventHandlingView : NSView {
    MacOSTestGUI* _guiInstance;
}
- (instancetype)initWithFrame:(NSRect)frameRect gui:(MacOSTestGUI*)gui;
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

    // 创建窗口
    NSRect frame = NSMakeRect(100, 100, 400, 400);
    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                              | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];

    [window setTitle:[NSString stringWithUTF8String:windowTitle.c_str()]];

    // 设置窗口为顶层窗口
    [window setLevel:NSFloatingWindowLevel];

    // 创建自定义容器视图（支持事件处理）
    EventHandlingView* containerView = [[EventHandlingView alloc] initWithFrame:NSMakeRect(0, 0, 400, 400) gui:this];

    // 创建文本标签（上面）
    NSTextField* textField = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 320, 400, 60)];
    [textField setStringValue:@"这是一个测试窗口，用于 MaaFW 识别。"];
    [textField setEditable:NO];
    [textField setBordered:NO];
    [textField setBackgroundColor:[NSColor clearColor]];
    [textField setAlignment:NSTextAlignmentCenter];
    [textField setFont:[NSFont systemFontOfSize:14]];
    [containerView addSubview:textField];

    // 创建 emoji 标签（下面居中）
    NSTextField* emojiField = [[NSTextField alloc] initWithFrame:NSMakeRect(150, 150, 100, 100)];
    [emojiField setStringValue:@"🐝"];
    [emojiField setEditable:NO];
    [emojiField setBordered:NO];
    [emojiField setBackgroundColor:[NSColor clearColor]];
    [emojiField setAlignment:NSTextAlignmentCenter];
    [emojiField setFont:[NSFont systemFontOfSize:100]];
    [containerView addSubview:emojiField];

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
    std::cout << std::endl;

    // 打印按键
    std::cout << "Key: ";
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
    std::cout << keyCodeToString(keyCode) << std::endl;

    std::cout << "KeyCode: " << keyCode << std::endl;

    std::cout << "Characters: '" << characters << "'" << std::endl;
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

std::string MacOSTestGUI::keyCodeToString(unsigned short keyCode)
{
    if (!keyboardLayoutData) {
        return "";
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
