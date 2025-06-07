/*
MIT License

Copyright (c) 2018 Manik Charan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "move.h"
#include "position.h"
#include "lookups.h"

static inline void add_move(int move, std::vector<Move>& mlist)
{
    mlist.push_back(move);
}

static inline void extract_quiets(int from, u64 bb, std::vector<Move>& mlist)
{
    while (bb) {
        add_move(get_move(from, fbitscan(bb), NORMAL), mlist);
        bb &= bb - 1;
    }
}

static inline void extract_captures(const Position& pos, int from, u64 bb, std::vector<Move>& mlist)
{
    while (bb) {
        int to = fbitscan(bb);
        add_move(get_move(from, to, CAPTURE, pos.piece_on(to)), mlist);
        bb &= bb - 1;
    }
}

void gen_piece_captures(const Position& pos, std::vector<Move>& mlist)
{
    int from;
    u64 occupancy = pos.occupancy_bb(),
        them = pos.color_bb(THEM);
    for (int pt = KING; pt >= KNIGHT; --pt) {
        u64 curr_pieces = pos.piece_bb(pt, US);
        while (curr_pieces) {
            from = fbitscan(curr_pieces);
            curr_pieces &= curr_pieces - 1;
            extract_captures(pos, from, lookups::attacks(pt, from, occupancy) & them, mlist);
        }
    }
}

void gen_pawn_captures(const Position& pos, std::vector<Move>& mlist) {
    int from;
    int cap_pt;
    u64 caps1, caps2, prom_caps1, prom_caps2;

    u64 pawns = pos.piece_bb(PAWN, US);

    caps1 = ((pawns & ~FILE_A_MASK) << 7) & pos.color_bb(THEM);
    prom_caps1 = caps1 & RANK_6_MASK;
    caps1 ^= prom_caps1;

    caps2 = ((pawns & ~FILE_H_MASK) << 9) & pos.color_bb(THEM);
    prom_caps2 = caps2 & RANK_6_MASK;
    caps2 ^= prom_caps2;

    int to;
    while (caps1) {
        to = fbitscan(caps1);
        caps1 &= caps1 - 1;
        add_move(get_move(to - 7, to, CAPTURE, pos.piece_on(to)), mlist);
    }
    while (caps2) {
        to = fbitscan(caps2);
        caps2 &= caps2 - 1;
        add_move(get_move(to - 9, to, CAPTURE, pos.piece_on(to)), mlist);
    }
    while (prom_caps1) {
        to = fbitscan(prom_caps1);
        prom_caps1 &= prom_caps1 - 1;
        cap_pt = pos.piece_on(to);
        add_move(get_move(to - 7, to, PROM_CAPTURE, cap_pt, PROM_TO_QUEEN), mlist);
        add_move(get_move(to - 7, to, PROM_CAPTURE, cap_pt, PROM_TO_KNIGHT), mlist);
        add_move(get_move(to - 7, to, PROM_CAPTURE, cap_pt, PROM_TO_BISHOP), mlist);
        add_move(get_move(to - 7, to, PROM_CAPTURE, cap_pt, PROM_TO_ROOK), mlist);
    }
    while (prom_caps2) {
        to = fbitscan(prom_caps2);
        prom_caps2 &= prom_caps2 - 1;
        cap_pt = pos.piece_on(to);
        add_move(get_move(to - 9, to, PROM_CAPTURE, cap_pt, PROM_TO_QUEEN), mlist);
        add_move(get_move(to - 9, to, PROM_CAPTURE, cap_pt, PROM_TO_KNIGHT), mlist);
        add_move(get_move(to - 9, to, PROM_CAPTURE, cap_pt, PROM_TO_BISHOP), mlist);
        add_move(get_move(to - 9, to, PROM_CAPTURE, cap_pt, PROM_TO_ROOK), mlist);
    }
}

void gen_quiet_promotions(const Position& pos, std::vector<Move>& mlist)
{
    u64 prom_destinations = ((pos.piece_bb(PAWN, US) & RANK_5_MASK) << 8) & ~pos.occupancy_bb();
    while (prom_destinations) {
        int to = fbitscan(prom_destinations);
        prom_destinations &= prom_destinations - 1;
        add_move(get_move(to - 8, to, PROMOTION, CAP_NONE, PROM_TO_QUEEN), mlist);
        add_move(get_move(to - 8, to, PROMOTION, CAP_NONE, PROM_TO_KNIGHT), mlist);
        add_move(get_move(to - 8, to, PROMOTION, CAP_NONE, PROM_TO_BISHOP), mlist);
        add_move(get_move(to - 8, to, PROMOTION, CAP_NONE, PROM_TO_ROOK), mlist);
    }
}


void gen_piece_quiets(const Position& pos, std::vector<Move>& mlist)
{
    u64 occupancy = pos.occupancy_bb();
    u64 vacancy = ~occupancy;
    for (int pt = KNIGHT; pt <= KING; ++pt) {
        u64 curr_pieces = pos.piece_bb(pt, US);
        while (curr_pieces) {
            int from = fbitscan(curr_pieces);
            curr_pieces &= curr_pieces - 1;
            extract_quiets(from, lookups::attacks(pt, from, occupancy) & vacancy, mlist);
        }
    }
}

void gen_pawn_quiets(const Position& pos, std::vector<Move>& mlist)
{
    u64 vacancy = ~pos.occupancy_bb();
    u64 single_pushes_bb = ((pos.piece_bb(PAWN, US) & ~RANK_5_MASK) << 8) & vacancy;
    while (single_pushes_bb) {
        int to = fbitscan(single_pushes_bb);
        single_pushes_bb &= single_pushes_bb - 1;
        add_move(get_move(to - 8, to, NORMAL), mlist);
    }
}

void gen_checker_captures(const Position& pos, u64 checkers, std::vector<Move>& mlist)
{
    u64 our_pawns = pos.piece_bb(PAWN, US);
    u64 non_king_mask = ~pos.piece_bb(KING);
    u64 occupancy = pos.occupancy_bb();

    while (checkers) {
        int checker_sq = fbitscan(checkers);
        int checker_pt = pos.piece_on(checker_sq);
        checkers &= checkers - 1;
        u64 attackers = pos.attackers_to(checker_sq, US, occupancy) & non_king_mask;
        while (attackers) {
            int attacker_sq = fbitscan(attackers);
            attackers &= attackers - 1;
            if (   (BB(attacker_sq) & our_pawns)
                && (BB(checker_sq) & RANK_6_MASK))
            {
                add_move(get_move(attacker_sq, checker_sq, PROM_CAPTURE,
                                  checker_pt, PROM_TO_QUEEN), mlist);
                add_move(get_move(attacker_sq, checker_sq, PROM_CAPTURE,
                                  checker_pt, PROM_TO_KNIGHT), mlist);
                add_move(get_move(attacker_sq, checker_sq, PROM_CAPTURE,
                                  checker_pt, PROM_TO_ROOK), mlist);
                add_move(get_move(attacker_sq, checker_sq, PROM_CAPTURE,
                                  checker_pt, PROM_TO_BISHOP), mlist);
            }
            else
            {
                add_move(get_move(attacker_sq, checker_sq, CAPTURE, checker_pt), mlist);
            }
        }
    }
}

void gen_check_blocks(const Position& pos, u64 blocking_possibilites, std::vector<Move>& mlist)
{
    u64 our_pawns = pos.piece_bb(PAWN, US);
    u64 inclusion_mask = ~(our_pawns | pos.piece_bb(KING) | pos.pinned(US));
    u64 occupancy = pos.occupancy_bb();
    u64 vacancy_mask = ~occupancy;

    while (blocking_possibilites) {
        int blocking_sq = fbitscan(blocking_possibilites);
        blocking_possibilites &= blocking_possibilites - 1;
        u64 pawn_blockers = BB(blocking_sq) >> 8;
        if (pawn_blockers & our_pawns)
        {
            int blocker_sq = fbitscan(pawn_blockers);
            if (BB(blocking_sq) & RANK_6_MASK)
            {
                add_move(get_move(blocker_sq, blocking_sq, PROMOTION,
                                  CAP_NONE, PROM_TO_QUEEN), mlist);
                add_move(get_move(blocker_sq, blocking_sq, PROMOTION,
                                  CAP_NONE, PROM_TO_KNIGHT), mlist);
                add_move(get_move(blocker_sq, blocking_sq, PROMOTION,
                                  CAP_NONE, PROM_TO_ROOK), mlist);
                add_move(get_move(blocker_sq, blocking_sq, PROMOTION,
                                  CAP_NONE, PROM_TO_BISHOP), mlist);
            }
            else
            {
                add_move(get_move(blocker_sq, blocking_sq, NORMAL), mlist);
            }
        }

        u64 candidate_blockers = pos.attackers_to(blocking_sq, US, occupancy)
                               & inclusion_mask;
        while (candidate_blockers) {
            add_move(get_move(fbitscan(candidate_blockers), blocking_sq,
                              NORMAL), mlist);
            candidate_blockers &= candidate_blockers - 1;
        }
    }
}

void Position::generate_in_check_movelist(std::vector<Move>& mlist) const
{
    int ksq = this->position_of(KING, US);
    u64 checkers = this->checkers_to(US);
    u64 evasions = lookups::king(ksq) & ~this->color_bb(US);

    u64 occupancy = this->occupancy_bb();
    u64 sans_king = occupancy ^ BB(ksq);

    while (evasions) {
        int sq = fbitscan(evasions);
        evasions &= evasions - 1;
        if (!this->attackers_to(sq, THEM, sans_king)) {
            if (occupancy & BB(sq))
                add_move(get_move(ksq, sq, CAPTURE, this->piece_on(sq)), mlist);
            else
                add_move(get_move(ksq, sq, NORMAL), mlist);
        }
    }

    if (checkers & (checkers - 1))
        return;

    gen_checker_captures(*this, checkers, mlist);

    if (checkers & lookups::king(ksq))
        return;

    u64 blocking_possibilites = lookups::intervening_sqs(fbitscan(checkers), ksq);
    if (blocking_possibilites)
        gen_check_blocks(*this, blocking_possibilites, mlist);
}

void Position::generate_quiesce_movelist(std::vector<Move>& mlist) const
{
    const Position& pos = *this;
    gen_piece_captures(pos, mlist);
    gen_pawn_captures(pos, mlist);
    gen_quiet_promotions(pos, mlist);
}

void Position::generate_movelist(std::vector<Move>& mlist) const
{
    const Position& pos = *this;
    gen_piece_captures(pos, mlist);
    gen_pawn_captures(pos, mlist);
    gen_quiet_promotions(pos, mlist);
    gen_piece_quiets(pos, mlist);
    gen_pawn_quiets(pos, mlist);
}

void Position::generate_legal_movelist(std::vector<Move>& mlist) const
{
    if (checkers_to(US))
        generate_in_check_movelist(mlist);
    else
        generate_movelist(mlist);
    for (std::size_t i = 0; i < mlist.size();) {
        if (!legal_move(mlist[i]))
            mlist.erase(mlist.begin() + i);
        else
            ++i;
    }
}
