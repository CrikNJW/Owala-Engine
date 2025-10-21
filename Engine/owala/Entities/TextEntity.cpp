#include "TextEntity.h"
#include "EntityManager.h"

namespace Framework{
	Entity createText(EntityManager& entityMgr, std::string textToDraw, std::string fontName, Color color, Vector2D pos, float scale)
	{
		Entity txt = entityMgr.CreateEntity();

		entityMgr.AddComponent(txt, TextComponent{ textToDraw, fontName, color, scale });
		entityMgr.AddComponent(txt, Position{ pos });

		return txt;
	}
}