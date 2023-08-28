#include <chrono>
#include <algorithm>
#include <bitset>
#include <fstream>
#include <filesystem>

#include <solitude/board.h>
#include <solitude/generator.h>
#include <solitude/strategies.h>

#include "benchmark.h"

using namespace std::chrono_literals;
using namespace sltd;

class TestCaseFinder
{
public:
    explicit TestCaseFinder( //
        const std::filesystem::path& path, //
        const std::size_t target = 10'000, //
        const bool show_steps = false, //
        const bool check_solver_correctness = false):
        file_(path),
        target_(target), show_steps_(show_steps), check_solver_correctness_(check_solver_correctness)
    {
    }

    void run()
    {
        const auto begin = bench::clock::now();
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
                if (solve_with<Intersection>())
                    continue;
                if (solve_with<NakedSubset>(2))
                    continue;
                if (solve_with<HiddenSubset>(2))
                    continue;
                if (solve_with<Fish>(2))
                    continue;
                if (solve_with<NakedSubset>(3))
                    continue;
                if (solve_with<HiddenSubset>(3))
                    continue;
                if (solve_with<NakedSubset>(4))
                    continue;
                if (solve_with<HiddenSubset>(4))
                    continue;
                if (solve_with<Fish>(3))
                    continue;
                if (show_steps_)
                {
                    fmt::print("\x1b[H");
                    current_.print(true);
                    fmt::println("\x1b[0JCan't solve using given strategies");
                    // (void)std::getchar();
                }
                break;
            }
            puzzle_count_++;
            if (show_steps_)
                continue;
            if (const auto now = bench::clock::now(); //
                case_count_ == target_ || now - last_update >= 2s)
            {
                last_update = now;
                fmt::println("[{:5}s] Generated {:7} puzzles, found {:6} test cases", //
                    (now - begin) / 1s, puzzle_count_, case_count_);
            }
        }
    }

private:
    std::ofstream file_;
    std::size_t target_ = 0;
    bool show_steps_ = false;
    bool check_solver_correctness_ = false;
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
            if (show_steps_)
            {
                fmt::print("\x1b[H");
                current_.print(true);
                fmt::println("\x1b[0J{}\n", opt->description());
                if constexpr (std::is_same_v<Solver, Fish>)
                    if ((args, ...) == 3)
                        (void)std::getchar();
            }
            if (check_solver_correctness_)
            {
                const Board before_apply = current_;
                opt->apply_to(current_);
                for (int i = 0; i < cell_count; i++)
                    if (const auto solution = current_solved_.cells[i]; //
                        (solution & current_.cells[i]) != solution)
                    {
                        fmt::println("The solver is faulty\nSolver name: {}\nBoard state: {}", //
                            Solver::name, before_apply.full_repr());
                        throw std::runtime_error("The solver is faulty");
                    }
            }
            else
                opt->apply_to(current_);
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
    constexpr std::string_view puzzle = "951643(28)(28)74721893566837524193(49)5"
                                        "(489)(79)6(278)(289)17(19)8(59)2(15)634"
                                        "2(149)63(179)(148)(78)(89)5124(58)6(58)"
                                        "973569237148837(49)(19)(14)562";
    const auto board = Board::from_full_repr(puzzle);
    board.print(true);
    if (const auto opt = Intersection::try_find(board))
        fmt::println("{}", opt->description());
    else
        fmt::println("Didn't find anything");
}

void generate_test()
{
    const std::filesystem::path path = "test_cases/vanilla_xwing.txt";
    TestCaseFinder finder{path, 1'000, false, true};
    finder.run();
}

void run_test()
{
    const std::filesystem::path path = "test_cases/naked_pair.txt";
    Tester tester(path);
    tester.test<NakedSubset>(2);
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
