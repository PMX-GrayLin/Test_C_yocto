#pragma once

#include "global.hpp"

#include <vector>

void UVC_handle_RESTful(std::vector<std::string> segments);

void UVC_setDevicePath(const string& devicePath);
void UVC_setImagePath(const string& imagePath);
void UVC_captureImage();

// Streaming
void Thread_UVCStreaming();
void UVC_streamingStart();
void UVC_streamingStop();
void UVC_streamingLED();
void UVC_setResolution(const string& resolutionS);
