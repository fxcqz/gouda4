#ifndef MATRIX_H
#define MATRIX_H

#include <map>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPSClientSession.h>

using json = nlohmann::json;
using param_t = std::map<std::string, std::string>;

const json EMPTY_JSON = "{}"_json;
const param_t NO_PARAMS;

std::string makeParams(const param_t& params);

class Matrix {
private:
  json m_config;
  std::unique_ptr<Poco::Net::HTTPSClientSession> m_conn;
  std::string m_userID;
  std::string m_roomID;
  std::string m_accessToken;
  std::string m_nextBatch;
  std::string m_filterID;
  int m_txID = 0;

  json readConfig(const std::string& filename);
  std::string buildUrl(const std::string& endpoint,
                       const param_t& params,
                       const std::string& version) const;
  json getResponse();
  json putOrPost(const std::string& method,
                 const std::string& endpoint,
                 const json& data,
                 const param_t& params = NO_PARAMS,
                 const std::string& version = "unstable");
  json POST(const std::string& endpoint,
            const json& data,
            const param_t& params = NO_PARAMS,
            const std::string& version = "unstable");
  json PUT(const std::string& endpoint,
           const json& data,
           const param_t& params = NO_PARAMS,
           const std::string& version = "unstable");
  json GET(const std::string& endpoint,
           const param_t& params = NO_PARAMS,
           const std::string& version = "unstable");
public:
  explicit Matrix(const std::string& filename);

  std::string getUsername() const;
  std::string getPassword() const;
  std::string getAddress() const;
  std::string getRoom() const;

  void login();
};

#endif
