#include <iostream>

#include "matrix.h"

int main (int argc, char *argv[])
{
  Matrix m("config.json");
  m.login();
  m.join();
  m.setMessageFilter();
  m.sync();
}
