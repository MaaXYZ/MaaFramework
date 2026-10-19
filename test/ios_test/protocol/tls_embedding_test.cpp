#include "startup_tls_budget.h"
#include "tls_psk_move.h"
#include "tunnel_tls_failure.h"
#include <cstdio>
#ifndef _WIN32
#include <csignal>
#endif
namespace
{
int failures = 0;

void check(bool ok, const char* message)
{
    if (!ok) {
        ++failures;
        std::fprintf(stderr, "FAIL: %s\n", message);
    }
}
#ifndef _WIN32
volatile std::sig_atomic_t received_sigpipe = 0;

void host_sigpipe_handler(int)
{
    received_sigpipe = 1;
}
#endif
}

int main()
{
#ifndef _WIN32
    struct sigaction before { }, saved { };
    before.sa_handler = host_sigpipe_handler;
    sigemptyset(&before.sa_mask);
    sigaction(SIGPIPE, &before, &saved);
#endif
    maa_ios_test::tls_psk_moves(check);
    maa_ios_test::startup_tls_budget(check);
    maa_ios_test::tunnel_tls_failures(check);
    {
        maa::ios::transport::Socket client, peer;
        std::string error;
        check(make_test_socket_pair(client, peer, error), "closed-peer fixture");
        peer.close();
#ifndef _WIN32
        received_sigpipe = 0;
#endif
        maa::ios::transport::TlsChannel tls;
        check(
            !tls.handshake_psk(client, std::vector<uint8_t>(32, 1), error) && !error.empty(),
            "closed-peer TLS fails without killing its embedding process");
#ifndef _WIN32
        check(received_sigpipe == 0, "TLS never delivers SIGPIPE to the embedding host");
#endif
    }
#ifndef _WIN32
    struct sigaction after { };
    sigaction(SIGPIPE, nullptr, &after);
    check(after.sa_handler == host_sigpipe_handler, "TLS leaves the embedding host's SIGPIPE disposition unchanged");
    sigaction(SIGPIPE, &saved, nullptr);
#endif
    return failures ? 1 : 0;
}
