#pragma once
#include "engine/ecs/Registry.h"
#include "engine/components/Mesh.h"
#include "game/board/Board.h"
#include "game/systems/AnimationSystem.h"
#include <array>

class BoardSystem {
public:
    BoardSystem(ecs::Registry& registry,
                AnimationSystem& animSys,
                Board& board,
                const std::array<components::Mesh, config::NUM_TYPES>& meshes);

    [[nodiscard]] Phase phase() const { return m_phase; }

    void beginSwap(int fromCol, int fromRow, int toCol, int toRow);
    void update();

    [[nodiscard]] int score() const { return m_score; }

private:
    void doChecking();
    void doFalling();
    void doSpawning();

    ecs::Registry&     m_registry;
    AnimationSystem&   m_animSys;
    Board&             m_board;
    const std::array<components::Mesh, config::NUM_TYPES>& m_meshes;

    Phase m_phase   = Phase::Idle;
    int   m_score   = 0;

    int m_selCol = -1, m_selRow = -1;
    int m_swpCol = -1, m_swpRow = -1;
};
