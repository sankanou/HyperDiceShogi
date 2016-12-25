#include <sys/time.h>
#include <string.h>
#include <assert.h>
#include "header.h"

static int mated_node( int depth, int ply );
static int not_mated_node( int depth, int ply );

int search_root( int dice )
{
  /*
    ret =  0: succeeded
        = -1: failed
        = -2: the game is already finished
   */
  int imove, best;
  int nmove = 0;
  int depth = SEARCH_DEPTH;
  short value;
  short max = 0;
  unsigned int legalmoves[ SIZE_LEGALMOVES ];

  nmove = gen_dicenum_legalmoves( legalmoves, dice );
  
  if( nmove == 0 )
    {
      if( is_mated() == 1 )
	{
	  out( " Checkmate.\n" );
	  return -2;
	}
      else
	{
	  out( " Stalemate.\n" );
	  return -3;
	}
    }

  out( " Root move generation -> %d moves.\n", nmove );
  
  if( nmove <= 1 )
    {
      best = 0;
      max = 0;
    }

  best = 0;
  max   = -SCORE_MAX;
  
  for( imove = 0; imove < nmove; imove++ )
    {
      MAKE_MOVE( legalmoves[ imove ] );
      value = -search( depth -1, 1 );
      UNMAKE_MOVE;
      
      if( value > max )
	{
	  max = value;
	  best = imove;
	}
    
    }

  MAKE_MOVE( legalmoves[ best ] );
  return 0;
}

int search( int depth, int ply )
{
  if( depth <= 0 )
    {
      return evaluate();
    }

  if( is_mated() == 1 )
    {
      return mated_node( depth, ply ); 
    }
  else
    {
      return not_mated_node( depth, ply );
    }
}

int mated_node( int depth, int ply )
{
  int imove;
  short value;
  short max = -SCORE_MAX;  
  int nmove;
  unsigned int legalmoves[ SIZE_LEGALMOVES ];
  
  nmove = gen_legalmoves( legalmoves );
  
  if( nmove == 0 )
    { return SCORE_MATED + ply; }
  
  for( imove = 0; imove < nmove; imove++ )
    {
      MAKE_MOVE( legalmoves[ imove ] );
      value = -search( depth -1, ply +1 );
      UNMAKE_MOVE;
      
      if( value > max )
	{
	  max = value;
	}
      
    }
  return max;
}

int not_mated_node( int depth, int ply )
{
  int imove, dice;
  short value;
  short max = -SCORE_MAX, dice_max[ DICE_NUM - 1 ];  
  int nmove, dice_nmove[ DICE_NUM - 1 ];
  unsigned int dicelegalmoves[ DICE_NUM - 1 ][ SIZE_DICE_LEGALMOVES ];

  nmove = gen_dicelegalmoves( dicelegalmoves, dice_nmove );
  
  if( nmove == 0 )
    { return -SCORE_MATED - ply; }

  for(dice = 1; dice < DICE_NUM; dice++)
    {
      if( dice_nmove[ dice - 1 ] != 0 ){
	max = -SCORE_MAX;
	for( imove = 0; imove < dice_nmove[ dice - 1 ]; imove++ )
	  {
	    MAKE_MOVE( dicelegalmoves[ dice - 1 ][ imove ] );
	    value = -search( depth -1, ply +1 );
	    UNMAKE_MOVE;
      
	    if( value > max )
	      {
		max = value;
	      }
      
	  }
	dice_max[ dice - 1 ] = max;
      }
    }

  max = -SCORE_MAX;
  for( dice = 1; dice < DICE_NUM; dice++ )
    if( dice_max[ dice - 1 ] > max && dice_nmove[ dice - 1 ] != 0 )
      max = dice_max[ dice - 1 ];

  for( dice = 1; dice < DICE_NUM; dice++ )
    if( dice_nmove[ dice - 1 ] == 0 )
      dice_max[ dice - 1 ] = max;
      
  return ( dice_max[0] + dice_max[1] + dice_max[2] + dice_max[3] + dice_max[4] + max ) / DICE_NUM;
}

short evaluate()
{
  int score = 0;

  score += ( popuCount( w_pawn )       - popuCount( b_pawn ) )       * 100;
  score += ( popuCount( w_silver )     - popuCount( b_silver ) )     * 300;
  score += ( popuCount( w_gold )       - popuCount( b_gold ) )       * 350;
  score += ( popuCount( w_bishop )     - popuCount( b_bishop ) )     * 350;
  score += ( popuCount( w_rook )       - popuCount( b_rook ) )       * 400;
  score += ( popuCount( w_pro_pawn )   - popuCount( b_pro_pawn ) )   * 200;
  score += ( popuCount( w_pro_silver ) - popuCount( b_pro_silver ) ) * 350;
  score += ( popuCount( w_horse )      - popuCount( b_horse ) )      * 450;
  score += ( popuCount( w_dragon )     - popuCount( b_dragon ) )     * 500;

  score += ( W_HAND( pawn )   - B_HAND( pawn ) )   * 100;
  score += ( W_HAND( silver ) - B_HAND( silver ) ) * 280;
  score += ( W_HAND( gold )   - B_HAND( gold ) )   * 370;
  score += ( W_HAND( bishop ) - B_HAND( bishop ) ) * 340;
  score += ( W_HAND( rook )   - B_HAND( rook ) )   * 390;
  
  return get_turn() ? -score: score;
}
