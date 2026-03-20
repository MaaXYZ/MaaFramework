#include <Carbon/Carbon.h>
#include <string>

class MacOSTestGUI
{
public:
    enum KbdEventType
    {
        KeyDown,
        KeyUp,
    };

    enum MouseEventType
    {
        MouseDown,
        MouseUp,
        MouseMove,
    };

    enum MouseButton
    {
        Left,
        Right,
        Other,
    };

    MacOSTestGUI(const std::string& windowTitle);
    ~MacOSTestGUI();

    void run();

    // 键盘事件回调
    void handleKeyEvent(
        KbdEventType type,
        unsigned short keyCode,
        uint64_t modifiers,
        std::string characters,
        std::string charactersIgnoringModifiers);
    // 鼠标事件回调
    void handleMouseEvent(MouseEventType type, MouseButton button, double x, double y, uint64_t modifiers);

private:
    // 键盘布局相关
    TISInputSourceRef keyboardLayout = nullptr;
    const UCKeyboardLayout* keyboardLayoutData = nullptr;

    std::string keyCodeToString(unsigned short keyCode);
};
