#pragma once
/*********************************************************************//**
 * \file   VideoPlayer.h
 *
 * \brief
 *      Windows Media Foundation (WMF) based MP4/video player.
 *      Decodes video frames and uploads them as OpenGL textures.
 *********************************************************************/

#ifdef _WIN32

#include <string>
#include <vector>
#include <functional>
#include <GL/glew.h>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <d3d9.h>
#include <dxva2api.h>

#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "dxguid.lib")

namespace Framework {

    class VideoPlayer {
    public:
        enum class State { Stopped, Playing, Paused, Finished };

        VideoPlayer();
        ~VideoPlayer();

        VideoPlayer(const VideoPlayer&) = delete;
        VideoPlayer& operator=(const VideoPlayer&) = delete;

        // ---- Main API ----
        bool Open(const std::string& path);
        void Play();
        void Pause();
        void Stop();
        void Close();
        void Update(float dt);

        // ---- Queries ----
        bool   IsReady()    const { return m_ready && m_texture != 0; }
        bool   IsPlaying()  const { return m_state == State::Playing; }
        bool   IsPaused()   const { return m_state == State::Paused; }
        bool   IsFinished() const { return m_state == State::Finished; }
        State  GetState()   const { return m_state; }
        GLuint GetTexture() const { return m_texture; }
        int    GetWidth()   const { return m_width; }
        int    GetHeight()  const { return m_height; }
        float  GetDuration()const { return m_duration; }
        float  GetPosition()const { return m_clock; }
        float  GetProgress()const {
            return (m_duration > 0.0f) ? (m_clock / m_duration) : 0.0f;
        }

        // ---- Options ----
        void SetLooping(bool loop) { m_looping = loop; }
        void SetOnFinished(std::function<void()> cb) { m_onFinished = std::move(cb); }

    private:
        bool InitWMF();
        void ShutdownWMF();
        bool ConfigureReader(const std::wstring& path);
        bool DecodeNextFrame();
        void UploadTexture();
        void CreateGLTexture();
        void DestroyGLTexture();
        void SeekToStart();
        //bool CreateD3D9Device();

        // State
        State  m_state = State::Stopped;
        bool   m_ready = false;
        bool   m_looping = false;
        bool   m_wmfInit = false;
        bool   m_firstFrameDecoded = false;

        // Timing
        float    m_clock = 0.0f;
        float    m_duration = 0.0f;
        float    m_nextFramePts = 0.0f;
        LONGLONG m_lastSampleTime = 0;

        // Video properties
        int    m_width = 0;
        int    m_height = 0;
        UINT32 m_stride = 0;

        // WMF objects
        IMFSourceReader* m_reader = nullptr;
        IMFMediaBuffer* m_currentBuffer = nullptr;

        // CPU pixel buffer (RGBA8)
        std::vector<BYTE> m_rgbaBuffer;
        bool m_frameUpdated = false;

        // OpenGL
        GLuint m_texture = 0;

        // Callback
        std::function<void()> m_onFinished;
    };

} // namespace Framework

#endif // _WIN32