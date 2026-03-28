#include "game/systems/BoardSystem.h"
#include "game/components/GameComponents.h"
#include "game/config/GameConfig.h"
#include "engine/components/Transform.h"
#include <algorithm>

using namespace config;

BoardSystem::BoardSystem(ecs::Registry& registry,
                         AnimationSystem& animSys,
                         Board& board,
                         const std::array<components::Mesh, NUM_TYPES>& meshes)
    : m_registry(registry)
    , m_animSys(animSys)
    , m_board(board)
    , m_meshes(meshes)
{}

void BoardSystem::beginSwap(int fromCol, int fromRow, int toCol, int toRow) {
    m_selCol = fromCol;  m_selRow = fromRow;
    m_swpCol = toCol;    m_swpRow = toRow;

    auto eA = m_board.entities[fromCol][fromRow];
    auto eB = m_board.entities[toCol][toRow];
    glm::vec3 posA = boardToWorld(fromCol, fromRow);
    glm::vec3 posB = boardToWorld(toCol,   toRow);

    m_registry.emplace<Animating>(eA, Animating{posA, posB, 0.0f});
    m_registry.emplace<Animating>(eB, Animating{posB, posA, 0.0f});

    std::swap(m_board.grid[fromCol][fromRow],     m_board.grid[toCol][toRow]);
    std::swap(m_board.entities[fromCol][fromRow],  m_board.entities[toCol][toRow]);
    m_registry.get<BoardPos>(eA) = {toCol, toRow};
    m_registry.get<BoardPos>(eB) = {fromCol, fromRow};

    m_phase = Phase::Swapping;
}

void BoardSystem::update() {
    switch (m_phase) {
    case Phase::Idle:
        break;
    case Phase::Swapping:
        if (m_animSys.allAnimsDone())
            m_phase = Phase::Checking;
        break;
    case Phase::Checking:
        doChecking();
        break;
    case Phase::SwapBack:
        if (m_animSys.allAnimsDone())
            m_phase = Phase::Idle;
        break;
    case Phase::Destroying:
        if (m_animSys.allDestroysDone())
            m_phase = Phase::Falling;
        break;
    case Phase::Falling:
        doFalling();
        break;
    case Phase::Spawning:
        if (m_animSys.allAnimsDone())
            doSpawning();
        break;
    case Phase::Recheck:
        if (m_animSys.allAnimsDone())
            m_phase = Phase::Checking;
        break;
    }
}

void BoardSystem::doChecking() {
    auto matches = findMatches(m_board);

    if (matches.empty()) {
        if (m_selCol >= 0 && m_swpCol >= 0) {
            auto eA = m_board.entities[m_selCol][m_selRow];
            auto eB = m_board.entities[m_swpCol][m_swpRow];
            glm::vec3 posA = boardToWorld(m_selCol, m_selRow);
            glm::vec3 posB = boardToWorld(m_swpCol, m_swpRow);

            m_registry.emplace<Animating>(eA, Animating{posA, posB, 0.0f});
            m_registry.emplace<Animating>(eB, Animating{posB, posA, 0.0f});

            std::swap(m_board.grid[m_selCol][m_selRow],    m_board.grid[m_swpCol][m_swpRow]);
            std::swap(m_board.entities[m_selCol][m_selRow], m_board.entities[m_swpCol][m_swpRow]);
            m_registry.get<BoardPos>(eA) = {m_swpCol, m_swpRow};
            m_registry.get<BoardPos>(eB) = {m_selCol, m_selRow};
        }
        m_selCol = m_selRow = m_swpCol = m_swpRow = -1;
        m_phase = Phase::SwapBack;
        return;
    }

    m_score += static_cast<int>(matches.size()) * SCORE_PER_GEM;
    for (auto& m : matches) {
        auto e = m_board.entities[m.col][m.row];
        if (e != ecs::NullEntity && !m_registry.has<Destroying>(e))
            m_registry.emplace<Destroying>(e, Destroying{1.0f});
        m_board.grid[m.col][m.row]     = -1;
        m_board.entities[m.col][m.row] = ecs::NullEntity;
    }
    m_selCol = m_selRow = m_swpCol = m_swpRow = -1;
    m_phase = Phase::Destroying;
}

void BoardSystem::doFalling() {
    for (int c = 0; c < BOARD_W; ++c) {
        int writeRow = 0;
        for (int r = 0; r < BOARD_H; ++r) {
            if (m_board.grid[c][r] >= 0) {
                if (r != writeRow) {
                    m_board.grid[c][writeRow]     = m_board.grid[c][r];
                    m_board.entities[c][writeRow]  = m_board.entities[c][r];
                    m_board.grid[c][r]            = -1;
                    m_board.entities[c][r]        = ecs::NullEntity;

                    auto e = m_board.entities[c][writeRow];
                    m_registry.get<BoardPos>(e) = {c, writeRow};
                    m_registry.emplace<Animating>(e,
                        Animating{boardToWorld(c, r), boardToWorld(c, writeRow), 0.0f});
                }
                ++writeRow;
            }
        }
    }
    m_phase = Phase::Spawning;
}

void BoardSystem::doSpawning() {
    for (int c = 0; c < BOARD_W; ++c) {
        float topY = boardToWorld(0, BOARD_H).y + CELL_SIZE;
        int spawned = 0;
        for (int r = 0; r < BOARD_H; ++r) {
            if (m_board.grid[c][r] < 0) {
                int type = m_board.randomType();
                glm::vec3 target = boardToWorld(c, r);
                glm::vec3 start  = target;
                start.y = topY + spawned * CELL_SIZE;

                auto e = m_registry.create();
                m_registry.emplace<components::Transform>(e, components::Transform{
                    .position = start,
                    .scale    = glm::vec3(GEM_SCALE),
                });
                m_registry.emplace<components::Mesh>(e, m_meshes[type]);
                m_registry.emplace<GemType>(e, GemType{type});
                m_registry.emplace<BoardPos>(e, BoardPos{c, r});
                m_registry.emplace<Animating>(e, Animating{start, target, 0.0f});

                m_board.entities[c][r] = e;
                m_board.grid[c][r]     = type;
                ++spawned;
            }
        }
    }
    m_phase = Phase::Recheck;
}
