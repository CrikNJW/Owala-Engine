#include "pch.h"

#ifdef _WIN32

/*********************************************************************//**
 * \file   VideoPlayer.cpp
 *
 * \brief
 *      WMF MP4 decoder with improved error handling and format support.
 *********************************************************************/
#include "VideoPlayer.h"
#include "Logger.h"
#include <fstream>
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

namespace Framework {

    static constexpr LONGLONG HNS_PER_SECOND = 10'000'000LL;

    // -----------------------------------------------------------------------
    VideoPlayer::VideoPlayer() = default;

    VideoPlayer::~VideoPlayer() {
        Close();
    }

    bool VideoPlayer::Open(const std::string& path) {
        Close();

        if (path.empty()) {
            Logger::Get().Error(DebugType::GameManager) << "[VideoPlayer] Path is empty!" << std::endl;
            return false;
        }

        // 1. Resolve to an absolute path. WMF is sensitive to relative paths.
        char absPath[MAX_PATH];
        if (GetFullPathNameA(path.c_str(), MAX_PATH, absPath, nullptr) == 0) {
            Logger::Get().Error(DebugType::GameManager)
                << "[VideoPlayer] Failed to resolve absolute path for: " << path << std::endl;
            return false;
        }

        std::string fullPath = absPath;

        // 2. Normalize Slashes: Convert '/' to '\' for Windows API consistency
        for (auto& c : fullPath) {
            if (c == '/') c = '\\';
        }

        // 3. Physical File Check: Verify the file actually exists
        struct stat buffer;
        if (stat(fullPath.c_str(), &buffer) != 0) {
            Logger::Get().Error(DebugType::GameManager)
                << "[VideoPlayer] PHYSICAL FILE MISSING at: " << fullPath << std::endl;
            return false;
        }

        Logger::Get().Info(DebugType::GameManager)
            << "[VideoPlayer] Attempting to open: " << fullPath << std::endl;

        // 4. Initialize WMF
        if (!InitWMF()) {
            return false;
        }

        // 5. Convert to Wide String (UTF-16)
        int len = MultiByteToWideChar(CP_UTF8, 0, fullPath.c_str(), -1, nullptr, 0);
        if (len <= 0) return false;

        std::wstring wpath(len, 0);
        MultiByteToWideChar(CP_UTF8, 0, fullPath.c_str(), -1, &wpath[0], len);

        // 6. Configure the Reader (Metadata and Stream Setup)
        if (!ConfigureReader(wpath)) {
            Logger::Get().Error(DebugType::GameManager)
                << "[VideoPlayer] WMF Reader failed for: " << fullPath << std::endl;
            ShutdownWMF();
            return false;
        }

        // 7. CRITICAL: Create the OpenGL texture now that m_width and m_height are known
        CreateGLTexture();

        // Reset playback state
        m_clock = 0.0f;
        m_nextFramePts = 0.0f;
        m_state = State::Stopped;
        m_firstFrameDecoded = false;

        Logger::Get().Info(DebugType::GameManager)
            << "[VideoPlayer] Successfully opened: " << m_width << "x" << m_height
            << " Duration: " << m_duration << "s" << std::endl;

        return true;
    }

    // -----------------------------------------------------------------------
    bool VideoPlayer::InitWMF() {
        if (m_wmfInit) return true;

        HRESULT hr = MFStartup(MF_VERSION);
        if (FAILED(hr)) {
            Logger::Get().Error(DebugType::GameManager)
                << "[VideoPlayer] MFStartup failed: 0x" << std::hex << hr << std::dec << std::endl;
            return false;
        }

        m_wmfInit = true;
        return true;
    }

    void VideoPlayer::ShutdownWMF() {
        if (m_currentBuffer) {
            m_currentBuffer->Release();
            m_currentBuffer = nullptr;
        }

        if (m_reader) {
            m_reader->Release();
            m_reader = nullptr;
        }

        if (m_wmfInit) {
            MFShutdown();
            m_wmfInit = false;
        }
    }

    // -----------------------------------------------------------------------
    bool VideoPlayer::ConfigureReader(const std::wstring& path) {
        if (m_reader) {
            m_reader->Release();
            m_reader = nullptr;
        }

        IMFAttributes* pAttributes = nullptr;
        HRESULT hr = MFCreateAttributes(&pAttributes, 2);

        if (SUCCEEDED(hr)) {
            pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
            pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
        }

        hr = MFCreateSourceReaderFromURL(path.c_str(), pAttributes, &m_reader);
        if (pAttributes) pAttributes->Release();

        if (FAILED(hr)) {
            Logger::Get().Error(DebugType::GameManager)
                << "[VideoPlayer] MFCreateSourceReaderFromURL failed: 0x" << std::hex << hr << std::dec << std::endl;
            return false;
        }

        // Select Video Stream
        m_reader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
        m_reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);

        // Setup RGB32 Output
        IMFMediaType* pType = nullptr;
        hr = MFCreateMediaType(&pType);
        if (SUCCEEDED(hr)) {
            pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
            hr = m_reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pType);
            pType->Release();
        }

        if (FAILED(hr)) return false;

        // Get Dimensions
        IMFMediaType* pCurrentType = nullptr;
        hr = m_reader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrentType);
        if (SUCCEEDED(hr)) {
            UINT32 w, h;
            MFGetAttributeSize(pCurrentType, MF_MT_FRAME_SIZE, &w, &h);
            m_width = static_cast<int>(w);
            m_height = static_cast<int>(h);

            LONG stride;
            if (SUCCEEDED(MFGetStrideForBitmapInfoHeader(MFVideoFormat_RGB32.Data1, w, &stride))) {
                m_stride = static_cast<UINT32>(abs(stride));
            }
            else {
                m_stride = w * 4;
            }
            pCurrentType->Release();
        }

        // Get Duration - FIXED
        PROPVARIANT var;
        PropVariantInit(&var); // Initialize to ensure it's not garbage
        hr = m_reader->GetPresentationAttribute((DWORD)MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
        if (SUCCEEDED(hr) && var.vt == VT_UI8) {
            // Convert 100-nanosecond units to seconds (divide by 10^7)
            m_duration = static_cast<float>(var.uhVal.QuadPart) / 10000000.0f;
        }
        else {
            m_duration = 0.0f;
        }
        PropVariantClear(&var); // Clear to free internal memory

        m_ready = true;
        return true;
    }

    // -----------------------------------------------------------------------
    bool VideoPlayer::DecodeNextFrame() {
        if (!m_reader) return false;

        DWORD streamIndex, flags;
        LONGLONG sampleTime;
        IMFSample* sample = nullptr;

        HRESULT hr = m_reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &streamIndex, &flags, &sampleTime, &sample);
        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
            if (sample) sample->Release();
            return false;
        }

        if (!sample) return true; // Keep trying next frame

        m_nextFramePts = static_cast<float>(sampleTime) / HNS_PER_SECOND;

        IMFMediaBuffer* buffer = nullptr;
        hr = sample->ConvertToContiguousBuffer(&buffer);

        if (SUCCEEDED(hr)) {
            BYTE* data = nullptr;
            DWORD curLen = 0;
            hr = buffer->Lock(&data, nullptr, &curLen);

            if (SUCCEEDED(hr) && data) {
                // Ensure buffer size matches expected RGBA output
                size_t expectedSize = static_cast<size_t>(m_width) * m_height * 4;
                if (m_rgbaBuffer.size() != expectedSize) m_rgbaBuffer.resize(expectedSize);

                // BGRA to RGBA conversion with Stride handling
                for (int y = 0; y < m_height; ++y) {
                    // WMF often returns frames upside down (negative stride)
                    const BYTE* srcRow = data + (y * m_stride);
                    BYTE* dstRow = m_rgbaBuffer.data() + ((m_height - 1 - y) * m_width * 4);

                    for (int x = 0; x < m_width; ++x) {
                        dstRow[x * 4 + 0] = srcRow[x * 4 + 2]; // R
                        dstRow[x * 4 + 1] = srcRow[x * 4 + 1]; // G
                        dstRow[x * 4 + 2] = srcRow[x * 4 + 0]; // B
                        dstRow[x * 4 + 3] = 255;            // A
                    }
                }
                m_frameUpdated = true;
                UploadTexture();

                if (!m_firstFrameDecoded) {
                    m_firstFrameDecoded = true;
                    m_ready = true;
                }
            }
            buffer->Unlock();
            buffer->Release();
        }
        sample->Release();
        return true;
    }

    // -----------------------------------------------------------------------
    void VideoPlayer::SeekToStart() {
        if (!m_reader) return;

        PROPVARIANT var;
        PropVariantInit(&var);
        var.vt = VT_I8;
        var.hVal.QuadPart = 0;
        m_reader->SetCurrentPosition(GUID_NULL, var);
        PropVariantClear(&var);

        m_clock = 0.0f;
        m_nextFramePts = 0.0f;
        m_lastSampleTime = 0;
        m_firstFrameDecoded = false;
    }

    // -----------------------------------------------------------------------
    void VideoPlayer::Play() {
        Logger::Get().Info(DebugType::GameManager)
            << "[VideoPlayer] Play called" << std::endl;

        if (m_state == State::Finished) SeekToStart();
        m_state = State::Playing;

        // Decode first frame if needed
        if (!m_firstFrameDecoded) {
            DecodeNextFrame();
        }
    }

    void VideoPlayer::Pause() {
        if (m_state == State::Playing) {
            m_state = State::Paused;
            Logger::Get().Info(DebugType::GameManager)
                << "[VideoPlayer] Paused" << std::endl;
        }
    }

    void VideoPlayer::Stop() {
        SeekToStart();
        m_state = State::Stopped;
        Logger::Get().Info(DebugType::GameManager)
            << "[VideoPlayer] Stopped" << std::endl;
    }

    void VideoPlayer::Close() {
        Logger::Get().Info(DebugType::GameManager)
            << "[VideoPlayer] Closing" << std::endl;

        m_state = State::Stopped;
        m_ready = false;
        m_firstFrameDecoded = false;
        DestroyGLTexture();
        ShutdownWMF();
        m_width = 0;
        m_height = 0;
        m_clock = 0.0f;
        m_duration = 0.0f;
        m_nextFramePts = 0.0f;
        m_rgbaBuffer.clear();
        m_frameUpdated = false;
    }

    // -----------------------------------------------------------------------
    void VideoPlayer::Update(float dt) {
        if (m_state != State::Playing) return;
        if (!m_ready) {
            // Try to decode first frame if not ready
            if (!m_firstFrameDecoded) {
                DecodeNextFrame();
            }
            return;
        }

        m_clock += dt;

        // Decode frames until we're caught up
        while (m_clock >= m_nextFramePts) {
            if (!DecodeNextFrame()) {
                if (m_looping) {
                    SeekToStart();
                    // Decode first frame of loop
                    DecodeNextFrame();
                }
                else {
                    m_state = State::Finished;
                    if (m_onFinished) {
                        m_onFinished();
                    }
                    return;
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    void VideoPlayer::UploadTexture() {
        if (!m_frameUpdated) return;
        if (m_texture == 0) return;
        if (m_rgbaBuffer.empty()) return;

        glBindTexture(GL_TEXTURE_2D, m_texture);

        // Update texture
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            m_width, m_height,
            GL_RGBA, GL_UNSIGNED_BYTE,
            m_rgbaBuffer.data());

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            Logger::Get().Error(DebugType::GameManager)
                << "[VideoPlayer] OpenGL error in UploadTexture: " << err << std::endl;
        }
        else {
            static int frameCount = 0;
            frameCount++;
            if (frameCount % 30 == 0) {  // Log every 30 frames
                Logger::Get().Info(DebugType::GameManager)
                    << "[VideoPlayer] Texture uploaded successfully - Frame: " << frameCount << std::endl;
            }
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        m_frameUpdated = false;
    }

    void VideoPlayer::CreateGLTexture() {
        if (m_width == 0 || m_height == 0) {
            Logger::Get().Error(DebugType::GameManager)
                << "[VideoPlayer] Invalid dimensions for texture creation" << std::endl;
            return;
        }

        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);

        // Create empty texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
            m_width, m_height, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            Logger::Get().Error(DebugType::GameManager)
                << "[VideoPlayer] OpenGL error in CreateGLTexture: " << err << std::endl;
            m_texture = 0;
        }
        else {
            Logger::Get().Info(DebugType::GameManager)
                << "[VideoPlayer] OpenGL texture created: " << m_width << "x" << m_height << std::endl;
        }
    }

    void VideoPlayer::DestroyGLTexture() {
        if (m_texture != 0) {
            glDeleteTextures(1, &m_texture);
            m_texture = 0;
        }
    }

} // namespace Framework

#endif // _WIN32