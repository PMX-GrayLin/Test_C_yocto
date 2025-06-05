
#include "restfulx.hpp"

#include <curl/curl.h>

void sendRESTFul(const std::string& url, int port) {
  CURL* curl = curl_easy_init();
  if (curl) {
    xlog("url:%s", url.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 100L);
    
    // Don't wait for the response body
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      xlog("curl_easy_perform failed:%s", curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
  }
}

void sendRESTFulAsync(const std::string& url, int port) {
  std::thread([url, port]() {
    sendRESTFul(url, port);
  }).detach();  // Detach so it runs independently
}

void sendRESTful_streamingStatus(int index, bool isStreaming, int port) {
  string content_fixed = "http://localhost:" + std::to_string(port) + "/fw/";
  string content_rest = "gige" + std::to_string(index + 1) + "/isStreaming/" + (isStreaming ? "true" : "false");
  string url = content_fixed + content_rest;
  sendRESTFulAsync(url);
}

void sendRESTful_DI(int index, bool isLevelHigh, int port) {
  string content_fixed = "http://localhost:" + std::to_string(port) + "/fw/";
  string content_rest = "di/" + std::to_string(index + 1) + "/status/" + (isLevelHigh ? "high" : "low");
  string url = content_fixed + content_rest;
  sendRESTFulAsync(url);
}


