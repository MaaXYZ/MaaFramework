#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include <iostream>

#include "./MacOSTestGUI.h"

// 自定义视图类，用于处理键盘和鼠标事件
@interface EventHandlingView : NSView
@end

@implementation EventHandlingView

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)keyDown:(NSEvent*)event
{
    unsigned short keyCode = [event keyCode];
    NSEventModifierFlags modifiers = [event modifierFlags];
    NSString* characters = [event characters];
    NSString* charactersIgnoringModifiers = [event charactersIgnoringModifiers];

    std::cout << std::endl;

    // 打印按键修饰符
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
    std::cout << "KeyCode: " << keyCode << std::endl;

    if (characters && [characters length] > 0) {
        std::cout << "Characters: '" << [characters UTF8String] << "'" << std::endl;
    }
}

- (void)mouseDown:(NSEvent*)event
{
    NSPoint location = [event locationInWindow];
    std::cout << "鼠标点击坐标: (" << location.x << ", " << location.y << ")" << std::endl;
}

- (void)mouseDragged:(NSEvent*)event
{
    NSPoint location = [event locationInWindow];
    std::cout << "鼠标拖动坐标: (" << location.x << ", " << location.y << ")" << std::endl;
}

@end

MacOSTestGUI::MacOSTestGUI(const std::string& windowTitle)
{
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
    EventHandlingView* containerView = [[EventHandlingView alloc] initWithFrame:NSMakeRect(0, 0, 400, 400)];

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
}

void MacOSTestGUI::run()
{
    NSApplication* app = [NSApplication sharedApplication];
    [app run];
}
