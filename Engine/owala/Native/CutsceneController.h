/*****************************************************************//**
 * \file	CutsceneController.h
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author

 *
 * \brief	Contains declarations for the Cutscene Controller.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "ScriptableEntity.h"
#include "VideoPlayerScript.h" // Add this

using namespace Framework;

class CutsceneController : public ScriptableEntity {
protected:
    void OnCreate() override;
    void OnUpdate() override;
    void OnDestroy() override;

private:
    enum class SlideType { Image, Video };

    struct Slide {
        std::string source;     // texture key for images, file path for videos
        float duration;         // for images only (ignored for video)
        SlideType type;
    };

    std::vector<Slide> slides;
    int currentSlide = 0;
    float timer = 0.f;
    bool isEnding = false;

    // Skip functionality
    float skipHoldTime = 0.f;
    bool wasSkipDown = false;
    bool skipTriggered = false;

    // Video player (only used when current slide is video)
    VideoPlayerScript* videoPlayer = nullptr;

    void ShowImageEntity(bool show);
    void ApplySlide();
    bool NextSlide();
    void FinishCutscene();
    void FitImageToScreen();
    bool IsSkipHeldDown() const;
    void CreateVideoPlayer(const std::string& videoPath);
};