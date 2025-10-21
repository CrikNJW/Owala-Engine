#include "AudioEntity.h"

namespace Framework {
	Entity createBGAudio(EntityManager& entityMgr,
		std::string audioID) {

		//Create a new entity with a unique ID
		Entity bg_audio = entityMgr.CreateEntity();
		/*Assign components to the player entity, and
		set initial values in the unordered maps.*/

		//entityStorage.tex[player] = TextureComponent{ texture };
		entityMgr.AddComponent(bg_audio, AudioComponent{ audioID, true, true });

		//Return the entity ID of the player
		return bg_audio;
	}
}