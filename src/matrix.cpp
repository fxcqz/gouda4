#include <fstream>
#include <sstream>
#include <streambuf>

#include "matrix.h"

json Matrix::readConfig(const std::string& filename)
{
  std::string contents;
  std::ifstream handle {filename};

  handle.seekg(0, std::ios::end);
  contents.reserve(handle.tellg());
  handle.seekg(0, std::ios::beg);

  contents.assign(
    (std::istreambuf_iterator<char>(handle)),
    std::istreambuf_iterator<char>()
  );

  return json::parse(contents);
}

Matrix::Matrix(const std::string& filename)
{
  m_config = readConfig(filename);
}

// Getters for matrix config (from json), see file for details

std::string Matrix::getUsername() const
{
  return m_config["username"];
}

std::string Matrix::getPassword() const
{
  return m_config["password"];
}

std::string Matrix::getAddress() const
{
  return m_config["address"];
}

std::string Matrix::getRoom() const
{
  return m_config["room"];
}

// Utility methods

std::string makeParams(const param_t& params)
{
  if (params.empty())
  {
    return "";
  }
  std::ostringstream buf;
  for (auto const& [key, val] : params)
  {
    buf << key << '=' << val << '&';
  }
  std::string result = buf.str();
  // strip final &
  result.erase(result.length() -1, 1);

  return result;
}

std::string Matrix::buildUrl(const std::string& endpoint,
                             const param_t& params,
                             const std::string& version) const
{
  char concat = '?';
  std::ostringstream url;
  url << getAddress() << "/_matrix/client/" << version << "/" << endpoint;
  std::string paramString = makeParams(params);
  if (paramString.length())
  {
    url << '?' << makeParams(params);
    concat = '&';
  }
  if (m_accessToken.length())
  {
    url << concat << "access_token=" << m_accessToken;
  }
  return url.str();
}

json Matrix::POST(const std::string& endpoint,
                  const json& data,
                  const param_t& params,
                  const std::string& version)
{
}
