#include <iostream>

#include "matrix.h"

int main (int argc, char *argv[])
{
  Matrix matrix("config.json");
  matrix.login();
  matrix.join();
  matrix.setMessageFilter();
  matrix.sync();

  while (true)
  {
    json data{matrix.sync()};
    std::vector<Message> messages{matrix.extractMessages(data)};

    if (messages.size() > 0)
    {
      matrix.markRead(messages.back());
    }

    for (const auto& message : messages)
    {
      if (message.m_body == "hello bot")
      {
        matrix.sendMessage("YO WHAT UP CUZ");
      }
    }
  }
}
