#include <chrono>
#include <algorithm>
#include <bitset>
#include <fstream>
#include <filesystem>
#include <clu/static_vector.h>
#include <clu/random.h>

#include <solitude/board.h>
#include <solitude/generator.h>
#include <solitude/strategies.h>

#include "benchmark.h"

// [x] single
// [ ] intersection
// [x] pair
// [ ] x-wing
// [x] triple
// [ ] single digit 2-length chain, finned x-wing
// [ ] xy-wing, w-wing
// [ ] swordfish, jellyfish, finned or not
// [ ] x-chain, xy-chain
// [ ] arbitrary chains
// [ ] ALS
// [ ] more insane fish
// [ ] fish with chains
// [ ] forcing chains, forcing nets
// [ ] brute force

using namespace std::chrono_literals;
using namespace sltd;

class TestCaseFinder
{
public:
    explicit TestCaseFinder( //
        const std::filesystem::path& path, //
        const std::size_t target = 10'000, //
        const bool show_steps = false):
        file_(path),
        target_(target), show_steps_(show_steps)
    {
    }

    void run()
    {
        auto last_update = bench::clock::now();
        while (case_count_ < target_)
        {
            current_ = generate_minimal_puzzle(SymmetryType::centrosymmetric);
            current_solved_ = current_;
            (void)current_solved_.brute_force_solve(1);
            while (case_count_ < target_ && !current_.filled.all())
            {
                if (solve_with<NakedSingle>())
                    continue;
                if (solve_with<HiddenSubset>(1))
                    continue;
                if (solve_with<NakedSubset>(2))
                    continue;
                if (solve_with<HiddenSubset>(2))
                    continue;
                if (solve_with<NakedSubset>(3))
                    continue;
                if (solve_with<HiddenSubset>(3))
                    continue;
                if (solve_with<NakedSubset>(4))
                    continue;
                if (solve_with<HiddenSubset>(4))
                    continue;
                if (show_steps_)
                {
                    fmt::print("\x1b[H");
                    current_.print(true);
                    fmt::println("\x1b[0JCan't solve using given strategies");
                    (void)std::getchar();
                }
                break;
            }
            puzzle_count_++;
            if (show_steps_)
                continue;
            if (const auto now = bench::clock::now(); now - last_update >= 2s)
            {
                last_update = now;
                fmt::println("Generated {:6} puzzles, found {:6} test cases", puzzle_count_, case_count_);
            }
        }
    }

private:
    std::ofstream file_;
    std::size_t target_ = 0;
    bool show_steps_ = false;
    std::size_t puzzle_count_ = 0;
    std::size_t case_count_ = 0;
    Board current_;
    Board current_solved_;

    template <typename Solver, typename Func, typename... Args>
    bool solve_with_impl(Func&& upon_finding, Args&&... args)
    {
        if (const auto opt = Solver::try_find(current_, static_cast<Args&&>(args)...))
        {
            upon_finding();
            opt->apply_to(current_);
            for (int i = 0; i < cell_count; i++)
                if (const auto solution = current_solved_.cells[i]; //
                    (solution & current_.cells[i]) != solution)
                {
                    fmt::println("The solver is faulty");
                    throw std::runtime_error("The solver is faulty");
                }
            if (show_steps_)
            {
                fmt::print("\x1b[H");
                current_.print(true);
                fmt::println("\x1b[0J{}\n", opt->description());
                // if constexpr (std::is_same_v<Solver, NakedSubset>)
                //     if ((args, ...) == 4)
                //         (void)std::getchar();
            }
            return true;
        }
        return false;
    }

    template <typename Solver, typename... Args>
    bool solve_with(Args&&... args)
    {
        return this->solve_with_impl<Solver>([] {}, static_cast<Args&&>(args)...);
    }

    template <typename Solver, typename... Args>
    bool solve_with_target(Args&&... args)
    {
        return this->solve_with_impl<Solver>(
            [&]
            {
                case_count_++;
                file_ << current_.full_repr() << '\n';
            },
            static_cast<Args&&>(args)...);
    }
};

class Tester
{
public:
    explicit Tester(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        std::string line;
        while (!file.eof())
        {
            std::getline(file, line);
            if (line.empty())
                break;
            boards_.push_back(Board::from_full_repr(line));
        }
        fmt::println("Read {} puzzles from {}", boards_.size(), path.generic_string());
    }

    template <typename Solver, typename... Args>
    void test(Args&&... args)
    {
        const bench::iteration_result t = bench::time_iterations(
            [&, i = std::size_t{}]() mutable
            {
                if (const auto opt = Solver::try_find(boards_[i], args...); !opt)
                {
                    fmt::println("Failed regression test!\nBoard representation: {}", boards_[i].full_repr());
                    throw std::runtime_error("Regression test failed");
                }
                i = (i + 1) % boards_.size();
            });
        fmt::println("Time: {:8.3f}±{:8.3f}μs - {:7} iter ({:8.3f}μs ~ {:8.3f}μs)", //
            t.mean / 1.0us, t.stddev / 1.0us, t.iterations, t.min / 1.0us, t.max / 1.0us);
    }

private:
    std::vector<Board> boards_;
};

void debug()
{
    constexpr std::string_view puzzle = "824713965(137)69548(12)(17)(12)(17)(17)5269(18)349(178)"
                                        "(18)(16)3(47)(1568)2(18)6(18)39524(18)7(147)52(16)8(47)"
                                        "(16)(19)3246375(18)(189)(189)(13)(138)(18)49675(128)597821346";
    const auto board = Board::from_full_repr(puzzle);
    board.print();
    fmt::println("{}", NakedSubset::try_find(board, 2)->description());
}

void run_test()
{
    const std::filesystem::path path = "test_cases/naked_pair.txt";
    Tester tester(path);
    tester.test<NakedSubset>(2);
}

void generate_test()
{
    const std::filesystem::path path = "test_cases/naked_triple.txt";
    TestCaseFinder finder{path, 10'000, true};
    finder.run();
}

int main()
try
{
    // debug();
    // run_test();
    generate_test();
    return 0;
}
catch (const std::exception& e)
{
    fmt::println("Caught exception: {}", e.what());
}
