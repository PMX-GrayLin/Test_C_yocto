#pragma once

#include "global.hpp"

#include <vector>

// #define defaultRESRfulPort 7654

// void sendRESTFul(const std::string&  url, int port = defaultRESRfulPort);
// void sendRESTFulAsync(const std::string& url, int port = defaultRESRfulPort);

#define DefaultRESRfulPort 7654

void sendRESTFul(const std::string& url, const std::string& content);
void sendRESTFulAsync(const std::string& url, const std::string& content);

void sendRESTful_streamingStatus(int index, bool isStreaming);
void sendRESTful_DI(int index, bool isLevelHigh);
void sendRESTful_DIODI(int index, bool isLevelHigh);


