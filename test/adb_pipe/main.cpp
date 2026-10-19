#include "Base/UnitBase.h"

#include <cerrno>
#include <iostream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

using namespace std::chrono_literals;

class TestUnit : public MaaNS::CtrlUnitNs::UnitBase
{
public:
    bool parse(const json::value&) override { return true; }

    using UnitBase::startup_and_read_pipe;
};

static int writer(const std::string& mode)
{
    if (mode == "empty") {
        return 0;
    }
    if (mode == "nonzero") {
        std::cout << "failure";
        return 42;
    }
    if (mode == "inherited_writer") {
        const pid_t pid = fork();
        if (pid < 0) {
            return 1;
        }
        if (pid == 0) {
            std::this_thread::sleep_for(2s);
            _exit(0);
        }
        return 0;
    }
    std::cout << "early" << std::flush;
    if (mode == "grace_exit") {
        std::this_thread::sleep_for(150ms);
        std::cout << "late";
    }
    else if (mode == "timeout_retry") {
        std::this_thread::sleep_for(2s);
    }
    else if (mode == "eof_before_exit") {
        close(STDOUT_FILENO);
        // A sleeping writer can outlive the fixed cleanup grace on a busy
        // runner. Exercise the EOF-before-exit ordering without that race.
        _exit(0);
    }
    else if (mode == "complete") {
        std::cout << "late";
    }
    else {
        return 2;
    }
    return 0;
}

int main(int argc, char** argv)
{
    if (argc == 3 && std::string(argv[1]) == "--child") {
        return writer(argv[2]);
    }
    if (argc != 2) {
        return 2;
    }
    const std::string mode = argv[1];
    const auto executable = std::filesystem::absolute(argv[0]);
    TestUnit unit;
    const auto start = std::chrono::steady_clock::now();
    const auto result = unit.startup_and_read_pipe({ executable, { "--child", mode } }, 100ms);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    std::cout << mode << ": elapsed_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
              << " accepted=" << result.has_value() << '\n';
    if (elapsed >= 700ms) {
        std::cerr << "read and cleanup exceeded the deadline allowance\n";
        return 1;
    }

    bool valid = false;
    if (mode == "complete") {
        valid = result && *result == "earlylate";
    }
    else if (mode == "empty") {
        valid = result && result->empty();
    }
    else if (mode == "eof_before_exit") {
        valid = result && *result == "early";
    }
    else if (mode == "grace_exit" || mode == "timeout_retry" || mode == "inherited_writer" || mode == "nonzero") {
        valid = !result;
    }
    if (!valid) {
        std::cerr << "unexpected result: " << result.value_or("<null>") << '\n';
        return 1;
    }
    if (mode == "timeout_retry") {
        const auto retry = unit.startup_and_read_pipe({ executable, { "--child", "complete" } }, 1s);
        if (!retry || *retry != "earlylate") {
            std::cerr << "next command did not succeed after timeout\n";
            return 1;
        }
    }
    int status = 0;
    if (waitpid(-1, &status, WNOHANG) != -1 || errno != ECHILD) {
        std::cerr << "direct child was not reaped\n";
        return 1;
    }
}
