# Owala Engine and StageBound

Owala is a custom C++ game engine and editor. StageBound, the game built on it, is a puzzle platformer where you rotate a theatre to shift platforms and reach the exit. A nine-person student team built the game and engine together.

Ng Jing Wen helped lead the project, built animation, particle, interface and asset tools, and separated game-specific code from the engine so its systems could be reused.

## Build and run

1. On Windows, install Visual Studio with the Desktop development with C++ workload and CMake as specified in the [project guide](ProjectGuide.txt).
2. Set the active game to StageBound in [active_game.cmake](active_game.cmake) and run [run.bat](run.bat) from the repository root.
3. Open the generated solution under build. Select StageBound to play the game or owala_Engine to work in the editor.

In StageBound, move with A and D, jump with W or Space, rotate the level with Q and E, and use F to enter the exit portal. See the [editor guide](editorGuide.pdf) and [original game notes](Owala_Readme.txt) for more detail.
