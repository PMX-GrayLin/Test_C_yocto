#include "global.hpp"

#include <cctype>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sys/stat.h>
#include <curl/curl.h>

// POSIX header
#include <libgen.h>

// test vars
int testCounter = 0;

bool isTimerRunning = false;
int counterTimer = 0;

void startTimer(int ms) {
  if (!isTimerRunning) {
    isTimerRunning = true;
    counterTimer = 0;

    std::thread([ms]() {
      xlog("timer start >>>>");

      while (isTimerRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));

      }
      xlog("timer stop >>>>");
    }).detach();  // Detach to run in the background

  } else {
    xlog("timer already running...");
  }
}

void stopTimer() {
    isTimerRunning = false;
}

void printBuffer(const uint8_t* buffer, size_t len) {
  printf("len:%ld: ", len);
  for (size_t i = 0; i < len; i++) {
    printf("0x%02X ", buffer[i]);
  }
  printf("\n\r");
}

bool isSameString(const char* s1, const char* s2, bool isCaseSensitive) {
  if (s1 == nullptr || s2 == nullptr) {
    return false;
  }

  if (isCaseSensitive) {
    return std::strcmp(s1, s2) == 0;
  } else {
    std::string str1(s1), str2(s2);
    if (str1.size() != str2.size()) {
      return false;
    }
    return std::equal(str1.begin(), str1.end(), str2.begin(), [](char a, char b) {
      return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
  }
}

bool isSameString(const std::string& s1, const std::string& s2, bool isCaseSensitive) {
  return isSameString(s1.c_str(), s2.c_str(), isCaseSensitive);;
}

bool isPathExist(const char* path) {
  struct stat buffer;
  return (stat(path, &buffer) == 0);
}

double limitValueInRange(double input, double rangeMin, double rangeMax) {
  if (input > rangeMax) {
    return rangeMax;
  } else if (input < rangeMin) {
    return rangeMin;
  } else {
    return input;
  }
}

std::string getTimeString() {
  std::time_t now = std::time(nullptr);

  setenv("TZ", "UTC-8", 1);   // Set timezone to UTC+8
  tzset();                    // Apply timezone setting

  std::tm* localTime = std::localtime(&now);

  std::ostringstream oss;
  oss << std::setw(2) << std::setfill('0') << localTime->tm_hour
      << std::setw(2) << std::setfill('0') << localTime->tm_min
      << std::setw(2) << std::setfill('0') << localTime->tm_sec;

  return oss.str();
}

std::string get_parent_directory(const std::string& path) {
    std::string temp = path;                 // Make a mutable copy
    char* path_buf = temp.data();            // Get modifiable char* from std::string
    std::string dir = dirname(path_buf);     // dirname modifies path_buf
    return dir;
}

std::string exec_command(const std::string& cmd) {
  xlog("cmd:%s", cmd.c_str());
  std::array<char, 128> buffer;
  std::string result;

  // Open pipe to file
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) {
    xlog("popen fail");
  }

  // Read till end of process:
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }

  xlog("result:%s", result.c_str());
  return result;
}

void sendRESTFul(const std::string& content, int port) {
  CURL* curl = curl_easy_init();
  if (curl) {
    std::string url = "http://localhost:" + std::to_string(port)  + "/fw/" + content;
    xlog("url:%s", url.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 100L);
    // Don't wait for the response body
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      xlog("curl_easy_perform() failed:%s", curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
  }
}

void sendRESTFulAsync(const std::string& content, int port = 7654) {
  std::thread([content, port]() {
    sendRESTFul(content, port);
  }).detach();  // Detach so it runs independently
}
