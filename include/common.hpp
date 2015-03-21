#pragma once

#include <msgpack.hpp>

enum { max_msg_length = 1024 };
typedef std::array<char, max_msg_length> buffer_t;


class chat_message
{
public:
    chat_message() {}

    std::string nickname;
    std::string message;

    MSGPACK_DEFINE( nickname, message );
};

