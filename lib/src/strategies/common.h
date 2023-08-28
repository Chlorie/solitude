#pragma once

#include <array>

#include "solitude/board.h"

namespace sltd
{
    constexpr auto generate_house_indices()
    {
        // 3 * board_size for rows, columns, and boxes
        std::array<std::array<int, board_size>, 3 * board_size> res{}; // NOLINT
        // Rows and columns
        for (int i = 0; i < board_size; i++)
            for (int j = 0; j < board_size; j++)
            {
                res[i][j] = i * board_size + j;
                res[j + board_size][i] = i * board_size + j;
            }
        // Boxes
        for (int i = 0; i < board_size; i++)
        {
            const int br = i / box_height * box_height, bc = i % box_height * box_width;
            for (int j = 0; j < board_size; j++)
                res[i + 2 * board_size][j] = (br + j / box_width) * board_size + bc + j % box_width;
        }
        return res;
    }

    constexpr auto house_indices = generate_house_indices();

    std::string cell_name(int idx);
    std::string house_name(int idx);
} // namespace sltd
