#pragma once

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// opencv
#pragma warning(disable : 4819)
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#ifdef _DEBUG
#pragma comment(lib, "opencv_world300d.lib")
#else
#pragma comment(lib, "opencv_world300.lib")
#endif


// oscpack
#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"

#ifdef _DEBUG
#pragma comment(lib, "liboscpack_vc12x86D.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#else
#pragma comment(lib, "liboscpack_vc12x86.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#endif
