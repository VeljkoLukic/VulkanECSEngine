#include "engine/core/Engine.h"
#include "engine/components/Transform.h"
#include "engine/components/Mesh.h"

#include "game/config/GameConfig.h"
#include "game/components/GameComponents.h"
#include "game/board/Board.h"
#include "game/board/MeshFactory.h"
#include "game/systems/AnimationSystem.h"
#include "game/systems/InputSystem.h"
#include "game/systems/BoardSystem.h"
#include "engine/systems/FreeFlyCamera.h"

#include <array>

using namespace config;

int main() {
    try {
        core::Engine engine(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);

        auto& cam  = engine.camera();
        cam.eye    = { 0.0f, 0.0f, CAMERA_DISTANCE };
        cam.target = { 0.0f, 0.0f, 0.0f };
        cam.up     = { 0.0f, 1.0f, 0.0f };
        cam.fovY   = CAMERA_FOV;

        auto& reg = engine.registry();

        Board board;
        fillBoardNoMatches(board);

        std::array<components::Mesh, NUM_TYPES> gemMeshes;
        for (int i = 0; i < NUM_TYPES; ++i)
            gemMeshes[i] = makeGemMesh(i);

        for (int c = 0; c < BOARD_W; ++c) {
            for (int r = 0; r < BOARD_H; ++r) {
                auto e = reg.create();
                reg.emplace<components::Transform>(e, components::Transform{
                    .position = boardToWorld(c, r),
                    .scale    = glm::vec3(GEM_SCALE),
                });
                reg.emplace<components::Mesh>(e, gemMeshes[board.grid[c][r]]);
                reg.emplace<GemType>(e, GemType{ board.grid[c][r] });
                reg.emplace<BoardPos>(e, BoardPos{ c, r });
                board.entities[c][r] = e;
            }
        }

        auto cursorEntity = reg.create();
        reg.emplace<components::Transform>(cursorEntity, components::Transform{
            .position = CURSOR_HIDDEN,
            .scale    = glm::vec3(GEM_SCALE * CURSOR_SCALE_MULT),
        });
        reg.emplace<components::Mesh>(cursorEntity, makeCursorMesh());

        AnimationSystem animSys(reg, engine.renderer());
        InputSystem     inputSys(engine.window(), cam);
        BoardSystem     boardSys(reg, animSys, board, gemMeshes);

        systems::FreeFlyCamera flyCam(engine.window(), cam);

        engine.setUpdateCallback([&](float dt) {
            bool flying = flyCam.update(dt);

            animSys.update(dt);

            if (!flying && boardSys.phase() == Phase::Idle) {
                int cursorCol = -1, cursorRow = -1;
                auto input = inputSys.update(cursorCol, cursorRow);

                if (cursorCol >= 0 && cursorRow >= 0)
                    reg.get<components::Transform>(cursorEntity).position =
                        boardToWorld(cursorCol, cursorRow) + glm::vec3(0.0f, 0.0f, CURSOR_Z_OFFSET);
                else
                    reg.get<components::Transform>(cursorEntity).position = CURSOR_HIDDEN;

                if (input.wantSwap) {
                    boardSys.beginSwap(input.fromCol, input.fromRow,
                                       input.toCol,   input.toRow);
                    inputSys.clearSelection();
                }
            }

            boardSys.update();
        });

        engine.run();

    } catch (const std::exception& e) {
        fprintf(stderr, "Fatal: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
