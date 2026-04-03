/*****************************************************************//**
 * \file	FogEmitter.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author

 *
 * \brief   Script for emitting fog VFX in the level.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once

#include "ScriptableEntity.h"
#include "Components.h"

using namespace Framework;

/*****************************************************************//**
 * \class   FogEmitter
 * \brief   Script that spawns and manages moving fog over time.
 *********************************************************************/
class FogEmitter : public ScriptableEntity {
protected:
	/*****************************************************************//**
	 * \brief   Initializes the emitter and randomizes the first spawn timer.
	 *********************************************************************/
	void OnCreate() override;

	/*****************************************************************//**
	 * \brief   Updates existing fog lifetimes and spawns new fog.
	 *********************************************************************/
	void OnUpdate() override;

	/*****************************************************************//**
	 * \brief   Cleans up all tracked fog entities on destruction.
	 *********************************************************************/
	void OnDestroy() override;

private:
	/*****************************************************************//**
	 * \struct  FogParticle
	 * \brief   Tracks a single fog entity and its remaining lifespan.
	 *********************************************************************/
	struct FogParticle {
		Entity entity;
		float lifeTimer;
		float maxLife;
	};

	std::vector<FogParticle> activeFogs;
	float nextSpawnTimer;

	Vector2D fogTextureHalfSize = { 200.0f, 200.0f };
	float fogVelocity = 50.0f;
	float fogLifeSpan = 15.0f; //15sec

	float minSpawnInterval = 4.0f;
	float maxSpawnInterval = 8.0f;
};
