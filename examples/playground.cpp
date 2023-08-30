#include <chrono>
#include <algorithm>
#include <bitset>
#include <fstream>
#include <filesystem>

#include <solitude/board.h>
#include <solitude/generator.h>
#include <solitude/strategies.h>
#include <solitude/utils.h>

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
            // current_ = Board::from_full_repr(
            //     "(13)496(37)2(17)852(38)7(358)(358)1694(1568)(68)(156)9(478)(48)23(17)(3689)5(1346)(348)27(34)(146)(89)"
            //     "(3689)72(348)1(389)5(46)(89)(1389)(389)(134)(3458)6(3589)(134)72(359)281(3459)6(47)(45)(37)7(369)(356)"
            //     "2(3459)(345)8(145)(136)41(356)7(358)(358)92(36)");
            current_solved_ = current_;
            (void)current_solved_.brute_force_solve(1);
            while (case_count_ < target_ && !current_.filled.all())
            {
                if (solve_with<NakedSingle>() || //
                    solve_with<HiddenSubset>(1) || //
                    solve_with<Intersection>() || //
                    solve_with<NakedSubset>(2) || //
                    solve_with<HiddenSubset>(2) || //
                    solve_with<Fish>(2, false) || // X-wing
                    solve_with<NakedSubset>(3) || //
                    solve_with<HiddenSubset>(3) || //
                    solve_with<Fish>(2, true) || // Finned X-wing
                    solve_with<Fish>(3, false) || // Swordfish
                    solve_with<Fish>(3, true) || // Finned Swordfish
                    solve_with<NakedSubset>(4) || //
                    solve_with<HiddenSubset>(4) || //
                    solve_with<Fish>(4, false) || // Jellyfish
                    solve_with<Fish>(4, true) // Finned Jellyfish
                )
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
                    if (std::pair{args...}.first >= 4)
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
    constexpr std::string_view puzzle = "(47)(56)(56)(478)19(37)2(38)(12)3(1278)6(78)594"
                                        "(18)9(147)(178)2(3478)(37)(17)65(13)(15)(135)"
                                        "(89)(89)2476(47)(467)(67)351289829(47)(47)6(13)"
                                        "5(13)58(37)(79)(2379)461(27)6941(27)853(27)(123)"
                                        "(17)(1237)56(37)894";
    const auto board = Board::from_full_repr(puzzle);
    board.print(true);
    if (const auto opt = Fish::try_find(board, 4, false))
        fmt::println("{}", opt->description());
    else
        fmt::println("Didn't find anything");
}

void generate_test()
{
    const std::filesystem::path path = "test_cases/test.txt";
    TestCaseFinder finder{path, 10'000, true, true};
    finder.run();
}

void run_test()
{
    const std::filesystem::path path = "test_cases/finned_swordfish.txt";
    Tester tester(path);
    tester.test<Fish>(3, true);
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
