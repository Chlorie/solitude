#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <clu/random.h>
#include <clu/integer_literals.h>
#include <clu/chrono_utils.h>

#include <solitude/generator.h>
#include <solitude/strategies.h>

inline constexpr std::size_t difficulty_count = 7;

class PuzzleDifficultyLabeler
{
public:
    explicit PuzzleDifficultyLabeler(const sltd::Board& board): board_(board) {}

    std::size_t calculate_difficulty()
    {
        using namespace sltd;
        using namespace clu::literals;
        while (!board_.filled.all())
        {
            if (solve_with<NakedSingle>(true) || // Full house
                solve_with<HiddenSingle>(true)) // Hidden single in box
                continue;
            diff_ = std::max(diff_, 1_uz);
            if (solve_with<HiddenSingle>() || // Hidden single
                solve_with<NakedSingle>() || //
                solve_with<HiddenSubset>(1) || //
                solve_with<Intersection>())
                continue;
            diff_ = std::max(diff_, 2_uz);
            if (solve_with<NakedSubset>(2) || //
                solve_with<HiddenSubset>(2) || //
                solve_with<Fish>(2, false))
                continue;
            diff_ = std::max(diff_, 3_uz);
            if (solve_with<NakedSubset>(3) || //
                solve_with<HiddenSubset>(3) || //
                solve_with<Fish>(2, true) || // Finned X-wing
                solve_with<XYWing>() || //
                solve_with<XYZWing>() || //
                solve_with<WWing>() || //
                solve_with<Fish>(3, false) || // Swordfish
                solve_with<Fish>(3, true) || // Finned Swordfish
                solve_with<XChain>(IntRange{.max = 3}) || // Turbot fish
                solve_with<XYChain>(IntRange{.max = 3}))
                continue;
            diff_ = std::max(diff_, 4_uz);
            if (solve_with<NakedSubset>(4) || //
                solve_with<HiddenSubset>(4) || //
                solve_with<RemotePair>() || //
                solve_with<SimpleColors>() || //
                solve_with<Fish>(4, false) || // Jellyfish
                solve_with<Fish>(4, true) || // Finned Jellyfish
                solve_with<XChain>(IntRange{.min = 5, .max = 5}) || //
                solve_with<XYChain>(IntRange{.min = 4, .max = 5}))
                continue;
            diff_ = std::max(diff_, 5_uz);
            if (solve_with<XChain>(IntRange{.min = 7}) || //
                solve_with<XYChain>(IntRange{.min = 6}) || //
                solve_with<Chain>(1'024) || //
                solve_with<SueDeCoq>(false) || //
                solve_with<AlsXZ>())
                continue;
            return difficulty_count - 1;
        }
        return diff_;
    }

private:
    std::size_t diff_ = 0;
    sltd::Board board_;

    template <typename Solver, typename... Args>
    bool solve_with(Args&&... args)
    {
        if (const auto opt = Solver::try_find(board_, static_cast<Args&&>(args)...))
        {
            opt->apply_to(board_);
            return true;
        }
        return false;
    }
};

class PuzzleGenerator
{
public:
    PuzzleGenerator( //
        const std::filesystem::path& path, const std::size_t target, const std::size_t num_worker_threads):
        path_(path),
        num_worker_threads_(num_worker_threads), target_(target)
    {
    }

    void run()
    {
        if (!exists(path_))
            create_directories(path_);
        workers_.reserve(num_worker_threads_);
        for (std::size_t i = 0; i < num_worker_threads_; i++)
            workers_.emplace_back([&] { work(); });
        logger();
        for (auto& thread : workers_)
            thread.join();
        for (std::size_t i = 0; i < difficulty_count; i++)
        {
            const auto file_path = path_ / fmt::format("{}.txt", difficulty_names[i]);
            log_time();
            fmt::println("Writing file {}", file_path.generic_string());
            std::ofstream file(file_path);
            for (auto board : puzzles_[i])
            {
                file << board.repr() << ' ';
                board.brute_force_solve(1);
                file << board.repr() << '\n';
            }
        }
        log_time();
        fmt::println("All done");
    }

private:
    static constexpr std::string_view difficulty_names[difficulty_count]{
        "trivial", "casual", "beginner", "intermediate", "advanced", "expert", "master" //
    };

    std::filesystem::path path_;
    std::size_t num_worker_threads_;
    std::size_t target_;
    std::size_t total_puzzles_;
    std::condition_variable cv_;
    std::mutex mut_;
    std::vector<std::thread> workers_;
    std::vector<sltd::Board> puzzles_[difficulty_count];

    void log_time() const
    {
        namespace chr = std::chrono;
        fmt::print("[{}] ", chr::time_point_cast<chr::seconds>(clu::local_now()));
    }

    bool generated_enough_puzzles() const
    {
        for (const auto& diff : puzzles_)
            if (diff.size() < target_)
                return false;
        return true;
    }

    void log_puzzle_counts() const
    {
        std::size_t counts[difficulty_count];
        for (std::size_t i = 0; i < difficulty_count; i++)
            counts[i] = puzzles_[i].size();
        log_time();
        fmt::println("Generated puzzle count by difficulty: {}", counts);
    }

    void logger()
    {
        using namespace std::literals;
        log_time();
        fmt::println("Starting puzzle generation with {} worker thread(s)", num_worker_threads_);
        {
            std::unique_lock lock(mut_);
            while (!cv_.wait_for(lock, 5s, [&] { return generated_enough_puzzles(); }))
                log_puzzle_counts();
            log_puzzle_counts();
        }
    }

    void work()
    {
        static constexpr std::size_t save_every = 1'000;
        std::vector<sltd::Board> puzzles[difficulty_count];
        for (std::size_t count = 0;; count++)
        {
            const auto [diff, board] = generate_one_puzzle();
            puzzles[diff].push_back(board);
            if ((count + 1) % save_every == 0)
            {
                std::unique_lock lock(mut_);
                for (std::size_t i = 0; i < difficulty_count; i++)
                {
                    if (puzzles_[i].size() < target_)
                        std::ranges::copy(puzzles[i], std::back_inserter(puzzles_[i]));
                    puzzles[i].clear();
                }
                if (generated_enough_puzzles())
                {
                    lock.unlock();
                    cv_.notify_all();
                    return;
                }
            }
        }
    }

    static std::pair<std::size_t, sltd::Board> generate_one_puzzle()
    {
        using enum sltd::SymmetryType;
        const auto sym = clu::randint(0, 1) == 0 ? centrosymmetric : axisymmetric;
        auto board = generate_minimal_puzzle(sym);
        return {PuzzleDifficultyLabeler(board).calculate_difficulty(), board};
    }
};

int main() { PuzzleGenerator("puzzles-20230914", 20'000, 20).run(); }
