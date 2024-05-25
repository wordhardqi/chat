//
// Created by rmq on 5/25/24.
//

#include "ChatServer.h"
#include  "ChatClient.h"

int main(int argc, char *argv[]) {
    spdlog::cfg::load_env_levels();
    spdlog::info("server start");
    try {
        if (argc < 2) {
            std::cerr << "Usage: ChatServer <port>\n";
            return 1;
        }
        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context);
        signals.add(SIGINT);
        signals.add(SIGTERM);

        std::unique_ptr<ChatServer> server;
        tcp::endpoint endpoint(tcp::v4(), static_cast<unsigned short>(std::atoi(argv[1])));
        server = std::make_unique<ChatServer>(io_context, endpoint);
        signals.async_wait([&](boost::system::error_code /*ec*/, int /*signo*/) {
            server->close();
            spdlog::warn("server exits");
            io_context.stop();
        });

        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("localhost", argv[1]);
        ChatClient c(io_context, endpoints, ChatClient::server_client_id);
        std::thread t([&io_context]() { io_context.run(); });


        char line[ChatMessage::max_body_length + 1];
        while (std::cin.getline(line, ChatMessage::max_body_length + 1)) {
            ChatMessage msg(ChatMessage::msg_type_new, c.get_id(), generate_seq(), std::strlen(line));
            std::memcpy(msg.body(), line, msg.body_length());
            c.write(msg);
        }

        c.close();
        t.join();

    }
    catch (std::exception &e) {
        spdlog::warn("Exception: {}", e.what());
    }

    return 0;
}
