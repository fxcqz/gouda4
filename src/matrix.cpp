#include <fstream>
#include <sstream>
#include <streambuf>

#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/Exception.h>

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
  : m_config(readConfig(filename))
  , m_conn(std::make_unique<Poco::Net::HTTPSClientSession>(
    getAddress(), 443,
    new Poco::Net::Context(
      Poco::Net::Context::TLSV1_2_CLIENT_USE,
      "",
      Poco::Net::Context::VERIFY_NONE,
      9, true
  )))
{}

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
  url << "/_matrix/client/" << version << "/" << endpoint;
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

json Matrix::getResponse()
{
  Poco::Net::HTTPResponse response;
  try {
    std::istream& is = m_conn->receiveResponse(response);
    std::stringstream ss;
    Poco::StreamCopier::copyStream(is, ss);

    return json::parse(ss.str());
  }
  catch (Poco::Exception& ex) {
    return EMPTY_JSON;
  }
}

json Matrix::PUT_OR_POST(const std::string& method,
                         const std::string& endpoint,
                         const json& data,
                         const param_t& params,
                         const std::string& version)
{
  const std::string url = buildUrl(endpoint, params, version);

  Poco::Net::HTTPRequest request{method, url};
  request.setContentType("application/json");

  const std::string body = data.dump();
  request.setContentLength(body.length());

  std::ostream& os = m_conn->sendRequest(request);
  os << body;

  return getResponse();
}

json Matrix::GET(const std::string& endpoint,
                 const param_t& params,
                 const std::string& version)
{
  const std::string url = buildUrl(endpoint, params, version);
  Poco::Net::HTTPRequest request{Poco::Net::HTTPRequest::HTTP_GET, url};
  m_conn->sendRequest(request);
  return getResponse();
}
