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
    for (const auto& message : messages)
    {
      std::cout << message.m_body << '\n';
    }
  }
}
