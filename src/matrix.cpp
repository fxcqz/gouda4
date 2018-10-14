#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
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
  std::string room = m_config["room"];
  room.replace(room.find('#'), 1, "%23");
  room.replace(room.find(':'), 1, "%3A");
  return room;
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

json Matrix::putOrPost(const std::string& method,
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

json Matrix::POST(const std::string& endpoint,
                  const json& data,
                  const param_t& params,
                  const std::string& version)
{
  return putOrPost(
    Poco::Net::HTTPRequest::HTTP_POST, endpoint, data, params, version
  );
}

json Matrix::PUT(const std::string& endpoint,
                 const json& data,
                 const param_t& params,
                 const std::string& version)
{
  return putOrPost(
    Poco::Net::HTTPRequest::HTTP_PUT, endpoint, data, params, version
  );
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

void Matrix::login()
{
  const json data = {
    {"type", "m.login.password"},
    {"user", getUsername()},
    {"password", getPassword()}
  };
  const json response = POST("login", data);
  if (response.empty())
  {
    throw std::runtime_error("Could not login to Matrix");
  }

  m_accessToken = response["access_token"];
  m_userID = response["user_id"];
}

void Matrix::join()
{
  const json response = POST("join/" + getRoom(), EMPTY_JSON);
  if (response.empty())
  {
    throw std::runtime_error("Could not join the room");
  }

  m_roomID = response["room_id"];
}

void Matrix::setMessageFilter()
{
  std::ostringstream data_string;
  data_string << R"({
    "account_data": {"types": ["m.room.meessage"]},
    "room": {"rooms": [")" << m_roomID << "\"]}}";
  const json data = json::parse(data_string.str());

  std::ostringstream url;
  url << "user/" << m_userID << "/filter";
  const json response = POST(url.str(), data);

  if (!response.empty())
  {
    m_filterID = response["filter_id"];
  }
}

json Matrix::sync()
{
  param_t params{{{"filter", m_filterID}}};
  if (m_nextBatch.length() > 0)
  {
    params.insert({"since", m_nextBatch});
  }

  const json response = GET("sync", params);

  if (!response.empty())
  {
    m_nextBatch = response["next_batch"];
  }

  return response;
}

std::vector<Message> Matrix::extractMessages(const json& data)
{
  std::vector<Message> result;
  try {
    const json roomData = data[0]["rooms"]["join"];
    if (roomData.find(m_roomID) != roomData.end())
    {
      const json events = roomData[m_roomID]["timeline"]["events"];
      for (const auto& event : events)
      {
        result.push_back({
          event["content"]["body"], event["sender"], event["event_id"]
        });
      }
    }
  }
  catch (...) {
    return {};
  }
  return result;
}

void Matrix::sendMessage(const std::string& message, const std::string& msgType)
{
  const json data = {
    {"body", message},
    {"msgtype", msgType}
  };
  std::ostringstream url;
  url << "rooms/" << m_roomID << "/send/m.room.message/" << m_txID;
  PUT(url.str(), data);
  m_txID += 1;
}
