#pragma once

#include "global.hpp"

#define DefaultRESRfulPort 7654

// void RESTful_register(const std::string&  portS);
// void RESTful_unRegister(const std::string& portS);

// void RESTFul_send(const std::string& url, const std::string& content);
// void RESTFul_sendAsync(const std::string& url, const std::string& content);

void RESTful_register(const std::string& url, const std::string& portS);
void RESTful_unRegister(const std::string& url, const std::string& portS);

void RESTFul_send(const std::string& content);
void RESTFul_sendAsync(const std::string& content);

void RESTful_send_streamingStatus_gst(bool isStreaming);
void RESTful_send_streamingStatus_gige_hik(int index, bool isStreaming);

void RESTful_send_DI(int index, bool isLevelHigh);
void RESTful_send_DIODI(int index, bool isLevelHigh);


