#include "solitude/board.h"

#include <bit>
#include <array>
#include <stack>
#include <vector>
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
            throw std::runtime_error("Unknown character representation");
        }

        constexpr CandidateMask get_cell_candidates(const Board& current, const int idx) noexcept
        {
            const int r = idx / board_size, c = idx % board_size;
            CandidateMask cell = full_mask;
            for (int i = 0; i < board_size; i++)
            {
                if (i != r)
                {
                    if (const CandidateMask other = current.at(i, c); std::has_single_bit(other))
                        cell &= ~other;
                }
                if (i != c)
                {
                    if (const CandidateMask other = current.at(r, i); std::has_single_bit(other))
                        cell &= ~other;
                }
            }
            const int br = r / box_height * box_height;
            const int bc = c / box_width * box_width;
            for (int i = br; i < r; i++)
            {
                if (i == r)
                    continue;
                for (int j = bc; j < bc + box_width; j++)
                {
                    if (j == c)
                        continue;
                    if (const CandidateMask other = current.at(i, j); std::has_single_bit(other))
                        cell &= ~other;
                }
            }
            return cell;
        }

        using HorizontalBandMask = std::bitset<cell_count / box_height>;
        using ColumnMask = std::bitset<board_size>;

        struct BandPattern
        {
            HorizontalBandMask band;
            ColumnMask columns;
        };

        auto generate_band_patterns()
        {
            static constexpr int column_mask_count = []
            {
                int res = 1;
                for (int i = 0; i < box_height; i++)
                    res *= box_width;
                return res;
            }();

            static constexpr auto to_column_mask = [](int idx)
            {
                ColumnMask mask;
                std::array<int, box_height> column_indices;
                for (int i = 0; i < box_height; i++)
                {
                    column_indices[i] = idx % box_width + i * box_width;
                    idx /= box_width;
                }
                for (const int i : column_indices)
                    mask.set(i);
                return std::pair{mask, column_indices};
            };

            std::vector<BandPattern> res;
            for (int i = 0; i < column_mask_count; i++)
            {
                auto [column_mask, indices] = to_column_mask(i);
                std::array<int, box_height> perm;
                for (int j = 0; j < box_height; j++)
                    perm[j] = j;
                do
                {
                    HorizontalBandMask band_mask;
                    for (int j = 0; j < box_height; j++)
                        band_mask.set(j * board_size + indices[j]);
                    res.push_back({band_mask, column_mask});
                } while (std::ranges::next_permutation(indices).found);
            }
            return res;
        }

        class PatternGenerator
        {
        public:
            PatternGenerator(): bands_(generate_band_patterns())
            {
                while (find_next_index())
                    add_current_pattern();
            }

            auto get_result() && { return std::move(result_); }

        private:
            std::vector<PatternMask> result_;
            std::vector<BandPattern> bands_;
            int indices_[box_width]{-1};
            int band_idx_ = 0;

            bool find_next_index()
            {
                ColumnMask filled_columns;
                for (int i = 0; i < band_idx_; i++)
                    filled_columns |= bands_[indices_[i]].columns;
                for (int& i = ++indices_[band_idx_]; i < bands_.size(); i++)
                    if ((bands_[i].columns & filled_columns).none())
                    {
                        if (band_idx_ != box_width - 1)
                        {
                            indices_[++band_idx_] = -1;
                            find_next_index();
                        }
                        return true;
                    }
                return band_idx_-- == 0 ? false : find_next_index();
            }

            void add_current_pattern()
            {
                PatternMask mask;
                for (int i = 0; i < box_width; i++)
                    mask |= PatternMask(bands_[indices_[i]].band.to_ullong()) << (i * cell_count / box_height);
                result_.push_back(mask);
            }
        };

        class BruteForceSolver
        {
        public:
            BruteForceSolver(const Board& board, const int max_solutions, const bool randomized):
                max_solutions_(max_solutions), randomized_(randomized), solution_(board)
            {
            }

            std::pair<Board, int> solve()
            {
                solve_recurse(solution_, {});
                return {solution_, solution_count_};
            }

        private:
            int max_solutions_;
            bool randomized_;
            int solution_count_ = 0;
            Board solution_;

            void solve_recurse(Board state, PatternMask filled)
            {
                if (!exhaust_naked_singles(state, filled))
                    return;
                if (filled.all()) // New solution
                {
                    solution_count_++;
                    solution_ = state;
                    return;
                }
                // Find cell with the least candidates
                int min_candidates = board_size + 1, min_idx = 0;
                for (int i = 0; i < cell_count; i++)
                {
                    if (filled[i])
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
                    solve_recurse(state, filled);
                    if (solution_count_ >= max_solutions_)
                        return;
                    candidates &= ~bit;
                }
            }

            static bool exhaust_naked_singles(Board& state, PatternMask& filled)
            {
                while (true)
                {
                    bool exhausted = true;
                    for (int i = 0; i < cell_count; i++)
                        if (!filled[i] && std::has_single_bit(state.cells[i]))
                        {
                            exhausted = false;
                            filled.set(i);
                            const int r = i / board_size, c = i % board_size;
                            for (int j = 0; j < board_size; j++)
                            {
                                if (j != r && !filled[j * board_size + c]) // Same column
                                    if ((state.at(j, c) &= ~state.cells[i]) == 0) // Contradiction
                                        return false;
                                if (j != c && !filled[r * board_size + j]) // Same row
                                    if ((state.at(r, j) &= ~state.cells[i]) == 0) // Contradiction
                                        return false;
                            }
                            const int br = r / 3 * 3, bc = c / 3 * 3;
                            for (int j = br; j < br + 3; j++)
                            {
                                if (j == r)
                                    continue;
                                for (int k = bc; k < bc + 3; k++)
                                    if (k != c && !filled[j * board_size + k]) // Same box
                                        if ((state.at(j, k) &= ~state.cells[i]) == 0) // Contradiction
                                            return false;
                            }
                        }
                    if (exhausted)
                        return true;
                }
            }
        };
    } // namespace

    PatternMask Board::pattern_of(const int number) const noexcept
    {
        PatternMask res;
        const CandidateMask bit = 1 << number;
        for (int i = 0; i < cell_count; i++)
            res.set(i, (cells[i] & bit) != 0);
        return res;
    }

    void Board::apply_pattern(const int number, const PatternMask mask) noexcept
    {
        const CandidateMask bit = 1 << number;
        for (int i = 0; i < cell_count; i++)
            if (mask.test(i))
                cells[i] = bit;
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
        Board board;
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
                board.cells[i] = from_char_repr(str[i]);
        return board;
    }

    int Board::brute_force_solve(const int max_solutions, const bool randomized)
    {
        const Board original = *this;
        Board solution = *this;
        int solution_count = 0;
        std::stack<CandidateMask, clu::static_vector<CandidateMask, cell_count>> stack;

        int idx = -1;
        while (true)
        {
            if (++idx == cell_count)
            {
                solution = *this;
                if (++solution_count == max_solutions)
                    return max_solutions;
                --idx;
            }
            else
            {
                const CandidateMask candidates = original.cells[idx] & get_cell_candidates(*this, idx);
                stack.push(candidates);
            }
            // Backtrack to the last point where we still had a choice
            while (!stack.empty() && stack.top() == 0)
            {
                cells[idx] = original.cells[idx];
                stack.pop();
                idx--;
            }
            // Stack exhausted, no more solutions
            if (stack.empty())
            {
                *this = solution;
                return solution_count;
            }
            // Try a candidate from the last choice and remove it from the stack top
            auto& choices = stack.top();
            const CandidateMask selected = randomized ? get_random_bit(choices) : get_rightmost_set_bit(choices);
            cells[idx] = selected;
            choices &= ~selected;
        }
    }

    int Board::brute_force_solve2(const int max_solutions, const bool randomized)
    {
        const auto [solution, count] = BruteForceSolver(*this, max_solutions, randomized).solve();
        *this = solution;
        return count;
    }

    void Board::print() const
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
                if (std::has_single_bit(at(i, j)))
                    fmt::print("{} ", to_char_repr(at(i, j)));
                else
                    fmt::print("- ");
            }
            fmt::println("|");
        }
        print_row_sep();
    }

    std::string Board::repr() const
    {
        std::string res(cell_count, '.');
        for (int i = 0; i < cell_count; i++)
            if (std::has_single_bit(cells[i]))
                res[i] = to_char_repr(cells[i]);
        return res;
    }
} // namespace sltd
