#include <stdio.h>
#include "board.h"

#define PROGRAM_NAME         "Sample HYPER Dice Shogi Player"
#define VERSION              "0.1.0"

#define SEARCH_DEPTH         5

#define SCORE_MAX            8100
#define SCORE_MATED          (-SCORE_MAX)

#define SIZE_FILENAME        64

extern const char *ch_piece[16];
extern const char *ch_piece2[32];
extern const char *ch_piece_csa[16];
extern FILE *file_log;

/* main.c */
void close_program();

/* io.c */
int cmd_prompt();
void out( const char *format, ... );
void out_file( FILE *fp, const char *format, ... );
void out_position();
void out_board();
void out_legalmoves( unsigned int moves[], int count );
void str_CSA_move( char *buf, unsigned int move );
unsigned int  CSA2Internal( const char *str );
int out_record( int resign );

/* ini.c */
void game_initialize();
void game_finalize();

/* search.c */
int search_root( int dice );
int search( int depth, int ply);
short evaluate();
