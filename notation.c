#include <stdio.h>
#include <string.h>

#include "dobutsu.h"

/*
 * This table returns for each square number the corresponding offset in
 * a position string.
 */
static const unsigned char coordinates[SQUARE_COUNT] = {
	16, 15, 14, 12, 11, 10, 8, 7, 6, 4, 3, 2
};

/*
 * Piece names used for display, render, and memchr();
 */
static const char sente_pieces[PIECE_COUNT] = {
	[CHCK_S] = 'C',
	[CHCK_G] = 'C',
	[GIRA_S] = 'G',
	[GIRA_G] = 'G',
	[ELPH_S] = 'E',
	[ELPH_G] = 'E',
	[LION_S] = 'L',
	[LION_G] = 'L'  /* should not happen */
}, gote_pieces[PIECE_COUNT] = {
	[CHCK_S] = 'c',
	[CHCK_G] = 'c',
	[GIRA_S] = 'g',
	[GIRA_G] = 'g',
	[ELPH_S] = 'e',
	[ELPH_G] = 'e',
	[LION_S] = 'l', /* should not happen */
	[LION_G] = 'l'
}, sg_pieces[PIECE_COUNT] = {
	[CHCK_S] = 'C',
	[CHCK_G] = 'c',
	[GIRA_S] = 'G',
	[GIRA_G] = 'g',
	[ELPH_S] = 'E',
	[ELPH_G] = 'e',
	[LION_S] = 'L',
	[LION_G] = 'l',
};

/*
 * Square names used for displaying moves.
 */
const char squares[12][2] = {
	"c4", "b4", "a4",
	"c3", "b3", "a3",
	"c2", "b2", "a2",
	"c1", "b1", "a1"
};

/*
 * Return the letter used to represent piece pc in pos.
 */
static char
piece_letter(size_t pc, const struct position *p)
{
	if (is_promoted(pc, p))
		return (p->pieces[pc] & GOTE_PIECE ? 'r' : 'R');
	else
		return (p->pieces[pc] & GOTE_PIECE ? gote_pieces[pc] : sente_pieces[pc]);
}

/*
 * Draw a nice graphical representation of a game position.  For
 * example, the position G/l-R/-e-/-C-/L--/Geg is represented as
 *
 *      ABC
 *     +---+
 *    1|l R| *eg
 *    2| e |
 *    3| C |
 *    4|L  | G
 *     +---+
 */
extern void
position_render(char render[MAX_RENDER], const struct position *p)
{
	char board[SQUARE_COUNT], sente_hand[8], gote_hand[8];
	size_t sente_count = 0, gote_count = 0, i;

	/* empty squares are spaces, no \0 terminator! */
	memset(board, ' ', sizeof board);

	if (gote_moves(p))
		gote_hand[gote_count++] = '*';
	else
		sente_hand[sente_count++] = '*';

	/* place pieces on the board */
	for (i = 0; i < PIECE_COUNT; i++) {
		if (piece_in(HAND, p->pieces[i])) {
			if (p->pieces[i] & GOTE_PIECE)
				gote_hand[gote_count++] = piece_letter(i, p);
			else
				sente_hand[sente_count++] = piece_letter(i, p);
		} else
			/* (11 - square) for coordinate transform */
			board[11 - (p->pieces[i] & ~GOTE_PIECE)] = piece_letter(i, p);
	}

	gote_hand[gote_count] = '\0';
	sente_hand[sente_count] = '\0';

	sprintf(render, "  ABC \n +---+\n1|%.3s| %s\n2|%.3s|\n3|%.3s|\n4|%.3s| %s\n +---+\n",
	    board + 0, gote_hand, board + 3, board + 6, board + 9, sente_hand);
}

/*
 * Make a string describing the position p in a syntax similar to the
 * Forsyth-Edwards-Notation used by international chess.  For example,
 * the initial position is encoded as S/gle/-c-/-C-/ELG/- with S or G
 * in the beginning encoding who makes the next move and the last part
 * indicating the pieces in hand or - if there are none.
 */
extern void
position_string(char render[MAX_POSSTR], const struct position *p)
{

	char hand[8];
	size_t i, hand_count = 0;

	render[0] = gote_moves(p) ? 'G' : 'S';
	strcpy(render + 1, "/---/---/---/---/");

	for (i = 0; i < PIECE_COUNT; i++)
		if (piece_in(HAND, p->pieces[i]))
			hand[hand_count++] = piece_letter(i, p);
		else
			render[coordinates[p->pieces[i] & ~GOTE_PIECE]] = piece_letter(i, p);

	if (hand_count == 0)
		hand[hand_count++] = '-';

	hand[hand_count] = '\0';

	strcat(render, hand);
}

/*
 * Display a move in modified algebraic notation.  The differences are:
 *  - check and mate are not indicated
 *  - a drop is indicated by an asterisk followed by the drop square
 *  - a promotion is indicated by an appended +
 */
extern void
move_string(char render[MAX_MOVSTR], const struct position *p, struct move m)
{
	render[0] = sente_pieces[m.piece];
	if (piece_in(HAND, p->pieces[m.piece]))
		memcpy(render + 1, "  *", 3);
	else {
		memcpy(render + 1, squares[p->pieces[m.piece] & ~GOTE_PIECE], 2);
		render[3] = piece_in(swap_colors(p->map), m.to) ? 'x' : '-';
	}

	memcpy(render + 4, squares[m.to & ~GOTE_PIECE], 2);
	if (!is_promoted(m.piece, p)
	    && (m.piece == CHCK_S || m.piece == CHCK_G)
	    && piece_in(gote_moves(p) ? PROMZ_G : PROMZ_S, m.to))
		memcpy(render + 6, "+", 2);
	else
		render[6] = '\0';
}

/*
 * Assign square sq to the piece represented by letter pc.  If both
 * pieces of that type have already been placed or if we can't figure
 * out what piece that's supposed to be, -1 is returned.  No other
 * invariants are checked, this is done by a call to position_valid()
 * later on.
 */
static int
place_piece(char pcltr, size_t sq, struct position *p)
{
	const char *pcptr;
	size_t pc;
	unsigned needs_promotion = 0;

	pcptr = memchr(sg_pieces, pcltr, PIECE_COUNT);
	if (pcptr != NULL)
		pc = pcptr - sg_pieces;
	else {
		needs_promotion = 1;
		if (pcltr == 'R')
			pc = CHCK_S;
		else if (pcltr == 'r')
			pc = CHCK_G;
		else
			return (-1);
	}

	if (pc & 1)
		sq |= GOTE_PIECE;

	if (p->pieces[pc] == 0xff) {
		p->pieces[pc] = sq;
		p->status |= needs_promotion << pc;
		return (0);
	}

	if (p->pieces[pc ^ 1] == 0xff) {
		p->pieces[pc ^ 1] = sq;
		p->status |= needs_promotion << (pc ^ 1);
		return (0);
	}

	return (-1);
}

/*
 * Parse a position string and fill p with the information derived from
 * it.  On success, return 0, on failure, return -1.  On error, the
 * content of p is undefined.
 */
extern int
parse_position(struct position *p, const char code[MAX_POSSTR])
{
	size_t i;

	/* quick sanity check */
	if (strlen(code) < 18)
		return (-1);

	p->map = 0;
	memset(p->pieces, -1, PIECE_COUNT);

	if (code[0] == 'S')
		p->status = 0;
	else if (code[0] == 'G')
		p->status = GOTE_MOVES;
	else
		return (-1);

	/* first, parse pieces in hand */
	for (i = 18; code[i] != '\0'; i++) {
		if (code[i] == '-')
			break;

		if (place_piece(code[i], IN_HAND, p))
			return (-1);
	}

	/* next, place pieces on the board */
	for (i = 0; i < SQUARE_COUNT; i++) {
		if (code[coordinates[i]] == '-')
			continue;

		if (place_piece(code[coordinates[i]], i, p))
			return (-1);
	}

	/* check if any pieces weren't assigned */
	if (memchr(p->pieces, -1, PIECE_COUNT) != NULL)
		return (-1);

	for (i = 0; i < PIECE_COUNT; i++)
		p->map |= 1 << p->pieces[i];

	p->map &= BOARD;

	return (position_valid(p) ? 0 : -1);
}