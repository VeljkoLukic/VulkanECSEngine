#pragma once
#include "engine/ecs/Types.h"
#include "game/config/GameConfig.h"
#include <glm/glm.hpp>
#include <random>
#include <vector>

enum class Phase {
    Idle,
    Swapping,
    Checking,
    SwapBack,
    Destroying,
    Falling,
    Spawning,
    Recheck,
};

struct Match { int col, row; };

struct Board {
    int           grid[config::BOARD_W][config::BOARD_H]{};
    ecs::EntityID entities[config::BOARD_W][config::BOARD_H]{};
    std::mt19937  rng{ std::random_device{}() };

    int randomType();
};

glm::vec3           boardToWorld(int col, int row);
void                fillBoardNoMatches(Board& board);
std::vector<Match>  findMatches(const Board& board);
