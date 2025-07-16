
#include "restfulx.hpp"

#include <curl/curl.h>

// void sendRESTFul(const std::string& url, int port) {
//   CURL* curl = curl_easy_init();
//   if (curl) {
//     xlog("url:%s", url.c_str());
//     curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//     curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 100L);

//     // Send as GET (default) instead of HEAD
//     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void*, size_t size, size_t nmemb, void*) -> size_t {
//       return size * nmemb;  // ignore response body
//     });

//     CURLcode res = curl_easy_perform(curl);
//     if (res != CURLE_OK) {
//       xlog("curl_easy_perform failed:%s", curl_easy_strerror(res));
//     }
//     curl_easy_cleanup(curl);
//   }
// }

// void sendRESTFulAsync(const std::string& url, int port) {
//   std::thread([url, port]() {
//     sendRESTFul(url, port);
//   }).detach();  // Detach so it runs independently
// }

std::vector<int> RESTful_ports = { 0 };

void sendRESTFul(const std::string& url, const std::string& content) {
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

void sendRESTFulAsync(const std::string& url, const std::string& content) {
  std::thread([url, content]() {
    sendRESTFul(url, content);
  }).detach();
}

void sendRESTful_streamingStatus(int index, bool isStreaming) {
  string url = "http://localhost";
  string content = "gige" + std::to_string(index + 1) + "/isStreaming/" + (isStreaming ? "true" : "false");
  sendRESTFulAsync(url, content);
}

void sendRESTful_DI(int index, bool isLevelHigh) {
  string url = "http://localhost";
  string content = "di/" + std::to_string(index + 1) + "/status/" + (isLevelHigh ? "high" : "low");
  sendRESTFulAsync(url, content);
}

void sendRESTful_DIODI(int index, bool isLevelHigh) {
  string url = "http://localhost";
  string content = "dio_di/" + std::to_string(index + 1) + "/status/" + (isLevelHigh ? "high" : "low");
  sendRESTFulAsync(url, content);
}
