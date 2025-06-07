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

#include "position.h"
#include "move.h"

std::string get_move_string(Move move, bool flipped)
{
    int from = from_sq(move),
        to = to_sq(move);

    if (flipped)
    {
        from ^= 56;
        to ^= 56;
    }

    std::string move_string;
    move_string.push_back('a' + file_of(from));
    move_string.push_back('1' + rank_of(from));


    move_string.push_back('a' + file_of(to));
    move_string.push_back('1' + rank_of(to));

    int prom = prom_type(move);
    switch (prom) {
    case QUEEN:
        move_string.push_back('q');
        break;
    case KNIGHT:
        move_string.push_back('n');
        break;
    case BISHOP:
        move_string.push_back('b');
        break;
    case ROOK:
        move_string.push_back('r');
        break;
    default:
        break;
    }
    return move_string;
}

bool Position::legal_move(Move move) const
{
    int from = from_sq(move);
    int ksq = this->position_of(KING, US);
    if (from == ksq)
        return !(this->attackers_to(to_sq(move), THEM));
    else
        return !(this->pinned(US) & BB(from))
             || (BB(to_sq(move)) & lookups::full_ray(from, ksq));
}

void Position::make_null_move()
{
    this->inc_half_moves();
    this->ep_sq = INVALID_SQ;
    this->flip();
    this->prev_hash_keys.push_back(this->hash_key);
    this->hash_key = this->calc_hash();
}

void Position::make_move(Move move)
{
    int from = from_sq(move),
        to = to_sq(move);

    if (this->ep_sq != INVALID_SQ)
        this->ep_sq = INVALID_SQ;

    if (this->check_piece_on(from, PAWN))
    {
        this->reset_half_moves();
        this->clear_prev_hash_keys();
    }
    else
    {
        this->prev_hash_keys.push_back(this->hash_key);
        this->inc_half_moves();
    }

    switch (move & MOVE_TYPE_MASK) {
        case NORMAL:
            this->move_piece(from, to, this->piece_on(from), US);
            break;
        case CAPTURE:
            this->remove_piece(to, this->piece_on(to), THEM);
            this->move_piece(from, to, this->piece_on(from), US);
            this->reset_half_moves();
            this->clear_prev_hash_keys();
            break;
        case PROM_CAPTURE:
            this->remove_piece(to, this->piece_on(to), THEM);
            this->remove_piece(from, PAWN, US);
            this->put_piece(to, prom_type(move), US);
            break;
        case PROMOTION:
            this->remove_piece(from, PAWN, US);
            this->put_piece(to, prom_type(move), US);
            break;
        default:
            std::cout << "MOVE TYPE ERROR!" << std::endl;
            break;
    }

    this->flip();
    this->hash_key = this->calc_hash();
}
