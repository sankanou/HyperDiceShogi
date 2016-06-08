#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "board.h"

tree_t game;
hist_t history[ RECORD_SIZE ];

unsigned int nOccupiedW;
unsigned int nOccupiedB;
unsigned int Occupied0;

unsigned int Attack_WPawn[32];
unsigned int Attack_WSilver[32];
unsigned int Attack_WGold[32];
unsigned int Attack_BPawn[32];
unsigned int Attack_BSilver[32];
unsigned int Attack_BGold[32];
unsigned int Attack_King[32];
unsigned int Attack_Rook[32][64];
unsigned int Attack_Rook_magic[32];
unsigned int Attack_Rook_mask[32];
unsigned int Attack_Bishop[32][64];
unsigned int Attack_Bishop_magic[32];
unsigned int Attack_Bishop_mask[32];
unsigned int Pin_Rook[32][32][64][2];
unsigned int Pin_Bishop[32][32][64][2];
unsigned int DoublePawn[32];

const int Attack_Rook_shift[32] =
{
  19, 20, 20, 20, 19,
  20, 21, 21, 21, 20,
  20, 21, 21, 21, 20,
  20, 21, 21, 21, 20,
  19, 20, 20, 20, 19
};

const int Attack_Bishop_shift[32] =
{
  22, 23, 23, 23, 22,
  23, 23, 23, 23, 23,
  23, 23, 21, 23, 23,
  23, 23, 23, 23, 23,
  22, 23, 23, 23, 22
};

int is_mated(){
  unsigned int attack_on_king;
  unsigned int att;

  if( TURN ) attack_on_king = attacks_to_w( SQ_B_KING, &att );
  else attack_on_king = attacks_to_b( SQ_W_KING, &att );

  if( attack_on_king > 0 ) return 1;
  return 0;
}

int gen_dicenum_legalmoves( unsigned int moves[], int dice ){
  /* dice is 1 to 6 */
  int buf_nmove, i, nmove = 0;
  unsigned int buf_moves[ SIZE_LEGALMOVES ];

  if( is_mated() == 1 || dice == 6)
      return gen_legalmoves( moves );

  buf_nmove = gen_legalmoves( buf_moves );
  for( i = 0; i < buf_nmove; i++ )
    if( MOVE_TO( buf_moves[i] ) % 5 == dice - 1 )
      {
	moves[ nmove ] = buf_moves[i];
	nmove++;
      }
  if( nmove != 0 )
    return nmove;
  else
    return gen_legalmoves( moves );
}

int gen_dicelegalmoves( unsigned int moves[][ SIZE_DICE_LEGALMOVES ], int nmove[] ){
  int buf_nmove, i, dice;
  unsigned int buf_moves[ SIZE_LEGALMOVES ];

  for( i = 0; i < DICE_NUM - 1; i++ ) nmove[i] = 0;
  buf_nmove = gen_legalmoves( buf_moves );

  for( i = 0; i < buf_nmove; i++ ){
    dice = MOVE_TO( buf_moves[i] ) % 5;
    moves[ dice ][ nmove[ dice ] ] = buf_moves[i];
    nmove[ dice ]++;
  }
  return buf_nmove;
}

int gen_legalmoves( unsigned int moves[] )
{
  unsigned int pin[32], attack_on_king;
  unsigned int att;
  int nmove = 0;
  
  if( TURN )
    {
      pinInfo_b( pin );
      attack_on_king = attacks_to_w( SQ_B_KING, &att );
      if( attack_on_king > 0 )
        {
          nmove = gen_evasion_b( moves, 0, attack_on_king, att, pin );
        }
      else
        {
          nmove = gen_cap_b( moves, nmove, pin );
          nmove = gen_drop_b( moves, nmove );
          nmove = gen_nocap_b( moves, nmove, pin );
        }
    }
  else
    {
      pinInfo_w( pin );
      attack_on_king = attacks_to_b( SQ_W_KING, &att );
      if( attack_on_king > 0 )
        {
          nmove = gen_evasion_w( moves, 0, attack_on_king, att, pin );
        }
      else{
        nmove = gen_cap_w( moves, nmove, pin );
        nmove = gen_drop_w( moves, nmove );
        nmove = gen_nocap_w( moves, nmove, pin );
      }
    }
  
  return nmove;
}

int gen_evasion_w( unsigned int moves[], int count, int nAttacks,
                   unsigned int attack_pieces, unsigned int pin[]  )
{
  unsigned int dests, att;
  int to, cap, index;

  dests = Attack_King[ SQ_W_KING ] & nOccupiedW;

  Occupied0   ^= Bit( SQ_W_KING );      /* assume that the king is absent */
  while( (to = FirstOne( dests )) != -1 )
    {
      if( attacks_to_b( to, &att ) == 0 )
        {
          cap = ( Bit( to ) & (~nOccupiedB) ) ? 1 : 0 ;
          moves[ count ] = MOVE( w_king, SQ_W_KING, to, 0, cap );
          count ++;
        }
      dests ^= Bit( to );
    }
  Occupied0   ^= Bit( SQ_W_KING );
  
  if( nAttacks >= 2 )
    { return count; }


  int sq = FirstOne( attack_pieces );
  assert( sq >= 0 );
  count = gen_attacks_to_w( moves, count, sq, 1, pin );


#define DROP( piece ) \
  if( IsHand_W( piece ) ){ \
    moves[ count ] = MOVE( w_ ## piece, move_drop, to, 0, 0 ); \
    count ++;}

  if( attack_pieces & ( Attack_King[ SQ_W_KING ] ) )
    {
      /* nothing to do */
    }
  else if( attack_pieces & ( BB_B_ROOK | BB_B_DRAGON ) )
    {
      index  = (( (Occupied0 & Attack_Rook_mask[sq]) * Attack_Rook_magic[sq])
               >> Attack_Rook_shift[ sq ]) & mask_magic;
      dests  = Attack_Rook[ sq ][ index ];

      index  = (( (Occupied0 & Attack_Rook_mask[ SQ_W_KING ])
                * Attack_Rook_magic[ SQ_W_KING ])
               >> Attack_Rook_shift[ SQ_W_KING ]) & mask_magic;
      dests &= Attack_Rook[ SQ_W_KING ][ index ];

      assert( dests );

      while( (to = FirstOne( dests )) != -1 )
        {
          if( W_HAND_A )
            {
              if( to >= 5 && IsHand_W( pawn ) )
                {
                  int dp = FirstOne( BB_W_PAWN );
                  if( dp < 0 || Bit(to) & ~DoublePawn[ dp ] )
                    {
                      moves[ count ] = MOVE( w_pawn, move_drop, to, 0, 0 );
                      count ++;
                    }
                }
              DROP( silver );
              DROP( gold );
              DROP( bishop );
              DROP( rook );
            }
          
          count = gen_attacks_to_w( moves, count, to, 0, pin );

          dests ^= Bit( to );
        }
    }
  else if( attack_pieces & ( BB_B_BISHOP | BB_B_HORSE ) )
    {
      index  = (( (Occupied0 & Attack_Bishop_mask[sq])
                  * Attack_Bishop_magic[sq])
                >> Attack_Bishop_shift[ sq ]) & mask_magic;
      dests  = Attack_Bishop[ sq ][ index ];

      index  = (( (Occupied0 & Attack_Bishop_mask[ SQ_W_KING ])
                * Attack_Bishop_magic[ SQ_W_KING ])
               >> Attack_Bishop_shift[ SQ_W_KING ]) & mask_magic;
      dests &= Attack_Bishop[ SQ_W_KING ][ index ];
      
      assert( dests );

      while( (to = FirstOne( dests )) != -1 )
        {
          if( W_HAND_A )
            {
              if( to >= 5 && IsHand_W( pawn ) )
                {
                  int dp = FirstOne( BB_W_PAWN );
                  if( dp < 0 || Bit(to) & ~DoublePawn[ dp ] )
                    { 
                      moves[ count ] = MOVE( w_pawn, move_drop, to, 0, 0 );
                      count ++;
                    }
                }
              DROP( silver );
              DROP( gold );
              DROP( bishop );
              DROP( rook );
            }
          
          count = gen_attacks_to_w( moves, count, to, 0, pin );

          dests ^= Bit( to );
        }
    }

#undef DROP
  
  return count;
}

int gen_evasion_b( unsigned int moves[], int count, int nAttacks,
                   unsigned int attack_pieces, unsigned int pin[] )
{
  unsigned int dests, att;
  int to, cap, index;

  dests = Attack_King[ SQ_B_KING ] & nOccupiedB;

  Occupied0   ^= Bit( SQ_B_KING );      /* assume that the king is absent */
  while( (to = FirstOne( dests )) != -1 )
    {
      if( attacks_to_w( to, &att ) == 0 )
        {
          cap = ( Bit( to ) & (~nOccupiedW) ) ? 1 : 0 ;
          moves[ count ] = MOVE( b_king, SQ_B_KING, to, 0, cap );
          count ++;
        }
      dests ^= Bit( to );
    }
  Occupied0   ^= Bit( SQ_B_KING );
  
  if( nAttacks >= 2 )
    { return count; }

  
  int sq = FirstOne( attack_pieces );
  assert( sq >= 0 );
  count = gen_attacks_to_b( moves, count, sq, 1, pin );


#define DROP( piece ) \
  if( IsHand_B( piece ) ){ \
    moves[ count ] = MOVE( b_ ## piece, move_drop, to, 0, 0 ); \
    count ++;}

  if( attack_pieces & ( Attack_King[ SQ_B_KING ] ) )
    {
      /* nothing to do */
    }
  else if( attack_pieces & ( BB_W_ROOK | BB_W_DRAGON ) )
    {
      index  = (( (Occupied0 & Attack_Rook_mask[sq]) * Attack_Rook_magic[sq])
               >> Attack_Rook_shift[ sq ]) & mask_magic;
      dests  = Attack_Rook[ sq ][ index ];

      index  = (( (Occupied0 & Attack_Rook_mask[ SQ_B_KING])
                * Attack_Rook_magic[ SQ_B_KING ])
               >> Attack_Rook_shift[ SQ_B_KING ]) & mask_magic;
      dests &= Attack_Rook[ SQ_B_KING ][ index ];
      
      assert( dests );
      
      while( (to = FirstOne( dests )) != -1 )
        {
          if( B_HAND_A )
            {
              if( to < 20 && IsHand_B( pawn ) )
                {
                  int dp = FirstOne( BB_B_PAWN );
                  if( dp < 0 || Bit(to) & ~DoublePawn[ dp ] )
                    { 
                      moves[ count ] = MOVE( b_pawn, move_drop, to, 0, 0 );
                      count ++;
                    }
                }
              DROP( silver );
              DROP( gold );
              DROP( bishop );
              DROP( rook );
            }
          
          count = gen_attacks_to_b( moves, count, to, 0, pin );

          dests ^= Bit( to );
        }
    }
  else if( attack_pieces & ( BB_W_BISHOP | BB_W_HORSE ) )
    {
      index  = (( (Occupied0 & Attack_Bishop_mask[sq])
                  * Attack_Bishop_magic[sq])
                >> Attack_Bishop_shift[ sq ]) & mask_magic;
      dests  = Attack_Bishop[ sq ][ index ];

      index  = (( (Occupied0 & Attack_Bishop_mask[ SQ_B_KING ])
                * Attack_Bishop_magic[ SQ_B_KING ])
               >> Attack_Bishop_shift[ SQ_B_KING ]) & mask_magic;
      dests &= Attack_Bishop[ SQ_B_KING ][ index ];
      
      assert( dests );

      while( (to = FirstOne( dests )) != -1 )
        {
          if( B_HAND_A )
            {
              if( to < 20 && IsHand_B( pawn ) )
                {
                  int dp = FirstOne( BB_B_PAWN );
                  if( dp < 0 || Bit(to) & ~DoublePawn[ dp ] )
                    { 
                      moves[ count ] = MOVE( b_pawn, move_drop, to, 0, 0 );
                      count ++;
                    }
                }
              DROP( silver );
              DROP( gold );
              DROP( bishop );
              DROP( rook );
            }
          
          count = gen_attacks_to_b( moves, count, to, 0, pin );

          dests ^= Bit( to );
        }
    }

#undef DROP
  
  return count;
}

int mate_by_dropping_pawn_w( int sq )
{
 /* ret = 1: illegal
        = 0: legal   */
  unsigned int dests, att;
  int to;
  unsigned int moves[ SIZE_LEGALMOVES ];
  
  dests = Attack_King[ SQ_B_KING ] & nOccupiedB;

  /* not necessary to assume king's absence 'cause the king cannot
     be attacked by sliding piece in this case.                 */

  Occupied0   ^= Bit( sq );
  while( (to = FirstOne( dests )) != -1 )
    {
      if( attacks_to_w( to, &att ) == 0 )
        {
          Occupied0   ^= Bit( sq );
          return 0;
        }
      dests ^= Bit( to );
    }
  Occupied0   ^= Bit( sq );

  unsigned int pin[32];
  pinInfo_b( pin );
  if( gen_attacks_to_b( moves, 0, sq, 0, pin ) )
    { return 0; }

  return 1;
}

int mate_by_dropping_pawn_b( int sq )
{
 /* ret = 1: illegal
        = 0: legal   */
  unsigned int dests, att;
  int to;
  unsigned int moves[ SIZE_LEGALMOVES ];
  
  dests = Attack_King[ SQ_W_KING ] & nOccupiedW;

  /* not necessary to assume king's absence 'cause the king cannot
     be attacked by sliding piece in this case.                 */

  Occupied0   ^= Bit( sq );
  while( (to = FirstOne( dests )) != -1 )
    {
      if( attacks_to_b( to, &att ) == 0 )
        {
          Occupied0   ^= Bit( sq );
          return 0;
        }
      dests ^= Bit( to );
    }
  Occupied0   ^= Bit( sq );


  unsigned int pin[32];
  pinInfo_w( pin );
  if( gen_attacks_to_w( moves, 0, sq, 0, pin ) )
    { return 0; }

  return 1;
}

#define GEN_ATTACK( piece, promote ) \
  while( ( from = FirstOne( dests ) ) != -1){ \
    int prom = 0; \
    if( ~pin[ from ] & Bit( sq ) ){ \
      if( promote && (from < 5 || sq < 5) ){ prom = 1; } \
      moves[ count ] = MOVE( piece, from, sq, prom, cap ); \
      count ++; } \
    dests ^= Bit( from ); }

int gen_attacks_to_w( unsigned int moves[], int count, int sq, int cap, unsigned int pin[] )
{
  /* King is not considered. */
  unsigned int dests;
  int from, index;

  if( (dests = (Attack_BPawn[ sq ] & BB_W_PAWN)) != 0 )
    {
      from = FirstOne( dests );
      if( ~pin[ from ] & Bit( sq ) )
        {
          if( sq < 5 )
            {
              moves[ count ] = MOVE( w_pawn, from, sq, 1, cap );
            }
          else
            {
              moves[ count ] = MOVE( w_pawn, from, sq, 0, cap );
            }
          count ++;
        }
    }

  dests  = Attack_BSilver[ sq ] & BB_W_SILVER;
  while( ( from = FirstOne( dests ) ) != -1)
    {
      if( ~pin[ from ] & Bit( sq ) ){
        if( from < 5 || sq < 5 )
          { moves[ count ] = MOVE( w_silver, from, sq, 1, cap );
            count ++; }
        moves[ count ] = MOVE( w_silver, from, sq, 0, cap );
        count ++; }
      dests ^= Bit( from );
    }
  
  dests  = Attack_BGold[ sq ] & BB_W_GOLD;
  GEN_ATTACK( w_gold, 0 );

  dests  = Attack_BGold[ sq ] & BB_W_PRO_SILVER;
  GEN_ATTACK( w_pro_silver, 0 );

  dests  = Attack_BGold[ sq ] & BB_W_PRO_PAWN;
  GEN_ATTACK( w_pro_pawn, 0 );

  dests  = Attack_King[ sq ];
  index =(((Occupied0 & Attack_Bishop_mask[sq])
           * Attack_Bishop_magic[sq])
          >> Attack_Bishop_shift[sq]) & mask_magic;
  dests |= Attack_Bishop[ sq ][ index ];
  dests &= BB_W_HORSE;
  GEN_ATTACK( w_horse, 0 );

  dests  = Attack_King[ sq ];
  index  = (( (Occupied0 & Attack_Rook_mask[sq]) * Attack_Rook_magic[sq])
            >> Attack_Rook_shift[ sq ]) & mask_magic;
  dests |= Attack_Rook[ sq ][ index ];
  dests &= BB_W_DRAGON;
  GEN_ATTACK( w_dragon, 0 );


  index =(((Occupied0 & Attack_Bishop_mask[sq])
           * Attack_Bishop_magic[sq])
          >> Attack_Bishop_shift[sq]) & mask_magic;
  dests = Attack_Bishop[ sq ][ index ];
  dests &= BB_W_BISHOP;
  GEN_ATTACK( w_bishop, 1 );

  index  = (( (Occupied0 & Attack_Rook_mask[sq]) * Attack_Rook_magic[sq])
            >> Attack_Rook_shift[ sq ]) & mask_magic;
  dests  = Attack_Rook[ sq ][ index ];
  dests &= BB_W_ROOK;
  GEN_ATTACK( w_rook, 1 );

  return count;
}
#undef GEN_ATTACK

#define GEN_ATTACK( piece, promote ) \
  while( ( from = FirstOne( dests ) ) != -1){ \
    int prom = 0; \
    if( ~pin[ from ] & Bit( sq ) ){ \
      if( promote && (from > 19 || sq > 19) ){ prom = 1; } \
      moves[ count ] = MOVE( piece, from, sq, prom, cap ); \
      count ++; } \
    dests ^= Bit( from ); }

int gen_attacks_to_b( unsigned int moves[], int count, int sq, int cap, unsigned int pin[] )
{
  /* King is not considered. */
  unsigned int dests;
  int from, index;

  if( (dests = (Attack_WPawn[ sq ] & BB_B_PAWN)) != 0 )
    {
      from = FirstOne( dests );
      if( ~pin[ from ] & Bit( sq ) )
        {
          if( sq > 19 )
            {
              moves[ count ] = MOVE( b_pawn, from, sq, 1, cap );
            }
          else
            {
              moves[ count ] = MOVE( b_pawn, from, sq, 0, cap );
            }
          count ++;
        }
    }

  dests  = Attack_WSilver[ sq ] & BB_B_SILVER;
  while( ( from = FirstOne( dests ) ) != -1)
    {
      if( ~pin[ from ] & Bit( sq ) ){
        if( from > 19 || sq > 19 )
          { moves[ count ] = MOVE( b_silver, from, sq, 1, cap );
            count ++; }
        moves[ count ] = MOVE( b_silver, from, sq, 0, cap );
        count ++; }
      dests ^= Bit( from );
    }
  
  dests  = Attack_WGold[ sq ] & BB_B_GOLD;
  GEN_ATTACK( b_gold, 0 );

  dests  = Attack_WGold[ sq ] & BB_B_PRO_SILVER;
  GEN_ATTACK( b_pro_silver, 0 );

  dests  = Attack_WGold[ sq ] & BB_B_PRO_PAWN;
  GEN_ATTACK( b_pro_pawn, 0 );

  dests  = Attack_King[ sq ];
  index =(((Occupied0 & Attack_Bishop_mask[sq])
           * Attack_Bishop_magic[sq])
          >> Attack_Bishop_shift[sq]) & mask_magic;
  dests |= Attack_Bishop[ sq ][ index ];
  dests &= BB_B_HORSE;
  GEN_ATTACK( b_horse, 0 );

  dests  = Attack_King[ sq ];
  index  = (( (Occupied0 & Attack_Rook_mask[sq]) * Attack_Rook_magic[sq])
            >> Attack_Rook_shift[ sq ]) & mask_magic;
  dests |= Attack_Rook[ sq ][ index ];
  dests &= BB_B_DRAGON;
  GEN_ATTACK( b_dragon, 0 );

  index =(((Occupied0 & Attack_Bishop_mask[sq])
           * Attack_Bishop_magic[sq])
          >> Attack_Bishop_shift[sq]) & mask_magic;
  dests = Attack_Bishop[ sq ][ index ];
  dests &= BB_B_BISHOP;
  GEN_ATTACK( b_bishop, 1 );

  index  = (( (Occupied0 & Attack_Rook_mask[sq]) * Attack_Rook_magic[sq])
            >> Attack_Rook_shift[ sq ]) & mask_magic;
  dests  = Attack_Rook[ sq ][ index ];
  dests &= BB_B_ROOK;
  GEN_ATTACK( b_rook, 1 );

  return count;
}

#undef GEN_ATTACK

int attacks_to_w( int sq, unsigned int *attack_pieces )
{
  int count = 0, from, index;
  unsigned int att;

  att  = Attack_BPawn[ sq ] & BB_W_PAWN;
  att |= Attack_BSilver[ sq ] & BB_W_SILVER;
  att |= Attack_BGold[ sq ] &
    ( BB_W_GOLD | BB_W_PRO_PAWN | BB_W_PRO_SILVER );
  att |= Attack_King[ sq ] &
    ( BB_W_HORSE | BB_W_DRAGON | BB_W_KING );

  index = (( (Occupied0 & Attack_Rook_mask[sq]) * Attack_Rook_magic[sq])
            >> Attack_Rook_shift[ sq ]) & mask_magic;
  att |= Attack_Rook[ sq ][ index ] & ( BB_W_ROOK | BB_W_DRAGON );

  index = (( (Occupied0 & Attack_Bishop_mask[sq]) * Attack_Bishop_magic[sq])
            >> Attack_Bishop_shift[ sq ]) & mask_magic;
  att |= Attack_Bishop[ sq ][ index ] & ( BB_W_BISHOP | BB_W_HORSE );

  *attack_pieces = att;
  
  while( (from = FirstOne(att)) != -1 )
    {
      count ++;
      att ^= Bit( from );
    }
  
  return count;
}

int attacks_to_b( int sq, unsigned int *attack_pieces )
{
  int count = 0, from;
  unsigned int att, index;

  att  = Attack_WPawn[ sq ] & BB_B_PAWN;
  att |= Attack_WSilver[ sq ] & BB_B_SILVER;
  att |= Attack_WGold[ sq ] &
    ( BB_B_GOLD | BB_B_PRO_PAWN | BB_B_PRO_SILVER );
  att |= Attack_King[ sq ] &
    ( BB_B_HORSE | BB_B_DRAGON | BB_B_KING  );

  index  = (( (Occupied0 & Attack_Rook_mask[sq]) * Attack_Rook_magic[sq])
            >> Attack_Rook_shift[ sq ]) & mask_magic;
  att |= Attack_Rook[ sq ][ index ] & ( BB_B_ROOK | BB_B_DRAGON );

  index = (( (Occupied0 & Attack_Bishop_mask[sq]) * Attack_Bishop_magic[sq])
            >> Attack_Bishop_shift[ sq ]) & mask_magic;
  att |= Attack_Bishop[ sq ][ index ] & ( BB_B_BISHOP | BB_B_HORSE );

  *attack_pieces = att;
  
  while( (from = FirstOne(att)) != -1 )
    {
      count ++;
      att ^= Bit( from );
    }
  
  return count;
}


void pinInfo_w( unsigned int pin[] ){
  memset( pin, 0, sizeof(unsigned int)*32 );

  unsigned bb;
  int pIndex, sq = SQ_W_KING;
  int index;

  bb = ( (BB_B_ROOK | BB_B_DRAGON) ) & Attack_Rook[sq][0];
  while( (pIndex = FirstOne( bb )) != -1 )
    {
      index  = (( (Occupied0 & Attack_Rook_mask[ pIndex ])
                 * Attack_Rook_magic[pIndex])
                >> Attack_Rook_shift[pIndex]) & mask_magic;
      pin[ Pin_Rook[pIndex][sq][index][0] ]
        |= Pin_Rook[pIndex][sq][index][1];
      bb ^= Bit( pIndex );
    }

  bb = ( (BB_B_BISHOP | BB_B_HORSE) ) & Attack_Bishop[sq][0];
  while( (pIndex = FirstOne( bb )) != -1 )
    {
      index  = (( (Occupied0 & Attack_Bishop_mask[ pIndex ])
                 * Attack_Bishop_magic[pIndex])
                >> Attack_Bishop_shift[pIndex]) & mask_magic;
      pin[ Pin_Bishop[pIndex][sq][index][0] ]
        |= Pin_Bishop[pIndex][sq][index][1];
      bb ^= Bit( pIndex );
    }
  
  return;
}

void pinInfo_b( unsigned int pin[] ){
  memset( pin, 0, sizeof(unsigned int)*32 );

  unsigned bb;
  int pIndex, sq = SQ_B_KING;
  int index;

  bb = ( (BB_W_ROOK | BB_W_DRAGON) ) & Attack_Rook[sq][0];
  while( (pIndex = FirstOne( bb )) != -1 )
    {
      index  = (( (Occupied0 & Attack_Rook_mask[ pIndex ])
                 * Attack_Rook_magic[pIndex])
                >> Attack_Rook_shift[pIndex]) & mask_magic;
      pin[ Pin_Rook[pIndex][sq][index][0] ]
        |= Pin_Rook[pIndex][sq][index][1];
      bb ^= Bit( pIndex );
    }

  bb = ( (BB_W_BISHOP | BB_W_HORSE) ) & Attack_Bishop[sq][0];
  while( (pIndex = FirstOne( bb )) != -1 )
    {
      index  = (( (Occupied0 & Attack_Bishop_mask[ pIndex ])
                 * Attack_Bishop_magic[pIndex])
                >> Attack_Bishop_shift[pIndex]) & mask_magic;
      pin[ Pin_Bishop[pIndex][sq][index][0] ]
        |= Pin_Bishop[pIndex][sq][index][1];
      bb ^= Bit( pIndex );
    }  
  
  return;
}


int gen_drop_w( unsigned int moves[], int count )
{
  unsigned long dests;
  int to;

  if( IsHand_W( gold ) )
    {
      dests = ~Occupied0 & bb_mask;
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_gold, move_drop, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
    }
  
  if( IsHand_W( silver ) )
    {
      dests = ~Occupied0 & bb_mask;
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_silver, move_drop, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
    }
  
  if( IsHand_W( bishop) )
    {
      dests = ~Occupied0 & bb_mask;
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_bishop, move_drop, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
    }

  if( IsHand_W( rook ) )
    {
      dests = ~Occupied0 & bb_mask;
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_rook, move_drop, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
    }
  
  if( IsHand_W( pawn ) )
    {
      dests = ~Occupied0 & bb_mask & ~b1_1111;
      to = FirstOne( BB_W_PAWN );
      if( to >= 0 )
        { dests &= ~DoublePawn[ to ]; }
      
      while( (to = FirstOne(dests)) != -1 )
        {
          if( to == SQ_B_KING + 5 &&  /* test "mate by dropping pawn" */
              mate_by_dropping_pawn_w( to ) )
            {
              dests ^= Bit( to );
              continue;
            }
          moves[ count ] = MOVE( w_pawn, move_drop, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
    }

  return count;
}

int gen_drop_b( unsigned int moves[], int count )
{
  unsigned long dests;
  int to;

  if( IsHand_B( gold ) )
    {
      dests = ~Occupied0 & bb_mask;
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_gold, move_drop, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
    }
  
  if( IsHand_B( silver ) > 0 )
    {
      dests = ~Occupied0 & bb_mask;
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_silver, move_drop, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
    }

  
  if( IsHand_B( bishop ) )
    {
      dests = ~Occupied0 & bb_mask;
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_bishop, move_drop, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
    }

  if( IsHand_B( rook ) )
    {
      dests = ~Occupied0 & bb_mask;
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_rook, move_drop, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
    }
  
  if( IsHand_B( pawn ) )
    {
      dests = ~Occupied0 & bb_mask & ~( b1_1111 << 20 );
      to = FirstOne( BB_B_PAWN );
      if( to >= 0 )
        { dests &= ~DoublePawn[ to ]; }

      while( (to = FirstOne(dests)) != -1 )
        {
          if( to == SQ_W_KING - 5 &&  /* test "mate by dropping pawn" */
              mate_by_dropping_pawn_b( to ) ) 
            {
              dests ^= Bit( to );
              continue;
            }
          moves[ count ] = MOVE( b_pawn, move_drop, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
    }
  
  return count;
}

int gen_cap_w( unsigned int moves[], int count, unsigned int pin[] )
{
  /* ret: number of total moves */
  unsigned int dests, bb, att;
  int from, to, index;
  
  bb = BB_W_PAWN;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_WPawn[ from ] & ~nOccupiedB;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( to < 5 )
            { moves[ count ] = MOVE( w_pawn, from, to, 1, 1 ); }
          else
            { moves[ count ] = MOVE( w_pawn, from, to, 0, 1 ); }
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_PRO_PAWN;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_WGold[ from ] & ~nOccupiedB;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_pro_pawn, from, to, 0, 1 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_SILVER;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_WSilver[ from ] & ~nOccupiedB;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( from < 5 || to < 5 )/* inside of the opponent's territory */
            { moves[ count ] = MOVE( w_silver, from, to, 1, 1 );
              count ++;                                                     }
          moves[ count ] = MOVE( w_silver, from, to, 0, 1 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_PRO_SILVER;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_WGold[ from ] & ~nOccupiedB;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_pro_silver, from, to, 0, 1 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_GOLD;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_WGold[ from ] & ~nOccupiedB;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_gold, from, to, 0, 1 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_BISHOP;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Bishop_mask[from])
               * Attack_Bishop_magic[from])
              >> Attack_Bishop_shift[from]) & mask_magic;
      dests = Attack_Bishop[ from ][ index ];
      dests &= ~nOccupiedB & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( from < 5 || to < 5){
            moves[ count ] = MOVE( w_bishop, from, to, 1, 1 );
            count ++;
	    moves[ count ] = MOVE( w_bishop, from, to, 0, 1 );
            count ++;
          }
          else{
            moves[ count ] = MOVE( w_bishop, from, to, 0, 1 );
            count ++;
          }
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_ROOK;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Rook_mask[from]) * Attack_Rook_magic[from])
             >> Attack_Rook_shift[from]) & mask_magic;
      dests = Attack_Rook[ from ][ index ];
      dests &= ~nOccupiedB & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( from < 5 || to < 5){
            moves[ count ] = MOVE( w_rook, from, to, 1, 1 );
            count ++;
	    moves[ count ] = MOVE( w_rook, from, to, 0, 1 );
            count ++;
          }
          else{
            moves[ count ] = MOVE( w_rook, from, to, 0, 1 );
            count ++;
          }
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_HORSE;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Bishop_mask[from])
               * Attack_Bishop_magic[from])
              >> Attack_Bishop_shift[from]) & mask_magic;
      dests = Attack_Bishop[ from ][ index ];
      dests |= Attack_King[ from ];
      dests &= ~nOccupiedB & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_horse, from, to, 0, 1 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_DRAGON;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Rook_mask[from]) * Attack_Rook_magic[from])
             >> Attack_Rook_shift[from]) & mask_magic;
      dests = Attack_Rook[ from ][ index ];
      dests |= Attack_King[ from ];
      dests &= ~nOccupiedB & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_dragon, from, to, 0, 1 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }
  
  dests = Attack_King[ SQ_W_KING ] & ~nOccupiedB;
  while( (to = FirstOne( dests )) != -1 )
    {
      if( attacks_to_b( to, &att ) == 0 )
        {
          moves[ count ] = MOVE( w_king, SQ_W_KING, to, 0, 1 );
          count ++;
        }
      dests ^= Bit( to );
    }

  return count;
}


int gen_cap_b( unsigned int moves[], int count, unsigned int pin[] )
{
  /* ret: number of total moves */
  unsigned int dests, bb, att;
  int from, to, index;
  
  bb = BB_B_PAWN;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_BPawn[ from ] & ~nOccupiedW;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( to > 19 )
            { moves[ count ] = MOVE( b_pawn, from, to, 1, 1 ); }
          else
            { moves[ count ] = MOVE( b_pawn, from, to, 0, 1 ); }
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_PRO_PAWN;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_BGold[ from ] & ~nOccupiedW;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_pro_pawn, from, to, 0, 1 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }
  
  bb = BB_B_SILVER;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_BSilver[ from ] & ~nOccupiedW;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( from > 19 || to > 19 )/* inside of the opponent's territory */
            { moves[ count ] = MOVE( b_silver, from, to, 1, 1 );
              count ++;                                                     }
          moves[ count ] = MOVE( b_silver, from, to, 0, 1 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_PRO_SILVER;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_BGold[ from ] & ~nOccupiedW;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_pro_silver, from, to, 0, 1 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_GOLD;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_BGold[ from ] & ~nOccupiedW;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_gold, from, to, 0, 1 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_BISHOP;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Bishop_mask[from])
               * Attack_Bishop_magic[from])
              >> Attack_Bishop_shift[from]) & mask_magic;
      dests = Attack_Bishop[ from ][ index ];
      dests &= ~nOccupiedW & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( from > 19 || to > 19){
            moves[ count ] = MOVE( b_bishop, from, to, 1, 1 );
            count ++;
	    moves[ count ] = MOVE( b_bishop, from, to, 0, 1 );
            count ++;
          }
          else{
            moves[ count ] = MOVE( b_bishop, from, to, 0, 1 );
            count ++;
          }
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_ROOK;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Rook_mask[from]) * Attack_Rook_magic[from])
             >> Attack_Rook_shift[from]) & mask_magic;
      dests = Attack_Rook[ from ][ index ];
      dests &= ~nOccupiedW & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( from > 19 || to > 19){
            moves[ count ] = MOVE( b_rook, from, to, 1, 1 );
            count ++;
	    moves[ count ] = MOVE( b_rook, from, to, 0, 1 );
            count ++;
          }
          else{
            moves[ count ] = MOVE( b_rook, from, to, 0, 1 );
            count ++;
          }
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_HORSE;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Bishop_mask[from])
               * Attack_Bishop_magic[from])
              >> Attack_Bishop_shift[from]) & mask_magic;
      dests = Attack_Bishop[ from ][ index ];
      dests |= Attack_King[ from ];
      dests &= ~nOccupiedW & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_horse, from, to, 0, 1 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_DRAGON;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Rook_mask[from]) * Attack_Rook_magic[from])
             >> Attack_Rook_shift[from]) & mask_magic;
      dests = Attack_Rook[ from ][ index ];
      dests |= Attack_King[ from ];
      dests &= ~nOccupiedW & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_dragon, from, to, 0, 1 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }
  
  dests = Attack_King[ SQ_B_KING ] & ~nOccupiedW;
  while( (to = FirstOne( dests )) != -1 )
    {
      if( attacks_to_w( to, &att ) == 0 )
        {
          moves[ count ] = MOVE( b_king, SQ_B_KING, to, 0, 1 );
          count ++;
        }
      dests ^= Bit( to );
    }
  
  return count;
}


int gen_nocap_w( unsigned int moves[], int count, unsigned int pin[] )
{
  /* ret: number of generated moves */
  unsigned int dests, bb, att;
  int from, to, index;

  bb = BB_W_HORSE;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Bishop_mask[from])
               * Attack_Bishop_magic[from])
              >> Attack_Bishop_shift[from]) & mask_magic;
      dests = Attack_Bishop[ from ][ index ];
      dests |= Attack_King[ from ];
      dests &= ~Occupied0 & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_horse, from, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_DRAGON;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Rook_mask[from]) * Attack_Rook_magic[from])
             >> Attack_Rook_shift[from]) & mask_magic;
      dests = Attack_Rook[ from ][ index ];
      dests |= Attack_King[ from ];
      dests &= ~Occupied0 & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_dragon, from, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }
  
  bb = BB_W_SILVER;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_WSilver[ from ] & ~Occupied0;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( from < 5 || to < 5 )/* inside of the opponent's territory */
            { moves[ count ] = MOVE( w_silver, from, to, 1, 0 );
              count ++;                                                     }
          moves[ count ] = MOVE( w_silver, from, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_GOLD;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_WGold[ from ] & ~Occupied0;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_gold, from, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_PRO_SILVER;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_WGold[ from ] & ~Occupied0;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_pro_silver, from, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_BISHOP;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Bishop_mask[from])
               * Attack_Bishop_magic[from])
              >> Attack_Bishop_shift[from]) & mask_magic;
      dests = Attack_Bishop[ from ][ index ];
      dests &= ~Occupied0 & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( from < 5 || to < 5){
            moves[ count ] = MOVE( w_bishop, from, to, 1, 0 );
            count ++;
	    moves[ count ] = MOVE( w_bishop, from, to, 0, 0 );
            count ++;
          }
          else{
            moves[ count ] = MOVE( w_bishop, from, to, 0, 0 );
            count ++;
          }
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_ROOK;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Rook_mask[from]) * Attack_Rook_magic[from])
             >> Attack_Rook_shift[from]) & mask_magic;
      dests = Attack_Rook[ from ][ index ];
      dests &= ~Occupied0 & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( from < 5 || to < 5){
            moves[ count ] = MOVE( w_rook, from, to, 1, 0 );
            count ++;
	    moves[ count ] = MOVE( w_rook, from, to, 0, 0 );
            count ++;
          }
          else{
            moves[ count ] = MOVE( w_rook, from, to, 0, 0 );
            count ++;
          }
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_W_PRO_PAWN;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_WGold[ from ] & ~Occupied0;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( w_pro_pawn, from, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }
  
  bb = BB_W_PAWN;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_WPawn[ from ] & ~Occupied0;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( to < 5 )
            { moves[ count ] = MOVE( w_pawn, from, to, 1, 0 ); }
          else
            { moves[ count ] = MOVE( w_pawn, from, to, 0, 0 ); }
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }
  
  dests = Attack_King[ SQ_W_KING ] & ~Occupied0;
  while( (to = FirstOne( dests )) != -1 )
    {
      if( attacks_to_b( to, &att ) == 0 )
        {
          moves[ count ] = MOVE( w_king, SQ_W_KING, to, 0, 0 );
          count ++;
        }
      dests ^= Bit( to );
    }
  
  return count;
}

int gen_nocap_b( unsigned int moves[], int count, unsigned int pin[]  )
{
  /* ret: number of generated moves */
  unsigned int dests, bb, att;
  int from, to, index;

  bb = BB_B_HORSE;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Bishop_mask[from])
               * Attack_Bishop_magic[from])
              >> Attack_Bishop_shift[from]) & mask_magic;
      dests = Attack_Bishop[ from ][ index ];
      dests |= Attack_King[ from ];
      dests &= ~Occupied0 & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_horse, from, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_DRAGON;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Rook_mask[from]) * Attack_Rook_magic[from])
             >> Attack_Rook_shift[from]) & mask_magic;
      dests = Attack_Rook[ from ][ index ];
      dests |= Attack_King[ from ];
      dests &= ~Occupied0 & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_dragon, from, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }
  
  bb = BB_B_SILVER;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_BSilver[ from ] & ~Occupied0;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( from > 19 || to > 19 )/* inside of the opponent's territory */
            { moves[ count ] = MOVE( b_silver, from, to, 1, 0 );
              count ++;                                                     }
          moves[ count ] = MOVE( b_silver, from, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_GOLD;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_BGold[ from ] & ~Occupied0;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_gold, from, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_PRO_SILVER;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_BGold[ from ] & ~Occupied0;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_pro_silver, from, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_BISHOP;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Bishop_mask[from])
               * Attack_Bishop_magic[from])
              >> Attack_Bishop_shift[from]) & mask_magic;
      dests = Attack_Bishop[ from ][ index ];
      dests &= ~Occupied0 & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( from > 19 || to > 19){
            moves[ count ] = MOVE( b_bishop, from, to, 1, 0 );
            count ++;
	    moves[ count ] = MOVE( b_bishop, from, to, 0, 0 );
            count ++;
          }
          else{
            moves[ count ] = MOVE( b_bishop, from, to, 0, 0 );
            count ++;
          }
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_ROOK;
  while( (from = FirstOne(bb)) != -1 )
    {
      index =(((Occupied0 & Attack_Rook_mask[from]) * Attack_Rook_magic[from])
             >> Attack_Rook_shift[from]) & mask_magic;
      dests = Attack_Rook[ from ][ index ];
      dests &= ~Occupied0 & ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( from > 19 || to > 19){
            moves[ count ] = MOVE( b_rook, from, to, 1, 0 );
            count ++;
	    moves[ count ] = MOVE( b_rook, from, to, 0, 0 );
            count ++;
          }
          else{
            moves[ count ] = MOVE( b_rook, from, to, 0, 0 );
            count ++;
          }
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }

  bb = BB_B_PRO_PAWN;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_BGold[ from ] & ~Occupied0;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          moves[ count ] = MOVE( b_pro_pawn, from, to, 0, 0 );
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }
  
  bb = BB_B_PAWN;
  while( (from = FirstOne(bb)) != -1 )
    {
      dests = Attack_BPawn[ from ] & ~Occupied0;
      dests &= ~(pin[ from ]);
      while( (to = FirstOne(dests)) != -1 )
        {
          if( to > 19 )
            { moves[ count ] = MOVE( b_pawn, from, to, 1, 0 ); }
          else
            { moves[ count ] = MOVE( b_pawn, from, to, 0, 0 ); }
          count ++;
          dests ^= Bit( to );
        }
      bb ^= Bit( from );
    }
  
  dests = Attack_King[ SQ_B_KING ] & ~Occupied0;
  while( (to = FirstOne( dests )) != -1 )
    {
      if( attacks_to_w( to, &att ) == 0 )
        {
          moves[ count ] = MOVE( b_king, SQ_B_KING, to, 0, 0 );
          count ++;
        }
      dests ^= Bit( to );
    }

  return count;
}


void make_move_w( unsigned int move )
{  
  const int from    = MOVE_FROM( move );
  const int to      = MOVE_TO( move );
  const int type    = MOVE_TYPE( move );
  const int promote = move & move_mask_promote;
  const int cap     = move & move_mask_capture;
  int cap_type = 0;

  assert( type < 16 );

  if( from == move_drop )
    {
      W_HAND_A -= HAND_ADD( type );
      BB_N( type ) |= Bit( to );
      
      nOccupiedW ^= Bit( to );
      Occupied0  ^= Bit( to );

    }
  else if( cap )
    {
      cap_type = get_piece_on_sq_b( to );
      int cap_nopro = cap_type & mask_nopro;
      assert( cap_type != no_piece && cap_type != b_king );

      if( promote ) /* cap / pro */
        {
          BB_N( type ) ^= Bit( from );
          BB_N( type + m_promote ) ^= Bit( to );
          BB_N( cap_type ) ^= Bit( to );
          W_HAND_A += HAND_ADD( cap_nopro );

          nOccupiedW ^= Bit( from ) | Bit( to );
          nOccupiedB ^= Bit( to );
          Occupied0  ^= Bit( from );
        }
      else /* cap / no pro */
        {
          BB_N( type ) ^= Bit( from ) | Bit( to );
          BB_N( cap_type ) ^= Bit( to );
          W_HAND_A += HAND_ADD( cap_type & mask_nopro );

          nOccupiedW ^= Bit( from ) | Bit( to );
          nOccupiedB ^= Bit( to );
          Occupied0  ^= Bit( from );

        }
    }
  else
    {
      if( promote ) /* no cap / pro */
        {
          BB_N( type ) ^= Bit( from );
          BB_N( type + m_promote ) ^= Bit( to );

          nOccupiedW ^= Bit( from ) | Bit( to );
          Occupied0  ^= Bit( from ) | Bit( to );
        }
      else /* no cap / no pro */
        {
          BB_N( type ) ^= Bit( from ) | Bit( to );

          nOccupiedW ^= Bit( from ) | Bit( to );
          Occupied0  ^= Bit( from ) | Bit( to );
        }
    }
  SQ_W_KING = FirstOne( BB_W_KING );
  assert( SQ_W_KING >= 0 );

  history[N_PLY].move =
    ( move & ~move_mask_captured ) + CAPTURED_2_MOVE( cap_type );

  FLIP_TURN;
  N_PLY ++;

  return;
}

void make_move_b( unsigned int move )
{
  const int from    = MOVE_FROM( move );
  const int to      = MOVE_TO( move );
  const int type    = MOVE_TYPE( move );
  const int promote = move & move_mask_promote;
  const int cap     = move & move_mask_capture;
  int cap_type = 0;
  
  assert( type >= 16 );

  if( from == move_drop )
    {
      B_HAND_A -= HAND_ADD( type & mask_nopro );
      BB_N( type ) |= Bit( to );

      nOccupiedB  ^= Bit( to );
      Occupied0   ^= Bit( to );
    }
  else if( cap )
    {
      cap_type = get_piece_on_sq_w( to );
      int cap_nopro = cap_type & mask_nopro;
      assert( cap_type != no_piece && cap_type != w_king );

      if( promote ) /* cap / pro */
        {
          BB_N( type ) ^= Bit( from );
          BB_N( type + m_promote ) ^= Bit( to );
          BB_N( cap_type ) ^= Bit( to );
          B_HAND_A += HAND_ADD( cap_nopro );

          nOccupiedB ^= Bit( from ) | Bit( to );
          nOccupiedW ^= Bit( to );
          Occupied0  ^= Bit( from );
        }
      else /* cap / no pro */
        {
          BB_N( type ) ^= Bit( from ) | Bit( to );
          BB_N( cap_type ) ^= Bit( to );
          B_HAND_A += HAND_ADD( cap_type & mask_nopro );

          nOccupiedB ^= Bit( from ) | Bit( to );
          nOccupiedW ^= Bit( to );
          Occupied0  ^= Bit( from );
        }
    }
  else
    {
      if( promote ) /* no cap / pro */
        {
          BB_N( type ) ^= Bit( from );
          BB_N( type + m_promote ) ^= Bit( to );

          nOccupiedB ^= Bit( from ) | Bit( to );
          Occupied0  ^= Bit( from ) | Bit( to );
        }
      else /* no cap / no pro */
        {
          BB_N( type ) ^= Bit( from ) | Bit( to );

          nOccupiedB ^= Bit( from ) | Bit( to );
          Occupied0  ^= Bit( from ) | Bit( to );
        }
    }
  SQ_B_KING = FirstOne( BB_B_KING );
  assert( SQ_B_KING >= 0 );
  
  history[N_PLY].move =
    ( move & ~move_mask_captured ) + CAPTURED_2_MOVE( cap_type );

  FLIP_TURN;
  N_PLY ++;

  return;
}

void unmake_move_w()
{
  FLIP_TURN;
  N_PLY --;
  int move = history[N_PLY].move;
  
  const int from     = MOVE_FROM( move );
  const int to       = MOVE_TO( move );
  const int type     = MOVE_TYPE( move );
  const int promote  = move & move_mask_promote;
  const int cap_type = MOVE_CAPTURED_TYPE( move );

  if( from == move_drop )
    {
      W_HAND_A += HAND_ADD( type );
      BB_N( type ) ^= Bit( to );

      nOccupiedW ^= Bit( to );
      Occupied0  ^= Bit( to );
    }
  else if( cap_type )
    {
      if( promote ) /* cap / pro */
        {
          BB_N( type ) ^= Bit( from );
          BB_N( type + m_promote ) ^= Bit( to );
          BB_N( cap_type ) ^= Bit( to );
          W_HAND_A -= HAND_ADD( cap_type & mask_nopro );

          nOccupiedW ^= Bit( from ) | Bit( to );
          nOccupiedB ^= Bit( to );
          Occupied0  ^= Bit( from );
        }
      else /* cap / no pro */
        {
          BB_N( type ) ^= Bit( from ) | Bit( to );
          BB_N( cap_type ) ^= Bit( to );
          W_HAND_A -= HAND_ADD( cap_type & mask_nopro );

          nOccupiedW ^= Bit( from ) | Bit( to );
          nOccupiedB ^= Bit( to );
          Occupied0  ^= Bit( from );
        }
    }
  else
    {
      if( promote ) /* no cap / pro */
        {
          BB_N( type ) ^= Bit( from );
          BB_N( type + m_promote ) ^= Bit( to );

          nOccupiedW ^= Bit( from ) | Bit( to );
          Occupied0  ^= Bit( from ) | Bit( to );
        }
      else /* no cap / no pro */
        {
          BB_N( type ) ^= Bit( from ) | Bit( to );

          nOccupiedW ^= Bit( from ) | Bit( to );
          Occupied0  ^= Bit( from ) | Bit( to );
        }
    }
  SQ_W_KING = FirstOne( BB_W_KING );

  return;
}

void unmake_move_b()
{
  FLIP_TURN;
  N_PLY --;
  int move = history[N_PLY].move;
  
  const int from     = MOVE_FROM( move );
  const int to       = MOVE_TO( move );
  const int type     = MOVE_TYPE( move );
  const int promote  = move & move_mask_promote;
  const int cap_type = MOVE_CAPTURED_TYPE( move );

  if( from == move_drop )
    {
      B_HAND_A += HAND_ADD( type & mask_nopro );
      BB_N( type ) ^= Bit( to );
      
      nOccupiedB ^= Bit( to );
      Occupied0  ^= Bit( to );
    }
  else if( cap_type )
    {
      if( promote ) /* cap / pro */
        {
          BB_N( type ) ^= Bit( from );
          BB_N( type + m_promote ) ^= Bit( to );
          BB_N( cap_type ) ^= Bit( to );
          B_HAND_A -= HAND_ADD( cap_type & mask_nopro );

          nOccupiedB ^= Bit( from ) | Bit( to );
          nOccupiedW ^= Bit( to );
          Occupied0  ^= Bit( from );
        }
      else /* cap / no pro */
        {
          BB_N( type ) ^= Bit( from ) | Bit( to );
          BB_N( cap_type ) ^= Bit( to );
          B_HAND_A -= HAND_ADD( cap_type & mask_nopro );

          nOccupiedB ^= Bit( from ) | Bit( to );
          nOccupiedW ^= Bit( to );
          Occupied0  ^= Bit( from );
        }
    }
  else
    {
      if( promote ) /* no cap / pro */
        {
          BB_N( type ) ^= Bit( from );
          BB_N( type + m_promote ) ^= Bit( to );

          nOccupiedB ^= Bit( from ) | Bit( to );
          Occupied0  ^= Bit( from ) | Bit( to );
        }
      else /* no cap / no pro */
        {
          BB_N( type ) ^= Bit( from ) | Bit( to );

          nOccupiedB ^= Bit( from ) | Bit( to );
          Occupied0  ^= Bit( from ) | Bit( to );
        }
    }
  SQ_B_KING = FirstOne( BB_B_KING );

  return;
}


int FirstOne( int bb )
{
  return bb ? __builtin_ctz( bb ) : -1;
}

int get_piece_on_sq_w( int sq )
{
#define TEST( piece ) \
  if( BB_N( piece ) & Bit( sq ) ) \
    return piece; \

  TEST( w_silver );
  TEST( w_gold );
  TEST( w_bishop );
  TEST( w_rook );
  TEST( w_pro_silver );
  TEST( w_horse );
  TEST( w_dragon );
  TEST( w_pawn );
  TEST( w_pro_pawn );
  if( SQ_W_KING == sq )
    return w_king;
  
  return no_piece;
}

int get_piece_on_sq_b( int sq )
{
  TEST( b_silver );
  TEST( b_gold );
  TEST( b_bishop );
  TEST( b_rook );
  TEST( b_pro_silver );
  TEST( b_horse );
  TEST( b_dragon );
  TEST( b_pawn );
  TEST( b_pro_pawn );
  if( SQ_B_KING == sq )
    return b_king;

  return no_piece;
#undef TEST
}

int get_piece_on_sq( int sq )
{
  int ret;
  
  if( (ret = get_piece_on_sq_w( sq )) != no_piece )
    { return ret; }

  return get_piece_on_sq_b( sq );
}

#define FILE_READ( variable, size ) \
  if( fread( variable, sizeof( unsigned int ), size, fp) != size ){ \
    printf(" fread \"%s\" failed.\n", #variable); \
    fclose( fp ); return -1; }\

int starting_initialize()
{
  /*
    return -1: failed
            0: succeeded
   */
  
  FILE *fp;

  fp = fopen("BB_Attack.bin","rb");

  FILE_READ( Attack_WPawn, 32 );
  FILE_READ( Attack_WSilver, 32 );
  FILE_READ( Attack_WGold, 32 );
  FILE_READ( Attack_BPawn, 32 );
  FILE_READ( Attack_BSilver, 32 );
  FILE_READ( Attack_BGold, 32 );
  FILE_READ( Attack_King, 32 );
  FILE_READ( Attack_Rook, 32 * 64 );
  FILE_READ( Attack_Rook_magic, 32 );
  FILE_READ( Attack_Rook_mask, 32 );
  FILE_READ( Attack_Bishop, 32 * 64 );
  FILE_READ( Attack_Bishop_magic, 32 );
  FILE_READ( Attack_Bishop_mask, 32 );
  FILE_READ( DoublePawn, 32 );
  FILE_READ( Pin_Rook, 32 * 32 * 64 * 2 );
  FILE_READ( Pin_Bishop, 32 * 32 * 64 * 2 );
  
  fclose( fp );

  return 0;
}

#undef FILE_READ

void clear_game()
{
  TURN      = white;
  N_PLY  = 0;

  memset( BB_ALL, 0, sizeof(BB_ALL) );
  W_HAND_A = 0;
  B_HAND_A = 0;

  BB_W_PAWN       = Bit( XY2INDEX( 5, 4 ) );
  BB_W_SILVER     = Bit( XY2INDEX( 3, 5 ) );
  BB_W_GOLD       = Bit( XY2INDEX( 4, 5 ) );
  BB_W_BISHOP     = Bit( XY2INDEX( 2, 5 ) );
  BB_W_ROOK       = Bit( XY2INDEX( 1, 5 ) );
  BB_W_KING       = Bit( XY2INDEX( 5, 5 ) );
  SQ_W_KING       =      XY2INDEX( 5, 5 )  ;
  BB_W_PRO_PAWN   = 0;
  BB_W_PRO_SILVER = 0;
  BB_W_HORSE      = 0;
  BB_W_DRAGON     = 0;

  BB_B_PAWN       = Bit( XY2INDEX( 1, 2 ) );
  BB_B_SILVER     = Bit( XY2INDEX( 3, 1 ) );
  BB_B_GOLD       = Bit( XY2INDEX( 2, 1 ) );
  BB_B_BISHOP     = Bit( XY2INDEX( 4, 1 ) );
  BB_B_ROOK       = Bit( XY2INDEX( 5, 1 ) );
  BB_B_KING       = Bit( XY2INDEX( 1, 1 ) );
  SQ_B_KING       =      XY2INDEX( 1, 1 )  ;
  BB_B_PRO_PAWN   = 0;
  BB_B_PRO_SILVER = 0;
  BB_B_HORSE      = 0;
  BB_B_DRAGON     = 0;

  calc_occupied_sq();
  
  return;
}

void calc_occupied_sq()
{  
  int i,index, tmp;
  
  nOccupiedW = bb_mask;
  nOccupiedB = bb_mask;
  Occupied0  = 0;

  for( i=0; i < bbs_size; i++ )
    {
      if( i == w_king )
        { nOccupiedW &= ~Bit( SQ_W_KING );
          Occupied0   ^= Bit( SQ_W_KING );
          continue;                       }
      else if(i == b_king )
        { nOccupiedB &= ~Bit( SQ_B_KING );
          Occupied0   ^= Bit( SQ_B_KING );
          continue;                       }
      tmp = BB_N(i);
      while( (index = FirstOne( tmp ) )  >= 0 )
        {
          if( i < (bbs_size / 2) )
            nOccupiedW &= ~Bit(index);
          else
            nOccupiedB &= ~Bit(index);
          Occupied0   ^= Bit( index );
          tmp ^= Bit(index);
        }
    }

  return;
}


int popuCount( int piece )
{
  int count = 0;
  unsigned bb = BB_N( piece );
  int b;
  
  while( ( b = FirstOne( bb ) ) != -1 )
    {
      count ++;
      bb &= ~Bit( b );
    }

  return count;
}

int get_turn()
{
  return TURN ? black : white ; 
}

int get_nply()
{
  return N_PLY;
}

