//
// Created by rmq on 5/25/24.
//
#include "ChatClient.h"

int main(int argc, char *argv[]) {
    spdlog::cfg::load_env_levels();
    spdlog::info("chat client start");
    try {
        if (argc != 4) {
            std::cerr << "Usage: ChatClient <id> <host> <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;

        size_t id = static_cast<size_t>(std::atoll(argv[1]));
        if (id == ChatClient::server_client_id) {
            std::cerr << "id 65 is reserved for server, use another id";
            return 1;
        }
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(argv[2], argv[3]);
        ChatClient c(io_context, endpoints, id);

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
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}