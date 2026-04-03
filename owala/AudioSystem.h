/*****************************************************************//**
 * \file   AudioSystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (70%)
 * \secondary_author
 *      Yar Xuan  (10%)
 *      Ng Jing Wen (10%)
 *      Jason Lou (10%)
 *
 * \brief
 *   Audio system wrapper around FMOD for loading, playing, and managing
 *   sounds within the Framework ECS. Provides simple caching by ID,
 *   fire-and-forget playback, master volume control, bulk stop,
 *   per-channel-group reverb (DSP), and basic 2D->3D spatial audio.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "ISystem.h"
#include "ResourceManager.h"
#include "EntityStorage.h"
#include "GameMessages.h"
#include "MessageBus.h"

#include <fmod.hpp>
#include <fmod_errors.h>
#include <fmod_dsp.h>   // DSP parameter enums live here

#include <string>
#include <unordered_map>
#include <vector>
#include <map>
#include <cstdio>
#include <algorithm>

namespace Framework {

    /*****************************************************************//**
     * \class   AudioSystem
     * \brief   Manages all audio playback, volume routing, 3D spatial
     *          sound, and FMOD library lifecycle.
     *********************************************************************/
    class AudioSystem : public ISystem {
    public:
        AudioSystem() = default;
        ~AudioSystem() override = default;

        /*****************************************************************//**
         * \brief   Sets up the FMOD core system and channel groups.
         *********************************************************************/
        void Initialize() override;

        /*****************************************************************//**
         * \brief   Updates FMOD state, listener position, and fading channels.
         *********************************************************************/
        void Update() override;

        /*****************************************************************//**
         * \brief   Cleans up all audio resources and shuts down FMOD.
         *********************************************************************/
        void Shutdown() override;

        /*****************************************************************//**
         * \brief   Returns the raw FMOD system pointer.
         *********************************************************************/
        FMOD::System* getFMODSystem() const;

        // ---------------- Loading ----------------

        /*****************************************************************//**
         * \brief   Loads a sound file into memory or prepares it for streaming.
         *********************************************************************/
        bool loadSound(const std::string& id,
            const char* path,
            bool loop = false,
            bool stream = false,
            bool is3D = false);

        // ---------------- Playback ----------------

        /*****************************************************************//**
         * \brief   Plays a standard 2D sound.
         *********************************************************************/
        FMOD::Channel* play(FMOD::Sound& sound, AudioType type, float volume = 1.0f, bool paused = false);

        /*****************************************************************//**
         * \brief   Plays a sound spatially in the 3D world.
         *********************************************************************/
        FMOD::Channel* play3D(FMOD::Sound& sound, AudioType type, float volume,
            const Vector2D& worldPos,
            float minDistance = 50.0f, float maxDistance = 700.0f,
            bool paused = false);

        /*****************************************************************//**
         * \brief   Quickly loads and plays a sound once.
         *********************************************************************/
        bool playOneShot(const char* path, float volume = 1.0f);

        /*****************************************************************//**
         * \brief   Sets the global volume level.
         *********************************************************************/
        void setMasterVolume(float volume);

        /*****************************************************************//**
         * \brief   Sets the music channel volume level.
         *********************************************************************/
        void setMusicVolume(float volume);

        /*****************************************************************//**
         * \brief   Sets the sound effects channel volume level.
         *********************************************************************/
        void setSFXVolume(float volume);

        /*****************************************************************//**
         * \brief   Gets the current global volume level.
         *********************************************************************/
        float getMasterVolume();

        /*****************************************************************//**
         * \brief   Gets the current music channel volume level.
         *********************************************************************/
        float getMusicVolume();

        /*****************************************************************//**
         * \brief   Gets the current sound effects channel volume level.
         *********************************************************************/
        float getSFXVolume();
        
        /*****************************************************************//**
         * \brief   Stops the 2D audio assigned to a specific entity.
         *********************************************************************/
        void Stop2DAudio(Entity entity, float fadeDuration = 0.f);

        /*****************************************************************//**
         * \brief   Stops the 3D audio assigned to a specific entity.
         *********************************************************************/
        void Stop3DAudio(Entity entity, float fadeDuration = 0.f);

        /*****************************************************************//**
         * \brief   Stops all audio associated with a specific entity.
         *********************************************************************/
        void StopAudio(Entity entity, float fadeDuration = 0.f);

        /*****************************************************************//**
         * \brief   Stops all currently playing audio.
         *********************************************************************/
        void stopAll(bool fadeOut = true);

        /*****************************************************************//**
         * \brief   Removes a loaded sound from memory.
         *********************************************************************/
        void unloadSound(const std::string& id);

        /*****************************************************************//**
         * \brief   Checks an FMOD result and logs any standard errors.
         *********************************************************************/
        static void check(FMOD_RESULT r, const char* where);

        /*****************************************************************//**
         * \brief   Listens for UI button clicks to play feedback sounds.
         *********************************************************************/
        void OnButtonClicked(const ButtonClickedEvent& event);

        // Interruption handling

        /*****************************************************************//**
         * \brief   Pauses all application audio temporarily.
         *********************************************************************/
        void SuspendAudio();

        /*****************************************************************//**
         * \brief   Resumes all application audio that was suspended.
         *********************************************************************/
        void ResumeAudio();

        // Random SFX helpers

        /*****************************************************************//**
         * \brief   Gets a random sound path associated with an event key.
         *********************************************************************/
        std::string getRandomSound(std::string eventKey);

        /*****************************************************************//**
         * \brief   Plays a randomly selected 2D sound from a given event key.
         *********************************************************************/
        FMOD::Channel* PlayRandomSound(std::string eventKey, AudioType audiotype, float volume, bool startPaused = false);

        /*****************************************************************//**
         * \brief   Plays a randomly selected 3D sound from a given event key.
         *********************************************************************/
        FMOD::Channel* PlayRandomSound3D(std::string eventKey, AudioType audiotype, float volume, Vector2D position, float minDist, float maxDist, bool startPaused = false);

        // Fade helper

        /*****************************************************************//**
         * \brief   Gradually lowers the volume of a channel until it stops.
         *********************************************************************/
        void FadeOutChannel(FMOD::Channel* channel, float duration);

        // ---------------- Reverb (per ChannelGroup) ----------------

        /*****************************************************************//**
         * \brief   Turns the reverb effect on or off for a specific audio type.
         *********************************************************************/
        void EnableGroupReverb(AudioType type, bool enable);

        /*****************************************************************//**
         * \brief   Sets the reverb wet mix level in decibels.
         *********************************************************************/
        void SetGroupReverbWetDb(AudioType type, float wetDb);   // e.g. -18..0 dB

        /*****************************************************************//**
         * \brief   Modifies the simulated room size for the reverb effect.
         *********************************************************************/
        void SetGroupReverbRoom(AudioType type, float room01);   // 0..1 (mapped to decay/density/diffusion)

        /*****************************************************************//**
         * \brief   Adjusts how long the reverb effect takes to decay.
         *********************************************************************/
        void SetGroupReverbDecay(AudioType type, float decayMs); // milliseconds

        // ---------------- Spatial audio ----------------

        /*****************************************************************//**
         * \brief   Configures global 3D spatial sound parameters in FMOD.
         *********************************************************************/
        void Set3DSettings(float dopplerScale, float distanceFactor, float rolloffScale);

    private:
        /*****************************************************************//**
         * \brief   Updates the main 3D listener position to match the player location.
         *********************************************************************/
        void UpdateListenerFromPlayer_();

        /*****************************************************************//**
         * \brief   Updates the 3D position of all currently tracked entity sounds.
         *********************************************************************/
        void Update3DChannelPositions_();

        // Tracks channels fading out: <Channel Pointer, Fade Speed (vol/sec)>
        std::unordered_map<FMOD::Channel*, float> fadingChannels_;

        // FMOD Core pointers
        FMOD::System* system_ = nullptr;
        FMOD::ChannelGroup* masterGroup_ = nullptr;
        FMOD::ChannelGroup* musicGroup_ = nullptr;
        FMOD::ChannelGroup* sfxGroup_ = nullptr;
        FMOD::ChannelGroup* drySfxGroup_ = nullptr;

        // Per-group reverb DSPs
        FMOD::DSP* musicReverbDSP_ = nullptr;
        FMOD::DSP* sfxReverbDSP_ = nullptr;

        // Caches
        std::unordered_map<std::string, FMOD::Sound*>   sounds_;
        std::unordered_map<std::string, FMOD::Channel*> channels_;
        std::unordered_map<std::string, std::vector<std::string>> soundPaths_;
        std::map<Entity, FMOD::Channel*> entityChannels_;    // AudioComponent-driven 2D channels
        std::map<Entity, FMOD::Channel*> entity3DChannels_;  // PlaySound3DEvent-driven 3D channels (position-tracked)

        // Spatial audio tunables
        float distanceFactor_ = 1.0f;
        float minHearingRange = 50.0f;
        float maxHearingRange = 700.0f;

        // Data-driven animation event to audio mapping (loaded from animationEventAudio.json)
        std::unordered_map<std::string, AnimationEventAudioConfig> animationEventAudioMap_;

        // Track the last played sound index for each event key
        std::unordered_map<std::string, int> lastPlayedIndices_;
    };

} // namespace Framework