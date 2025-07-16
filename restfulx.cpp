
#include "restfulx.hpp"

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_map>

#include <curl/curl.h>

const int RESTful_MAX_FAIL = 3;
std::unordered_map<int, int> port_fail_counts;
std::vector<int> RESTful_ports = {DefaultRESRfulPort};

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
    xlog("no ports provided, skip...");
    xlog("no ports provided, skip...");
    return;
  }

  if (content.empty()) {
    xlog("content is empty, skipping...");
    xlog("content is empty, skipping...");
    return;
  }

  // Use a copy of the ports because we may modify RESTful_ports while iterating
  std::vector<int> ports_copy = RESTful_ports;

  for (int port : ports_copy) {
  // Use a copy of the ports because we may modify RESTful_ports while iterating
  std::vector<int> ports_copy = RESTful_ports;

  for (int port : ports_copy) {
    if (port == 0) {
      xlog("port is zero, do nothing...");
      xlog("port is zero, do nothing...");
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

        port_fail_counts[port]++;
        if (port_fail_counts[port] >= RESTful_MAX_FAIL) {
          xlog("Port %d exceeded max fail count (%d), auto-unregistering...", port, RESTful_MAX_FAIL);
          RESTful_unRegister(std::to_string(port));
          port_fail_counts.erase(port);  // Clean up failure count
        }

      } else {
        // On success, reset failure count
        port_fail_counts[port] = 0;
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

void RESTful_send_streamingStatus_gst(bool isStreaming) {
  string url = "http://localhost";
  string content = "gst/isStreaming/" + (isStreaming ? "true" : "false");
  RESTFul_sendAsync(url, content);
}

void RESTful_send_streamingStatus_gige_hik(int index, bool isStreaming) {
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
