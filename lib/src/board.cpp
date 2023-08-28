#include "solitude/board.h"

#include <bit>
#include <algorithm>
#include <fmt/format.h>
#include <clu/static_vector.h>

#include "solitude/utils.h"

namespace sltd
{
    namespace
    {
        char to_char_repr(const CandidateMask mask)
        {
            const int num = std::countr_zero(mask) + 1;
            return static_cast<char>(num < 10 ? num + '0' : num - 10 + 'A');
        }

        CandidateMask from_char_repr(const char ch)
        {
            if (ch >= '1' && ch <= '9')
                return 1 << (ch - '1');
            else if (ch >= 'A' && ch <= 'G')
                return 1 << (ch - 'A' + 9);
            throw std::runtime_error(fmt::format("Unknown character {:?}", ch));
        }

        class BruteForceSolver
        {
        public:
            BruteForceSolver(const Board& board, const int max_solutions, const bool randomized):
                max_solutions_(max_solutions), randomized_(randomized), solution_(board)
            {
            }

            std::pair<Board, int> solve()
            {
                if (!solution_.reduce_candidates())
                    return {solution_, 0};
                solve_recurse(solution_);
                return {solution_, solution_count_};
            }

        private:
            int max_solutions_;
            bool randomized_;
            int solution_count_ = 0;
            Board solution_;

            void solve_recurse(Board state)
            {
                if (!exhaust_naked_singles(state))
                    return;
                if (state.filled.all()) // New solution
                {
                    solution_count_++;
                    solution_ = state;
                    return;
                }
                // Find cell with the least candidates
                int min_candidates = board_size + 1, min_idx = 0;
                for (int i = 0; i < cell_count; i++)
                {
                    if (state.filled[i])
                        continue;
                    if (const auto count = std::popcount(state.cells[i]); count < min_candidates)
                    {
                        min_candidates = count;
                        min_idx = i;
                        if (min_candidates == 2)
                            break;
                    }
                }
                // Try the candidates one by one
                auto candidates = state.cells[min_idx];
                while (candidates != 0)
                {
                    const auto bit = randomized_ ? get_random_bit(candidates) : get_rightmost_set_bit(candidates);
                    state.cells[min_idx] = bit;
                    solve_recurse(state);
                    if (solution_count_ >= max_solutions_)
                        return;
                    candidates &= ~bit;
                }
            }

            static bool exhaust_naked_singles(Board& state)
            {
                while (true)
                {
                    bool exhausted = true;
                    for (int i = 0; i < cell_count; i++)
                    {
                        if (state.filled[i] || !std::has_single_bit(state.cells[i]))
                            continue;
                        exhausted = false;
                        state.filled.set(i);
                        if (!state.reduce_candidates_from_naked_single(i))
                            return false;
                    }
                    if (exhausted)
                        return true;
                }
            }
        };

        constexpr auto candidate_to_braille_position()
        {
            constexpr std::array<std::uint8_t, 8> values{1, 2, 4, 64, 8, 16, 32, 128}; // column-major
            std::array<std::pair<bool, std::uint8_t>, board_size> res{};
            for (int i = 0; i < box_height; i++)
                for (int j = 0; j < box_width; j++)
                    res[i * box_width + j] = {j >= 2, values[j % 2 * 4 + i]};
            return res;
        }

        constexpr auto candidate_braille_positions = candidate_to_braille_position();

        std::string braille_dots(const std::uint8_t value)
        {
            const std::uint32_t cp = 0x2800 + value;
            const char bytes[4]{ // Convert codepoint to 3 UTF-8 bytes
                static_cast<char>(((cp >> 12) & 0xf) | 0xe0), //
                static_cast<char>(((cp >> 6) & 0x3f) | 0x80),
                static_cast<char>((cp & 0x3f) | 0x80) //
            };
            return bytes;
        }

        std::string candidates_to_braille_patterns(const CandidateMask candidates)
        {
            std::pair<std::uint8_t, std::uint8_t> values{};
            for (int i = 0; i < board_size; i++)
                if (candidates & (1 << i))
                {
                    const auto [second, added] = candidate_braille_positions[i];
                    (second ? values.second : values.first) += added;
                }
            return braille_dots(values.first) + braille_dots(values.second);
        }
    } // namespace

    void Board::set_number_at(const int num, const PatternMask mask) noexcept
    {
        const CandidateMask bit = 1 << num;
        for (int i = 0; i < cell_count; i++)
            if (mask.test(i))
                cells[i] = bit;
        filled |= mask;
    }

    PatternMask Board::pattern_of(const int num) const noexcept
    {
        PatternMask res;
        const CandidateMask bit = 1 << num;
        for (int i = 0; i < cell_count; i++)
            res.set(i, (cells[i] & bit) != 0);
        return res;
    }

    Board Board::empty_board() noexcept
    {
        Board board;
        for (auto& cell : board.cells)
            cell = full_mask;
        return board;
    }

    Board Board::random_filled_board() noexcept
    {
        if constexpr (board_size > 9) // Fallback for larger puzzles
        {
            auto board = empty_board();
            board.brute_force_solve(1, true);
            return board;
        }

        Board board;
        board.filled.set();
        Board backtrack;
        int cur = 0;
        while (cur < cell_count)
        {
            backtrack.cells[cur] = full_mask;
            const int r = cur / board_size, c = cur % board_size;
            CandidateMask& cell = backtrack.cells[cur];
            for (int i = 0; i < r; i++)
                cell &= ~board.at(i, c);
            for (int i = 0; i < c; i++)
                cell &= ~board.at(r, i);
            const int br = r / box_height * box_height;
            const int bc = c / box_width * box_width;
            for (int i = br; i < r; i++)
                for (int j = bc; j < bc + box_width; j++)
                    cell &= ~board.at(i, j);
            while (backtrack.cells[cur] == 0)
            {
                cur--;
                backtrack.cells[cur] &= ~board.cells[cur];
            }
            board.cells[cur] = get_random_bit(backtrack.cells[cur]);
            cur++;
        }
        return board;
    }

    Board Board::from_repr(const std::string_view str)
    {
        if (str.size() != cell_count)
            throw std::runtime_error("Representation and board size don't match");
        Board board = empty_board();
        for (int i = 0; i < cell_count; i++)
            if (str[i] != '.')
            {
                board.cells[i] = from_char_repr(str[i]);
                board.filled.set(i);
            }
        return board;
    }

    Board Board::from_full_repr(const std::string_view str)
    {
        Board res;
        std::size_t si = 0;
        for (int i = 0; i < cell_count; i++)
        {
            if (si >= str.size())
                throw std::runtime_error("Too few cells in the string");
            if (str[si] != '(') // Filled
            {
                res.filled.set(i);
                res.cells[i] = from_char_repr(str[si]);
            }
            else
            {
                si++;
                while (si < str.size() && str[si] != ')')
                {
                    res.cells[i] |= from_char_repr(str[si]);
                    si++;
                }
                if (si == str.size())
                    throw std::runtime_error("Parenthesis not closed");
            }
            si++;
        }
        if (si != str.size())
            throw std::runtime_error("Too many cells in the string");
        return res;
    }

    bool Board::reduce_candidates_from_naked_single(const int r, const int c) noexcept
    {
        const auto mask = ~at(r, c);
        for (int i = 0; i < board_size; i++)
        {
            if (i != r && !filled_at(i, c)) // Same column
                if ((at(i, c) &= mask) == 0) // Contradiction
                    return false;
            if (i != c && !filled_at(r, i)) // Same row
                if ((at(r, i) &= mask) == 0) // Contradiction
                    return false;
        }
        const int br = r / box_height * box_height, bc = c / box_width * box_width;
        for (int i = br; i < br + box_height; i++)
        {
            if (i == r)
                continue;
            for (int j = bc; j < bc + box_width; j++)
                if (j != c && !filled_at(i, j)) // Same box
                    if ((at(i, j) &= mask) == 0) // Contradiction
                        return false;
        }
        return true;
    }

    bool Board::reduce_candidates() noexcept
    {
        for (int i = 0; i < cell_count; i++)
        {
            if (!filled[i])
                continue;
            if (!reduce_candidates_from_naked_single(i))
                return false;
        }
        return true;
    }

    int Board::brute_force_solve(const int max_solutions, const bool randomized)
    {
        const auto [solution, count] = BruteForceSolver(*this, max_solutions, randomized).solve();
        *this = solution;
        return count;
    }

    void Board::print(const bool display_candidates_with_braille_dots) const
    {
        const auto print_row_sep = []
        {
            for (int i = 0; i < box_height; i++)
                fmt::print("+{:-<{}}", "", 2 * box_width + 1);
            fmt::println("+");
        };
        for (int i = 0; i < board_size; i++)
        {
            if (i % box_height == 0)
                print_row_sep();
            for (int j = 0; j < board_size; j++)
            {
                if (j % box_width == 0)
                    fmt::print("| ");
                if (filled_at(i, j))
                    fmt::print("{} ", to_char_repr(at(i, j)));
                else if (display_candidates_with_braille_dots)
                    fmt::print("{}", candidates_to_braille_patterns(at(i, j)));
                else
                    fmt::print(". ");
            }
            fmt::println("|");
        }
        print_row_sep();
    }

    std::string Board::repr() const
    {
        std::string res(cell_count, '.');
        for (int i = 0; i < cell_count; i++)
            if (filled[i])
                res[i] = to_char_repr(cells[i]);
        return res;
    }

    std::string Board::full_repr() const
    {
        std::string res;
        res.reserve(cell_count);
        for (int i = 0; i < cell_count; i++)
            if (filled[i])
                res.push_back(to_char_repr(cells[i]));
            else
            {
                res.push_back('(');
                for (int j = 0; j < board_size; j++)
                    if (const CandidateMask bit = 1 << j; bit & cells[i])
                        res.push_back(to_char_repr(bit));
                res.push_back(')');
            }
        return res;
    }
} // namespace sltd
