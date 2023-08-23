#include <chrono>
#include <algorithm>
#include <bitset>
#include <clu/static_vector.h>
#include <clu/random.h>
#include <fmt/format.h>
#include <fmt/chrono.h>

#include <solitude/board.h>

using namespace std::chrono_literals;
using namespace sltd;

template <typename F>
auto test_time(F&& func)
{
    constexpr std::size_t warm_up = 5;
    constexpr std::size_t iters = 100;
    for (std::size_t i = 0; i < warm_up; i++)
        func();
    std::chrono::nanoseconds total{};
    for (std::size_t i = 0; i < iters; i++)
    {
        const auto begin = std::chrono::steady_clock::now();
        func();
        const auto end = std::chrono::steady_clock::now();
        total += end - begin;
        // if (end - begin > 5ms)
        //     fmt::println("Iteration {}: {}", i + 1, end - begin);
    }
    return total / iters;
}

enum class SymmetryType
{
    none,
    centrosymmetric,
    axisymmetric
};

Board minimal_puzzle(const SymmetryType symmetry, bool another)
{
    auto current = Board::random_filled_board();
    clu::static_vector<std::pair<int, int>, cell_count> pairs;
    using enum SymmetryType;
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
        default: fmt::println("Fuck"); return {};
    }
    std::ranges::shuffle(pairs, clu::thread_rng());
    while (!pairs.empty())
    {
        const auto pair = pairs.back();
        pairs.pop_back();
        auto board = current;
        board.cells[pair.first] = board.cells[pair.second] = full_mask;
        if ((another ? board.brute_force_solve2(2) : board.brute_force_solve(2)) == 1)
            current.cells[pair.first] = current.cells[pair.second] = full_mask;
    }
    return current;
}

int main()
{
    using enum SymmetryType;
    {
        auto board = minimal_puzzle(centrosymmetric, true);
        board.print();
        fmt::println("");
    }
    // auto board = minimal_puzzle(centrosymmetric);
    // board.print();
    // fmt::println("");
    // board.brute_force_solve();
    // board.print();
    // return 0;

    fmt::println("Generate randomly filled puzzle:");
    {
        const auto t = test_time([] { (void)Board::random_filled_board(); });
        fmt::println("Filling a board known to be empty: {:8.3f}μs", t / 1.0us);
    }
    {
        const auto t = test_time(
            []
            {
                auto board = Board::empty_board();
                board.brute_force_solve(1, true);
            });
        fmt::println("Top left to bottom right:          {:8.3f}μs", t / 1.0us);
    }
    {
        const auto t = test_time(
            []
            {
                auto board = Board::empty_board();
                board.brute_force_solve2(1, true);
            });
        fmt::println("Prioritizing least candidates:     {:8.3f}μs", t / 1.0us);
    }

    fmt::println("");

    for (std::size_t i = 0; i < 100; i++)
        fmt::println("{}", minimal_puzzle(centrosymmetric, true).repr());

    const std::pair<SymmetryType, const char*> syms[]{
        {centrosymmetric, "Centrosymmetric"},
        {axisymmetric, "Axisymmetric"},
        {none, "No symmetry"},
    };
    for (const auto sym : syms)
    {
        fmt::println("{}:", sym.second);
        {
            const auto t = test_time([=] { (void)minimal_puzzle(sym.first, false); });
            fmt::println("Top left to bottom right:      {:8.3f}ms", t / 1.0ms);
        }
        {
            const auto t = test_time([=] { (void)minimal_puzzle(sym.first, true); });
            fmt::println("Prioritizing least candidates: {:8.3f}ms", t / 1.0ms);
        }
    }
    return 0;
}
