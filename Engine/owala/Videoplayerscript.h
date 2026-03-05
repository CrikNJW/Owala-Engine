#pragma once
/*********************************************************************//**
 * \file   VideoPlayerScript.h
 *
 * \brief
 *      NativeScript wrapper around VideoPlayer.
 *      Attach to any entity to play a video as a full-screen cutscene.
 *
 *      Setup:
 *        1. Create an empty entity in your scene
 *        2. Add NativeScriptsComponent ? attach VideoPlayerScript
 *        3. Set videoPath and options below
 *        4. That's it — no other components needed
 *********************************************************************/
#include "ScriptableEntity.h"
#include "VideoPlayer.h"

using namespace Framework;

class VideoPlayerScript : public ScriptableEntity {
protected:
    void OnCreate()  override;
    void OnUpdate()  override;
    void OnDestroy() override;

public:
    // ---- Configure these ----
    std::string videoPath = "Assets/Sprites/Cutscenes/Testvideo.mp4";
    bool        autoPlay = true;   // start immediately on scene load
    bool        loop = false;  // loop when finished

    // External control
    void Play() { m_player.Play(); }
    void Pause() { m_player.Pause(); }
    void Stop() { m_player.Stop(); }

    bool IsFinished() const { return m_player.IsFinished(); }
    bool IsPlaying()  const { return m_player.IsPlaying(); }

private:
    VideoPlayer m_player;
    void DrawFullscreen();
};