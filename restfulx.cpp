
#include "restfulx.hpp"

// #include <vector>
#include <algorithm>
#include <unordered_map>
#include <set>
#include <utility> 

#include <curl/curl.h>

struct PairHash {
  std::size_t operator()(const std::pair<std::string, int>& p) const {
    return std::hash<std::string>()(p.first) ^ (std::hash<int>()(p.second) << 1);
  }
};

const int RESTful_MAX_FAIL = 10;
std::set<std::pair<std::string, int>> RESTful_targets;
std::unordered_map<std::pair<std::string, int>, int, PairHash> RESTful_failCount;

bool isTestRESTful = true;


void RESTful_register(const std::string& url, const std::string& portS) {
  int port = std::stoi(portS);
  if (port <= 0) return;

  // Ensure the URL starts with "http://"
  std::string urlx = url;
  if (urlx.find("http://") != 0 && urlx.find("https://") != 0) {
    urlx = "http://" + urlx;
  }

  auto key = std::make_pair(urlx, port);
  if (RESTful_targets.find(key) == RESTful_targets.end()) {
    RESTful_targets.insert(key);
    xlog("Registered: %s:%d", urlx.c_str(), port);
    isTestRESTful = false;
  } else {
    xlog("Already registered: %s:%d", urlx.c_str(), port);
  }
}

void RESTful_unRegister(const std::string& url, const std::string& portS) {
  int port = std::stoi(portS);

  // Ensure the URL starts with "http://"
  std::string urlx = url;
  if (urlx.find("http://") != 0 && urlx.find("https://") != 0) {
    urlx = "http://" + urlx;
  }

  auto key = std::make_pair(urlx, port);
  auto it = RESTful_targets.find(key);
  if (it != RESTful_targets.end()) {
    RESTful_targets.erase(it);
    RESTful_failCount.erase(key);  // Clean up fail count
    xlog("Unregistered: %s:%d", urlx.c_str(), port);
  } else {
    xlog("Not found: %s:%d", urlx.c_str(), port);
  }
}

void RESTFul_send(const std::string& content) {

  if (isTestRESTful) {
    xlog("content:%s", content.c_str());
    return;
  }

  if (RESTful_targets.empty()) {
    xlog("No targets to send, skipping...");
    return;
  }

  if (content.empty()) {
    xlog("Content is empty, skipping...");
    return;
  }

  std::set<std::pair<std::string, int>> targets_copy = RESTful_targets;

  for (const auto& [url, port] : targets_copy) {
    if (port == 0) {
      xlog("Port is zero, skipping...");
      continue;
    }

    CURL* curl = curl_easy_init();
    if (curl) {
      std::string full_url = url + ":" + std::to_string(port);
      if (content.front() != '/')
        full_url += "/";
      full_url += content;

      xlog("Sending to: %s", full_url.c_str());

      curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
      curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 100L);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void*, size_t size, size_t nmemb, void*) -> size_t {
        return size * nmemb;
      });

      CURLcode res = curl_easy_perform(curl);
      auto key = std::make_pair(url, port);
      if (res != CURLE_OK) {
        xlog("curl_easy_perform failed: %s", curl_easy_strerror(res));

        RESTful_failCount[key]++;
        if (RESTful_failCount[key] >= RESTful_MAX_FAIL) {
          xlog("Exceeded max fail count for %s:%d. Auto-unregistering.", url.c_str(), port);
          RESTful_targets.erase(key);
          RESTful_failCount.erase(key);
        }
      } else {
        RESTful_failCount[key] = 0;
      }

      curl_easy_cleanup(curl);
    }
  }
}

void RESTFul_sendAsync(const std::string& content) {
  std::thread([content]() {
    RESTFul_send(content);
  }).detach();
}

void RESTful_send_streamingStatus_gst(bool isStreaming) {
  string content = std::string("cis/isStreaming/") + (isStreaming ? "true" : "false");
  RESTFul_sendAsync(content);
}

void RESTful_send_streamingStatus_gige_hik(int index, bool isStreaming) {
  string content = "gige" + std::to_string(index + 1) + "/isStreaming/" + (isStreaming ? "true" : "false");
  RESTFul_sendAsync(content);
}

void RESTful_send_streamingStatus_uvc(bool isStreaming) {
  string content = std::string("uvc/isStreaming/") + (isStreaming ? "true" : "false");
  RESTFul_sendAsync(content);
}

void RESTful_send_DI(int index, bool isLevelHigh) {
  string content = "di/" + std::to_string(index + 1) + "/status/" + (isLevelHigh ? "high" : "low");
  RESTFul_sendAsync(content);
}

void RESTful_send_Trigger(int index, bool isLevelHigh) {
  string content = "trigger/" + std::to_string(index + 1) + "/status/" + (isLevelHigh ? "high" : "low");
  RESTFul_sendAsync(content);
}

void RESTful_send_DIODI(int index, bool isLevelHigh) {
  string content = "dio_di/" + std::to_string(index + 1) + "/status/" + (isLevelHigh ? "high" : "low");
  RESTFul_sendAsync(content);
}

void RESTful_send_triggerMode_gige_hik(int index, bool triggerMode) {
  string content = "gige" + std::to_string(index + 1) + "/isTriggerMode/" + (triggerMode ? "true" : "false");
  RESTFul_sendAsync(content);
}
