//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once
#include "pch.h"
#ifndef CC_WIN8_PHONE
class MediaStreamer
{
private:
    WAVEFORMATEX                        m_waveFormat;
    uint32                              m_maxStreamLengthInBytes;
    Windows::Storage::StorageFolder^    m_installedLocation;
    Platform::String^                   m_installedLocationPath;

public:
    Microsoft::WRL::ComPtr<IMFSourceReader> m_reader;
    Microsoft::WRL::ComPtr<IMFMediaType> m_audioType;

public:
    MediaStreamer();
    ~MediaStreamer();

    WAVEFORMATEX& GetOutputWaveFormatEx()
    {
        return m_waveFormat;
    }

    UINT32 GetMaxStreamLengthInBytes()
    {
        return m_maxStreamLengthInBytes;
    }

    void Initialize(_In_ const WCHAR* url); 
    bool GetNextBuffer(uint8* buffer, uint32 maxBufferSize, uint32* bufferLength);
    void ReadAll(uint8* buffer, uint32 maxBufferSize, uint32* bufferLength); 
    void Restart();
};
#else
#include <vector>

ref class MediaStreamer
{
private:
    WAVEFORMATEX      m_waveFormat;
    uint32            m_maxStreamLengthInBytes;
    std::vector<byte> m_data;
    UINT32            m_offset;
	Platform::Array<byte>^ ReadData(
    _In_ Platform::String^ filename
    );
internal:
    Windows::Storage::StorageFolder^ m_location;
    Platform::String^ m_locationPath;

public:
    virtual ~MediaStreamer();

internal:
    MediaStreamer();

    WAVEFORMATEX& GetOutputWaveFormatEx()
    {
        return m_waveFormat;
    }

    UINT32 GetMaxStreamLengthInBytes()
    {
		return m_data.size();
    }

    void Initialize(_In_ const WCHAR* url); 
    void ReadAll(uint8* buffer, uint32 maxBufferSize, uint32* bufferLength); 
    void Restart();
};
#endif