#include "utils.hpp"

int main(int argc, char* argv[])
{
    using namespace MaaNS::ControllerNS;

    auto repl = parse_argc(argc, argv);
    auto cfg = json::parse(read_adb_argv(std::filesystem::current_path().parent_path() / "test" / "config"));
    auto io = PlatformFactory::create();
    MaaSetGlobalOption(MaaGlobalOption_Logging, (std::filesystem::current_path() / "debug").string().c_str());

    auto connect = new Unit::Connection();
    connect->set_io(io);

    connect->parse(cfg.value());
    connect->set_replacement(repl);

    std::cout << connect->connect() << std::endl;
}