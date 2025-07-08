#pragma once

#include "global.hpp"

void UVC_handle_RESTful(std::vector<std::string> segments);

void UVC_setDrvicePath(const string& devicePath);
void UVC_setImagePath(const string& imagePath);
void UVC_captureImage();

// Streaming
void Thread_UVCStreaming();
void UVC_streamingStart();
void UVC_streamingStop();
void UVC_streamingLED();
