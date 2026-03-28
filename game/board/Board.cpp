#include "game/board/Board.h"
#include <set>

using namespace config;

int Board::randomType() {
    return std::uniform_int_distribution<int>(0, NUM_TYPES - 1)(rng);
}

glm::vec3 boardToWorld(int col, int row) {
    float x = (col - BOARD_W * 0.5f + 0.5f) * CELL_SIZE;
    float y = (row - BOARD_H * 0.5f + 0.5f) * CELL_SIZE;
    return { x, y, 0.0f };
}

void fillBoardNoMatches(Board& board) {
    for (int c = 0; c < BOARD_W; ++c) {
        for (int r = 0; r < BOARD_H; ++r) {
            int type;
            int attempts = 0;
            do {
                type = board.randomType();
                bool bad = false;
                if (c >= 2 && board.grid[c-1][r] == type && board.grid[c-2][r] == type)
                    bad = true;
                if (r >= 2 && board.grid[c][r-1] == type && board.grid[c][r-2] == type)
                    bad = true;
                if (!bad) break;
                ++attempts;
            } while (attempts < MAX_FILL_ATTEMPTS);
            board.grid[c][r] = type;
        }
    }
}

std::vector<Match> findMatches(const Board& board) {
    std::set<std::pair<int,int>> matched;

    for (int r = 0; r < BOARD_H; ++r) {
        for (int c = 0; c < BOARD_W - 2; ) {
            int t = board.grid[c][r];
            if (t < 0) { ++c; continue; }
            int end = c + 1;
            while (end < BOARD_W && board.grid[end][r] == t) ++end;
            if (end - c >= 3)
                for (int x = c; x < end; ++x) matched.emplace(x, r);
            c = end;
        }
    }

    for (int c = 0; c < BOARD_W; ++c) {
        for (int r = 0; r < BOARD_H - 2; ) {
            int t = board.grid[c][r];
            if (t < 0) { ++r; continue; }
            int end = r + 1;
            while (end < BOARD_H && board.grid[c][end] == t) ++end;
            if (end - r >= 3)
                for (int y = r; y < end; ++y) matched.emplace(c, y);
            r = end;
        }
    }

    std::vector<Match> result;
    result.reserve(matched.size());
    for (auto& [c, r] : matched) result.push_back({c, r});
    return result;
}
