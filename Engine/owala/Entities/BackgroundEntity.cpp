#include "BackgroundEntity.h"
#include "EntityManager.h"

namespace Framework{
	Entity createBackground(EntityManager& entityMgr, Vector2D pos, std::string textureID)
	{
		Entity bg = entityMgr.CreateEntity();

		entityMgr.AddComponent(bg, BackgroundTag{});
		entityMgr.AddComponent(bg, Position{ pos });
		entityMgr.AddComponent(bg, TextureComponent{ textureID ,  {1.0f, 1.0f } });

		return bg;
	}
}