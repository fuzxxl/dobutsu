This file contains some explanations on how the game database and its
index works.

Lion positions
==============

The sente lion has five squares to be on: If the lion is on A,
he has already won, so this can't happen.  If he's on B, we can
mirror the board. When he's on C, there is no way to place the
Gote lion so this can't be the case.

    +---+
    |AAA|
    |BCX|
    |BXX|
    |BXX|
    +---+

The Gote lion has up to seven squares. When he's in the opponents
promotion zone A he is either in check (in which case the position is
invalid) or has already won. When he's on B, he is in check by Sente
which makes the position invalid. This leaves 7 + 6 + 5 + 3 + 3 = 24
positions for the lions:

    +---+ +---+ +---+ +---+ +---+
    |XXX| |XXX| |XXX| |XXX| |XBB|
    |XXX| |XXX| |XBB| |BBB| |XBL|
    |XBB| |BBB| |XBL| |BLB| |XBB|
    |AAL| |ALA| |AAA| |AAA| |AAA|
    +---+ +---+ +---+ +---+ +---+

Other pieces
============

The remaining pieces have up to 11 positions: up to ten free squares or
in the hand of the player.  Requiring the first piece to be on a higher
square than the second for parity, this gives 56 positions for each pair
of pieces (9 + 8 + ... + 1 and 1 for both pieces being in hand).  This
is used to make the index smaller.

Ownership and promotion
=======================

The ownership and promotion byte is defined as follows:

    owned by gote:      0
    owned by sente:     1

    unpromoted (chick): 0
    promoted (rooster): 1

There are eight fields in the byte:

     owner 1st chick:    0x01
     owner 2nd chick:    0x02
     prom. 1st chick:    0x04
     prom. 2nd chick:    0x08
     owner 1st elephant: 0x10
     owner 2nd elephant: 0x20
     owner 1st giraffe:  0x40
     owner 2nd giraffe:  0x80
 
We require a chick in hand to be not promoted to remove a couple more
positions from the file.  Lastly, if Gote's lion is in check Sente has
already won so we don't have to store these positions in the table.

Number of positions
===================

Altogether we get:

    256 * 24 * 56 * 56 * 56 = 1078984704 positions.

If we have one byte per position, that's an endgame file of 1 Gigabyte.

Only 141192553 positions are neither invalid nor already won or lost, so
86.91% of the space is wasted.  It's hard to make a better index format
without making it very complicated.

Sample positions
================

The initial position is encoded as 719600465, displayed as

    +---+
    |gle| 
    | c |
    | C |
    |ELG| 
    +---+

and represented by a position structure containing
`( 4, 7, 0,11, 2, 9,10, 1,51)`. Compare the output of `displaytest`.

There are two positions that end in a stalemate because Sente cannot
move.  They are described by the indices 893614067 and 896421363 and
look like this:

    +---+    +---+
    |lEG|    | EG|
    |EGL|    |EGL|
    | CC|    |lCC|
    |   |    |   |
    +---+    +---+

Endgame database content
========================

This is not just an endgame tablebase, it's a complete database. The
database contains one byte of information for each encoded position.
This byte is 0xff for an invalid position, 0xfe for a position that is
neither won nor lost, even for a won position and odd for a lost
position.  Should the position be won or lost, it's value indicates
the distance to win when the opponent plays the strongest possible
defence.  A position from which Sente can take Gote's lion or from which
Sente can ascend is encoded as 0x00, a position in which Sente is mated
is encoded as 0x01 as regardless of what move Sente does, Gote can
always take its lion.  Technically speaking, we could also ignore
entries marked 0x00 or 0xff to make the database easier to compress,
which is done in the proposed second level encoding below, but it's
slightly faster to lookup the position code in the table than to decode
the index and check if the position is valid.