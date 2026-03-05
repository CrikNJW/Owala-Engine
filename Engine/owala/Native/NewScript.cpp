#include "NewScript.h"
#include "ScriptRegistry.h"

REGISTER_SCRIPT(NewScript, "NewScript")

void NewScript::OnCreate() {
	// Initialization Code Here
}

void NewScript::OnUpdate() {
	// Per-Frame Logic Here
}

void NewScript::OnDestroy() {
	// Logic to be called before Entity is deleted
}
