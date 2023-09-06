#include <chrono>
#include <algorithm>
#include <bitset>
#include <fstream>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/chrono.h>

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
            // current_ =
            //     Board::from_repr("...86...7....752..3..2...8..57....42..2...3..63....85..9...2..6..351....4...96...");
            // current_ =
            //     Board::from_full_repr("3248615976579438219185273642(6789)143(589)(79)(578)(5689)5(789)32(18)6(1479)("
            //                           "478)(89)4(689)(69)7(158)(589)(129)(358)(235689)1(349)(259)6(58)(258)(2479)("
            //                           "34578)(23589)8(39)(259)1746(35)(2359)7(46)(256)39(258)(24)1(258)");
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
                    solve_with<XYWing>() || //
                    solve_with<XYZWing>() || //
                    solve_with<WWing>() || //
                    solve_with<Fish>(3, false) || // Swordfish
                    solve_with<Fish>(3, true) || // Finned Swordfish
                    solve_with<XChain>(IntRange{.max = 3}) || // Turbot fish
                    solve_with<XYChain>(IntRange{.max = 3}) || //
                    solve_with<NakedSubset>(4) || //
                    solve_with<HiddenSubset>(4) || //
                    solve_with<RemotePair>() || //
                    solve_with<SimpleColors>() || //
                    solve_with<Fish>(4, false) || // Jellyfish
                    solve_with<Fish>(4, true) || // Finned Jellyfish
                    solve_with<XChain>(IntRange{.min = 5, .max = 5}) || //
                    solve_with<XYChain>(IntRange{.min = 4, .max = 5}) || //
                    solve_with<SueDeCoq>(false) || // Basic SdC
                    solve_with_target<SueDeCoq>(true) || // Extended SdC
                    solve_with<XChain>(IntRange{.min = 7}) || //
                    solve_with<XYChain>(IntRange{.min = 6}) //
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
                if constexpr (std::is_same_v<Solver, SueDeCoq>)
                    //    if (std::pair{args...}.first >= 4)
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
        fmt::println("Time: {:8.3f}±{:8.3f}μs - {:7} iter ({:6.1f}μs ~ {:6.1f}μs)", //
            t.mean / 1.0us, t.stddev / 1.0us, t.iterations, t.min / 1.0us, t.max / 1.0us);
    }

    template <typename Solver, typename... Args>
    void show_solutions(Args&&... args)
    {
        fmt::print("\x1b[H\x1b[0J");
        for (const auto& board : boards_)
            if (const auto opt = Solver::try_find(board, args...); opt)
            {
                fmt::print("\x1b[H");
                board.print(true);
                fmt::println("\x1b[0J{}\n", opt->description());
                (void)std::getchar();
            }
            else
            {
                fmt::println("Failed regression test!\nBoard representation: {}", board.full_repr());
                throw std::runtime_error("Regression test failed");
            }
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

void generate_test() { TestCaseFinder("test_cases/test.txt", 1'000, false, true).run(); }

void run_test()
{
    // Tester("test_cases/extended_sue_de_coq.txt").show_solutions<SueDeCoq>(true); return;
    Tester("test_cases/finned_xwing.txt").test<Fish>(2, true);
    Tester("test_cases/finned_swordfish.txt").test<Fish>(3, true);
    Tester("test_cases/finned_jellyfish.txt").test<Fish>(4, true);
    Tester("test_cases/turbot_fish.txt").test<XChain>(IntRange{.max = 3});
    Tester("test_cases/xy_wing.txt").test<XYWing>();
    Tester("test_cases/xyz_wing.txt").test<XYZWing>();
    Tester("test_cases/w_wing.txt").test<WWing>();
    Tester("test_cases/basic_sue_de_coq.txt").test<SueDeCoq>(false);
    Tester("test_cases/extended_sue_de_coq.txt").test<SueDeCoq>(true);
}

int main()
try
{
    // debug();
    // generate_test();
    run_test();
}
catch (const std::exception& e)
{
    fmt::println("Caught exception: {}", e.what());
}
