#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "dobutsutable.h"

/*
 * Release all storage associated with tb.  The pointer to tb then
 * becomes invalid.
 */
extern void
free_tablebase(struct tablebase *tb)
{
	free(tb);
}

/*
 * Lookup the position with position code pc in the table base.  It is
 * assumed that pc is a valid position code.
 */
extern tb_entry
lookup_poscode(const struct tablebase *tb, poscode pc)
{

	/*
	 * all positions between LIONPOS_COUNT and LIONPOS_TOTAL_COUNT
	 * are immediate wins and not stored in the table base.
	 */
	if (pc.lionpos >= LIONPOS_COUNT)
		return (1);
	else
		return (tb->positions[position_offset(pc)]);
}

/*
 * This useful auxillary function encodes a position and then looks it
 * up in the table base, saving some time.
 */
extern tb_entry
lookup_position(const struct tablebase *tb, const struct position *p)
{
	poscode pc;

	/* checkmates aren't looked up */
	if (gote_moves(p) ? sente_in_check(p) : gote_in_check(p))
		return (1);

	encode_position(&pc, p);

	return (lookup_poscode(tb, pc));
}

/*
 * Write tb to file f.  It is assumed that f has been opened in binary
 * mode for writing and truncated.  This function returns 0 on success,
 * -1 on error with errno indicating the reason for failure.
 */
extern int
write_tablebase(FILE *f, const struct tablebase *tb)
{
	poscode pc;

	rewind(f);

	pc.map = 0;
	for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++)
		for (pc.cohort = 0; pc.cohort < COHORT_COUNT; pc.cohort++)
			for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++)
				fwrite(tb->positions + position_offset(pc),
				    1, cohort_size[pc.cohort].size, f);

	fflush(f);

	return (ferror(f) ? -1 : 0);
}

/*
 * Read a tablebase from file f.  It is assumed that f has been opened
 * in binary mode for reading.  This function returns a pointer to the
 * newly loaded tablebase on success or NULL on error with errno
 * indicating the reason for failure.
 */
extern struct tablebase *
read_tablebase(FILE *f)
{
	struct tablebase *tb = malloc(sizeof *tb);
	poscode pc;

	if (tb == NULL)
		return (NULL);

	rewind(f);

	pc.map = 0;
	for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++)
		for (pc.cohort = 0; pc.cohort < COHORT_COUNT; pc.cohort++)
			for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++)
				fread(tb->positions + position_offset(pc),
				    1, cohort_size[pc.cohort].size, f);

	if (ferror(f)) {
		free(tb);
		return (NULL);
	}

	return (tb);
}
