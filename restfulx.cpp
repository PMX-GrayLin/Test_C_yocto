
#include "restfulx.hpp"

#include <vector>
#include <algorithm>

#include <curl/curl.h>

std::vector<int> RESTful_ports = {};

void RESTful_register(const std::string& portS) {

  int port = std::stoi(portS);
  if (port <= 0) return;  // Ignore invalid ports

  // Check if port already exists
  if (std::find(RESTful_ports.begin(), RESTful_ports.end(), port) == RESTful_ports.end()) {
    RESTful_ports.push_back(port);
    xlog("Registered port: %d", port);
  } else {
    xlog("Port %d already registered", port);
  }
}

void RESTful_unRegister(const std::string& portS) {

  int port = std::stoi(portS);
  auto it = std::find(RESTful_ports.begin(), RESTful_ports.end(), port);
  if (it != RESTful_ports.end()) {
    RESTful_ports.erase(it);
    xlog("Unregistered port: %d", port);
  } else {
    xlog("Port %d not found, cannot unregister", port);
  }
}

void RESTFul_send(const std::string& url, const std::string& content) {
  if (RESTful_ports.empty()) {
    xlog("no ports provided, skipping RESTful request.");
    return;
  }

  if (content.empty()) {
    xlog("content is empty, skipping request.");
    return;
  }

  for (int port : RESTful_ports) {
    if (port == 0) {
      xlog("port is zero, do nothing");
      continue;
    }

    CURL* curl = curl_easy_init();
    if (curl) {
      std::string full_url = url + ":" + std::to_string(port);
      if (!content.empty()) {
        if (content.front() != '/')
          full_url += "/";
        full_url += content;
      }
      xlog("send to url: %s", full_url.c_str());

      curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
      curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 100L);

      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void*, size_t size, size_t nmemb, void*) -> size_t {
        return size * nmemb;
      });

      CURLcode res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        xlog("curl_easy_perform failed: %s", curl_easy_strerror(res));
      }

      curl_easy_cleanup(curl);
    }
  }
}

void RESTFul_sendAsync(const std::string& url, const std::string& content) {
  std::thread([url, content]() {
    RESTFul_send(url, content);
  }).detach();
}

void RESTful_send_streamingStatus(int index, bool isStreaming) {
  string url = "http://localhost";
  string content = "gige" + std::to_string(index + 1) + "/isStreaming/" + (isStreaming ? "true" : "false");
  RESTFul_sendAsync(url, content);
}

void RESTful_send_DI(int index, bool isLevelHigh) {
  string url = "http://localhost";
  string content = "di/" + std::to_string(index + 1) + "/status/" + (isLevelHigh ? "high" : "low");
  RESTFul_sendAsync(url, content);
}

void RESTful_send_DIODI(int index, bool isLevelHigh) {
  string url = "http://localhost";
  string content = "dio_di/" + std::to_string(index + 1) + "/status/" + (isLevelHigh ? "high" : "low");
  RESTFul_sendAsync(url, content);
}
