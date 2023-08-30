#pragma once

#include <chrono>
#include <cstddef>

namespace bench
{
    using namespace std::chrono_literals;
    using clock = std::chrono::steady_clock;
    using duration = clock::duration;

    struct iteration_result
    {
        std::size_t iterations;
        duration mean;
        duration stddev;
        duration min;
        duration max;
    };

    template <typename F>
    duration time_once(F&& func)
    {
        const auto begin = clock::now();
        func();
        const auto end = clock::now();
        return end - begin;
    }

    template <typename F>
    iteration_result time_iterations(F&& func)
    {
        static constexpr std::size_t max_warm_up = 10;
        static constexpr auto max_warm_up_time = 1s;
        static constexpr std::size_t min_iterations = 5;
        static constexpr std::size_t max_iterations = 1'000'000;
        static constexpr auto max_benchmark_time = 10s;

        duration warm_up_total{};
        for (std::size_t i = 0; i < max_warm_up; i++)
        {
            if (const duration time = bench::time_once(func); //
                (warm_up_total += time) >= max_warm_up_time)
                break;
        }

        duration total{};
        duration min_it_time = duration::max();
        duration max_it_time = duration::min();
        double sum_t2 = 0.0;
        std::size_t iter = 0;
        for (; iter < max_iterations; iter++)
        {
            const duration time = bench::time_once(func);
            const auto count = static_cast<double>(time.count());
            total += time;
            sum_t2 += count * count;
            min_it_time = std::min(min_it_time, time);
            max_it_time = std::max(max_it_time, time);
            if (iter + 1 >= min_iterations && total >= max_benchmark_time)
                break;
        }

        const double mean_d = static_cast<double>(total.count()) / static_cast<double>(iter);
        const double var_d = sum_t2 / static_cast<double>(iter) - mean_d * mean_d;
        return {
            .iterations = iter,
            .mean = duration{static_cast<duration::rep>(mean_d)},
            .stddev = duration{static_cast<duration::rep>(std::sqrt(var_d))},
            .min = min_it_time,
            .max = max_it_time //
        };
    }
} // namespace bench
