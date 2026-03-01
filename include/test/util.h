#pragma once

#include <thread>

template<class Server, class Client>
void test_(const Server &server, const Client client) {
    std::thread t1(server);
    std::thread t2(client);

    t1.join();
    t2.join();
}