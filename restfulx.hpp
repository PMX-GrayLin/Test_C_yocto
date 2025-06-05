#pragma once

#include "global.hpp"

#define defaultRESRfulPort 7654

void sendRESTFul(const std::string&  url, int port = defaultRESRfulPort);
void sendRESTFulAsync(const std::string& url, int port = defaultRESRfulPort);

void sendRESTful_streamingStatus(int index, bool isStreaming, int port = defaultRESRfulPort);
void sendRESTful_DI(int index, bool isLevelHigh, int port = defaultRESRfulPort);


