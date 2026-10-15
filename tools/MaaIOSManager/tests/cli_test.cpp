#include "Cli.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

using namespace maa::ios::tool;

namespace
{
void check(bool condition, const char* message)
{
    if (condition) {
        return;
    }
    std::cerr << message << '\n';
    std::exit(1);
}
}

int main()
{
    Options options;
    std::string error;
    check(parse({ }, options, error) && options.command == Command::Help, "empty arguments should print help");
    check(
        parse({ "devices", "--json", "--timeout", "0", "--usb-only" }, options, error) && options.json && options.timeout_ms == 0
            && options.usb_only,
        "USB snapshot options");
    check(
        parse({ "pair", "-s", "synthetic-udid", "--wifi", "--timeout", "300000" }, options, error) && options.wifi == "auto"
            && options.udid == "synthetic-udid",
        "wireless pairing options");
    check(
        parse({ "connect", "--wifi", "fe80::1%7", "--port", "12345", "--json" }, options, error) && options.port == 12345
            && options.wifi == "fe80::1%7",
        "explicit address scope must survive parsing");
    check(
        parse({ "prepare", "--cache-dir", "/temporary/cache", "--replace", "--download-timeout", "600" }, options, error) && options.replace
            && options.download_timeout_seconds == 600,
        "DDI preparation options");
    for (const auto& arguments : std::vector<std::vector<std::string_view>> { { "disconnect" },
                                                                              { "devices", "--timeout", "-1" },
                                                                              { "devices", "--timeout", "60001" },
                                                                              { "devices", "--usb-only", "--wifi-only" },
                                                                              { "pair", "--json" },
                                                                              { "pair", "--wifi", "--repair" },
                                                                              { "connect", "--port", "42" },
                                                                              { "connect", "--wifi", "auto", "--port", "42" },
                                                                              { "connect", "--wifi", "127.0.0.1", "--port", "65536" },
                                                                              { "prepare", "--replace", "--replace" },
                                                                              { "pair", "--udid", "A", "-s", "B" },
                                                                              { "pair", "--timeout", "" },
                                                                              { "pair", "--timeout" },
                                                                              { "pair", "--timeout", "12x" },
                                                                              { "pair", "--timeout", "0" },
                                                                              { "prepare", "--install-timeout", "0" } }) {
        check(!parse(arguments, options, error) && !error.empty(), "invalid or conflicting arguments were accepted");
    }
    check(terminal_text("a\n\x1b") == "a\\x0a\\x1b", "terminal control characters must be escaped");
    check(json_string("a\n\"\\") == "\"a\\u000a\\\"\\\\\"", "JSON control characters must be escaped");
    std::ostringstream output;
    std::ostringstream errors;
    int calls = 0;
    Execute execute = [&calls](const Options&, std::ostream&, std::ostream&, const std::function<bool()>&) {
        ++calls;
        return 0;
    };
    options.command = Command::Devices;
    check(
        run(
            options,
            output,
            errors,
            [] { return true; },
            execute)
                == 130
            && calls == 0,
        "pre-cancellation must avoid discovery or side effects");
    bool cancelled = false;
    execute = [&cancelled](const Options&, std::ostream&, std::ostream&, const std::function<bool()>&) {
        cancelled = true;
        return 1;
    };
    check(
        run(
            options,
            output,
            errors,
            [&cancelled] { return cancelled; },
            execute)
            == 130,
        "cancelled I/O failure must become exit status 130");
    options.command = Command::Help;
    check(
        run(
            options,
            output,
            errors,
            [] { return true; },
            execute)
                == 0
            && output.str().find("then closes the tunnel") != std::string::npos,
        "help must state connect lifetime and require no device");
    std::cout << "MaaIOSManager CLI tests passed\n";
}
