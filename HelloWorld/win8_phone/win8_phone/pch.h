#pragma once
#ifndef CC_WIN8_PHONE
#pragma region Application Family
#include <wrl.h>
#include <d3d11_1.h>
#include <agile.h>
#include <DirectXMath.h>
#include <memory>

#define XAUDIO2_HELPER_FUNCTIONS 1
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <mmreg.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mfreadwrite.h>
#include <mfmediaengine.h>

#else
#include "targetver.h"

#include <D3D11_1.h>
#include <wrl/client.h>

#define XAUDIO2_HELPER_FUNCTIONS 1
#include <xaudio2.h>
#include <xaudio2fx.h>
#endif