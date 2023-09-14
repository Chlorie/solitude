#pragma once

#include <optional>
#include <vector>

#include "../board.h"

SOLITUDE_SUPPRESS_EXPORT_WARNING

namespace sltd
{
    struct SOLITUDE_API ChainNode
    {
        enum Type : uint8_t
        {
            normal,
            group
        };

        CandidateMask line_mask = 0; // Bit i is set for cell #i in house #idx
        CandidateMask box_mask = 0; // Bit i is set for cell #i in box #box_idx
        std::uint8_t idx = 0; // Cell index for normal nodes, line house index for group nodes
        std::uint8_t box_idx = 0; // Box index for group nodes
        std::uint8_t candidate = 0; // The candidate number in question
        Type type : 4 = normal;
        bool set : 4 = false; // Whether the candidate is set or not

        constexpr friend bool operator==(ChainNode, ChainNode) noexcept = default;
        friend bool has_same_cells(ChainNode lhs, ChainNode rhs) noexcept;
    };
    static_assert(sizeof(std::uint64_t) == sizeof(ChainNode) && //
        std::has_unique_object_representations_v<ChainNode>); // To ensure the cast to uint64 is unique

    struct SOLITUDE_API IntRange
    {
        int min = 0;
        int max = board_size * cell_count;
    };

    struct SOLITUDE_API XChain
    {
        static constexpr std::string_view name = "X-Chain";

        std::vector<int> node_idx;
        PatternMask eliminations;
        CandidateMask candidate = 0;

        std::string description() const;
        static std::optional<XChain> try_find(const Board& board, IntRange length);
        void apply_to(Board& board) const;
    };

    struct SOLITUDE_API XYChain
    {
        static constexpr std::string_view name = "XY-Chain";

        std::vector<int> node_idx;
        PatternMask eliminations;
        CandidateMask candidate = 0;

        std::string description() const;
        static std::optional<XYChain> try_find(const Board& board, IntRange length);
        void apply_to(Board& board) const;
    };

    struct SOLITUDE_API Chain
    {
        static constexpr std::string_view name = "Chain";

        std::vector<ChainNode> nodes;
        PatternMask eliminations[board_size];

        std::string description() const;
        static std::optional<Chain> try_find(const Board& board, std::size_t max_length);
        void apply_to(Board& board) const;
    };
} // namespace sltd

template <>
struct std::hash<sltd::ChainNode>
{
    std::size_t operator()(const sltd::ChainNode node) const noexcept
    {
        const auto uint = std::bit_cast<std::uint64_t>(node);
        return std::hash<std::uint64_t>{}(uint);
    }
};

SOLITUDE_RESTORE_EXPORT_WARNING
