# Owala Engine Context Summary (for AI prompting)

## 1) What this project is
- **Repository:** `CrikNJW/Owala-Engine`
- **Engine:** Custom C++ ECS-based engine in `owala\`
- **Game currently integrated:** `StageBound\`
- **Editor:** In-engine editor tools in `Editor\` (compiled into `owala_Engine` target)
- **Build system:** CMake + Visual Studio solution generation (`run.bat` / `clean.bat`)

**Game concept (StageBound):** 2D puzzle/platformer where the player rotates the world (90-degree steps), navigates trap platforms/enemies, and exits through a portal.

## 2) Top-level structure
- `owala\` → reusable engine systems (core loop, ECS, physics, rendering, audio, scene/game managers, scripting, VFX, GUI)
- `Editor\` → editor-only systems/UI (inspector, asset browser, gizmos, history/undo-redo, project scaffold)
- `StageBound\` → game-specific code + data + assets
- `CMakeLists.txt` + `active_game.cmake` → target wiring and active game injection

## 3) Build targets and runtime modes
- **`owala_Engine` target** = Engine + Editor code (`BUILD_EDITOR` defined).
- **`StageBound` target** = Engine + StageBound game code (no editor code).
- `active_game.cmake` currently sets active game to **StageBound**.

Core bootstrap entrypoint: `owala\main.cpp`
- Creates core systems, registers them to `GameManager`, adds them to `CoreEngine` in update order.
- Injects game app via `CreateApplication(CoreEngine&)` (from StageBound app when active game is included).

## 4) Core architecture
### 4.1 Engine loop
`CoreEngine` (`owala\Core\Core.h/.cpp`)
- Owns `ISystem*` list.
- `Initialize()` calls system initialize in order.
- `GameLoop()` updates systems each frame, tracks per-system timings.
- Supports **Editor mode** vs **Play mode** filtering:
  - In editor mode, gameplay-heavy systems (physics/audio/platform behavior/game systems) are skipped.
- Handles focus/minimize interruption behavior:
  - Resets input, suspends audio, pauses physics while unfocused/minimized.

### 4.2 ECS
`EntityStorage` (`owala\Entities\EntityStorage.h`)
- Archetype-bucket style storage + component signatures.
- Component enum (`ComponentType`) includes gameplay, render, GUI, scripting, FSM, particle, tags.

Core component definitions: `owala\Entities\Components.h`
- Transform/Physics: `Transform`, `Velocity`, `Acceleration`, `RigidBody`, `AABBCollider`
- Gameplay: `PlayerState`, `TrapPlatformComponent`, `PlayerHealthComponent`
- Render/UI: `TextureComponent`, `AnimationComponent`, `TextComponent`, `Button`, `Slider`
- Camera/3D: `Camera`, `Transform3D`, `Primitive3D`, `DirectionalLight3D`
- AI/Scripting/VFX: `FSMComponent`, `NativeScriptsComponent`, `ParticleComponent`, `ParticleEmitterComponent`
- Tags: `PlayerTag`, `EnemyTag`, `PlatformTag`, `GUITag`, `SceneOwned`, `PersistentTag`, `LoadedTag`, etc.

### 4.3 Messaging/event bus
- `MessageBus` singleton (`owala\MessageBus.h`) provides pub/sub by message type.
- Message/event definitions in `owala\GameMessages.h` include:
  - Audio triggers (`PlaySoundEvent`, `PlaySound3DEvent`)
  - UI button actions (`ButtonClickedEvent`, scene switch/resume/quit actions)
  - Gameplay flow (`LevelStartEvent`, `LevelClearEvent`, `PlayerDeathEvent`, `PlayerDamageRequestEvent`)
  - VFX/animation events (`ParticleBurstEvent`, `VFXTriggerEvent`, `AnimationEvent`)

### 4.4 Scene & game coordination
- `GameManager` singleton (`owala\Managers\GameManager\GameManager.*`):
  - Central hub for registered systems
  - Scene callback registration (init/update/free per scene)
  - Fade overlay and queued/deferred transitions
  - Native/C# script update bridge
  - Editor play-mode snapshot/restore flow (runtime ECS copy vs editor ECS)
- `SceneManager` singleton (`owala\Managers\SceneManager\SceneManager.*`):
  - Scene asset registry from `Data/scenes.json`
  - Single/additive loading, unloading, loaded tags, scene ownership
  - DDOL-style persistent scene support (`Persistent` reserved scene)

## 5) Key engine systems currently wired in main
From `owala\main.cpp`:
- `WindowsSystem`
- `InputSystem`
- `TransformationSystem`
- `PhysicsSystem`
- `AudioSystem`
- `PlatformBehaviorSystem`
- `AISystem`
- `VFXSystem`
- `GameManager`
- `ResourceManager`
- `CameraSystem`
- `AnimationSystem`
- `GuiSystem`
- `Graphics`
- Editor-only: `Editor`, `GizmoSystem`

## 6) StageBound (game layer) specifics
Game app entry: `StageBound\Source\StageBoundApp.cpp`
- Adds game systems:
  - `PlayerSystem`
  - `TrapPlatformSystem`
  - `SB_InputSystem` (debug/cheat/testing shortcuts)
- Loads game config and registers scene lifecycle scripts.

### 6.1 StageBound data/config
- `StageBound\Data\GameConfig.json`
  - startup scene: `MainMenu`
  - loading scene: `LoadingScreen`
  - overlay scenes: `PauseScene`, `OptionsMenu`
- `StageBound\Data\scenes.json`
  - includes `MainMenu`, `LevelSelection`, `HowToPlay`, `Tutorial 1/2`, `Level 1-4`, `PauseScene`, `GameWinScreen`, etc.
- `StageBound\Data\config.json`
  - default runtime window config: 1920x1080 fullscreen

### 6.2 StageBound gameplay systems/scripts
- `PlayerSystem` handles movement/combat/death/invulnerability and player/enemy trap interactions.
- `TrapPlatformSystem` reacts to generic landing events and emits trap-specific events/audio.
- Enemy FSM/state definitions present under `StageBound\Source\AI\`.
- Many scene/UI/gameplay scripts under `StageBound\Source\Native\` (pause menu, loading flow, transitions, save, portal, HUD, etc.).

## 7) Editor capabilities (high-level)
Editor code in `Editor\` includes:
- Asset browser/viewers
- Gizmo system
- History/undo-redo
- Animation editor support
- Project scaffolding helpers
- Viewport rendering helpers

The editor can run play sessions by snapshotting editor ECS and swapping to a runtime ECS copy, then restoring after stop.

## 8) Data-driven files AI should know about
- **Window/world/game config:** `Data/config.json`, `Data/Config/worldConfig.json`, `Data/GameConfig.json`
- **Scene catalog and scene files:** `Data/scenes.json`, `Data/Scenes/*.json`
- **Audio map:** `Data/audioFiles.json`
- **Animation/state maps:** `Data/stateToAnimationMapping.json`, `Data/textureToAnimationMapping.json`, `Data/animationEventAudio.json`
- **Layers/prefabs/particles:** `Data/LayerConfig.json`, `Data/Prefabs/*`, `Data/ParticleEffects/*`

