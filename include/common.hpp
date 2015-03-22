#pragma once

#include <iostream>
#include <msgpack.hpp>

enum { max_msg_length = 1024 };
typedef std::array<char, max_msg_length> buffer_t;


class chat_message
{
public:
    chat_message() {}

    std::string nickname;
    std::string message;

    friend std::ostream& operator<<( std::ostream& out, const chat_message& obj )
    {
        out << obj.nickname << ": " << obj.message;
        return out;
    }

    MSGPACK_DEFINE( nickname, message );
};

