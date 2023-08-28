#include "solitude/generator.h"

#include <clu/static_vector.h>
#include <clu/random.h>

namespace sltd
{
    namespace
    {
        auto generate_symmetry_map(const SymmetryType symmetry)
        {
            using enum SymmetryType;
            clu::static_vector<std::pair<int, int>, cell_count> pairs;
            switch (symmetry)
            {
                case none:
                    for (int i = 0; i < cell_count; i++)
                        pairs.push_back({i, i});
                    break;
                case centrosymmetric:
                    for (int i = 0; i < (cell_count + 1) / 2; i++)
                        pairs.push_back({i, cell_count - 1 - i});
                    break;
                case axisymmetric:
                    for (int i = 0; i < board_size; i++)
                        for (int j = 0; j < (board_size + 1) / 2; j++)
                            pairs.push_back({i * board_size + j, (i + 1) * board_size - 1 - j});
                    break;
                default: throw std::runtime_error("Unknown symmetry type");
            }
            return pairs;
        }
    } // namespace

    Board generate_minimal_puzzle(const SymmetryType symmetry)
    {
        auto current = Board::random_filled_board();
        auto sets = generate_symmetry_map(symmetry);
        std::ranges::shuffle(sets, clu::thread_rng());
        while (!sets.empty())
        {
            const auto pair = sets.back();
            sets.pop_back();
            auto board = current;
            board.set_unknown_at(pair.first);
            board.set_unknown_at(pair.second);
            if (board.brute_force_solve(2) == 1)
            {
                current.set_unknown_at(pair.first);
                current.set_unknown_at(pair.second);
            }
        }
        current.eliminate_candidates();
        return current;
    }
} // namespace sltd
