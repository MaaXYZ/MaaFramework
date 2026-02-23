#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include <iostream>

#include "./MacOSTestGUI.h"

MacOSTestGUI::MacOSTestGUI(const std::string& windowTitle)
{
    NSApplication* app = [NSApplication sharedApplication];

    // 创建窗口
    NSRect frame = NSMakeRect(100, 100, 400, 400);
    NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                              | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];

    [window setTitle:[NSString stringWithUTF8String:windowTitle.c_str()]];

    // 创建容器视图
    NSView* containerView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 400, 400)];

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
