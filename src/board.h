//よくある用語
//W: 白, B: 黒, チェス由来であり，先手を白，後手を黒とする
//sq: squareの略でマスを表す
//ply: チェスなどで用いられる,ターンに似た概念
//一人分の行動は1プライと呼ぶ．二人のplyを合わせて1ターンとする場合もある．
//例: チェスにおける20ターンは,白20, 黒20の40プライと等しい
//bb: ビットボード
//move: 指し手
//hand: 持ち駒


#define RECORD_SIZE        256
#define SIZE_LEGALMOVES    256
#define SIZE_DICE_LEGALMOVES    64
#define DICE_NUM        6

//ゲームの状態を示す構造体
typedef struct tree tree_t;
struct tree
{
  unsigned int BBs[32]; //双方の盤面の状態
  unsigned int whand; //白番の持ち駒
  unsigned int bhand; //黒番の持ち駒
  int sq_wking; //白の玉の位置
  int sq_bking; //黒の玉の位置
  char turn; //現在の手番
  short n_ply; //現在の手数
};

//指し手の履歴ひとつ分を示す構造体
typedef struct
{
  unsigned int move;
  int dice;
} hist_t;

//gameに，ゲームの全状態を格納しておく。
//historyは，指し手の配列。これまでの手を登録する?
extern tree_t game;
extern hist_t history[ RECORD_SIZE ];
//2進数を表現する.binary
enum { b0000, b0001, b0010, b0011, b0100, b0101, b0110, b0111,
       b1000, b1001, b1010, b1011, b1100, b1101, b1110, b1111,
       b1_1111 = 31  };

// 駒の状態を表す番号．
// 4番目のビットを立てる事で成る事ができる．
// pro_pawn == pawn | m_promote
// b(黒)の駒は5番目のビットが立っている．
enum{
  m_promote    =  b1000,

  pawn         =  1,
  silver       =  4,
  gold         =  5,
  bishop       =  6,
  rook         =  7,
  king         =  8,
  pro_pawn     =  9,
  pro_silver   = 12,
  horse        = 14,
  dragon       = 15,

  w_pawn       =  1,
  w_silver     =  4,
  w_gold       =  5,
  w_bishop     =  6,
  w_rook       =  7,
  w_king       =  8,
  w_pro_pawn   =  9,
  w_pro_silver = 12,
  w_horse      = 14,
  w_dragon     = 15,

  b_pawn       = 17,
  b_silver     = 20,
  b_gold       = 21,
  b_bishop     = 22,
  b_rook       = 23,
  b_king       = 24,
  b_pro_pawn   = 25,
  b_pro_silver = 28,
  b_horse      = 30,
  b_dragon     = 31,

  bbs_size
};
// 指し手情報の取得に使うマジックナンバー
// 指し手の変数に，マスクをかけて欲しい情報を取得する
// move & b1_1111という使い方で，最右の5ビットだけ抜き出すことができる
// 例: ..0000 1111 1011 1101 & 1 1111 => ..0000 0000 0001 1101
// 最右から5ビットが駒の種類，次の5ビットが動く前の位置…と表現され
// 変数ごとに左シフトでマスクをかける位置をずらしている．
enum{
  move_drop          =  25, //駒打ちを示す番号
  move_mask_type     = (b1_1111 << 0 ),
  move_mask_from     = (b1_1111 << 5 ),
  move_mask_to       = (b1_1111 << 10),
  move_mask_promote  = (b0001   << 15),
  move_mask_capture  = (b0001   << 16),
  move_mask_captured = (b1_1111   << 17),
};
//便利マスク。欲しいところと&演算することで必要な値を取ることができる
enum{
  bb_mask            =  0x01ffffff,//盤面の計算対象の25bit分マスクする
  mask_magic         =  0x0000003f,//マジックナンバー計算用の6bit分マスク
  mask_type          =  b1111,//利用無し
  mask_type_c        =  b1_1111,//利用無し
  mask_nopro         =  b0111,//駒の種類について3bitをマスクすれば成る前の駒種がとれる
  mask_piece_color   = (b0001 << 4),//駒の手番は5bit目をマスクすればとれる
  white              =  0,
  black              =  1,
  myturn_not_set     =  -1,
  no_piece           =  -1
};

//ビットボードへのアクセスを便利にするためのマクロ
//たとえばBB_W_PAWNとすると，白番の歩の状態を表すビットボード変数が取れる
#define BB_ALL           (game.BBs)
#define BB_N(n)          (game.BBs[n])

#define BB_W_PAWN        (game.BBs[w_pawn])
#define BB_W_SILVER      (game.BBs[w_silver])
#define BB_W_GOLD        (game.BBs[w_gold])
#define BB_W_BISHOP      (game.BBs[w_bishop])
#define BB_W_ROOK        (game.BBs[w_rook])
#define BB_W_PRO_PAWN    (game.BBs[w_pro_pawn])
#define BB_W_PRO_SILVER  (game.BBs[w_pro_silver])
#define BB_W_HORSE       (game.BBs[w_horse])
#define BB_W_DRAGON      (game.BBs[w_dragon])
#define BB_W_KING        (game.BBs[w_king])
#define SQ_W_KING        (game.sq_wking)

#define BB_B_PAWN        (game.BBs[b_pawn])
#define BB_B_SILVER      (game.BBs[b_silver])
#define BB_B_GOLD        (game.BBs[b_gold])
#define BB_B_BISHOP      (game.BBs[b_bishop])
#define BB_B_ROOK        (game.BBs[b_rook])
#define BB_B_PRO_PAWN    (game.BBs[b_pro_pawn])
#define BB_B_PRO_SILVER  (game.BBs[b_pro_silver])
#define BB_B_HORSE       (game.BBs[b_horse])
#define BB_B_DRAGON      (game.BBs[b_dragon])
#define BB_B_KING        (game.BBs[b_king])
#define SQ_B_KING        (game.sq_bking)

#define W_HAND_A          (game.whand)
#define B_HAND_A          (game.bhand)
#define IsHand_W(piece)   (W_HAND_A & ( b0011 << ((piece)*2) ))
#define IsHand_B(piece)   (B_HAND_A & ( b0011 << ((piece)*2) ))
#define W_HAND(piece)     (int)((W_HAND_A >> ((piece)*2)) & b0011)
#define B_HAND(piece)     (int)((B_HAND_A >> ((piece)*2)) & b0011)
#define HAND_ADD(piece)   (1 << ((piece)*2))
/* hand
  ........ ........ ....xx.. pawn
  ........ ......xx ........ silver
  ........ ....xx.. ........ gold
  ........ ..xx.... ........ bishop
  ........ xx...... ........ rook
  */

#define TURN             (game.turn)
#define N_PLY            (game.n_ply)
//ユーティリティ関数
//x番目を立てたビットを用意する
#define Bit(x)           ( 1 << x )
//x, yによる座標系から変換し，盤面の添え字を引く．
#define XY2INDEX(x,y)    ( (5-x) + (y-1)*5 )
//
#define INVERSE(x)       ( 24 - x )

//始めにビットが立っている位置を取得する
#define FIRSTONE(bb)   (31 - __builtin_clz( bb )) /* ret -1: bb = 0 */

/* move 
  ........ ........ ...xxxxx piece to move
  ........ ......xx xxx..... starting square ( 25-> drop )
  ........ .xxxxx.. ........ destination
  ........ x....... ........ promote or not
  .......x ........ ........ capture something
  xxxxxxx. ........ ........ captured piece  (0 -> unknown)
 */

//指し手から情報を取得するマクロ
//上記のマスクを用いて取得している
//駒の種類，移動前・後，成ったか否か，駒を取ったか否か，取った駒の種類
//成ったか否か?成っているか否か？は要調査
#define MOVE_TYPE(move)           ( (move & move_mask_type)         )
#define MOVE_FROM(move)           ( (move & move_mask_from)    >> 5 )
#define MOVE_TO(move)             ( (move & move_mask_to)      >> 10)
#define MOVE_PROMOTE(move)        ( (move & move_mask_promote) >> 15)
#define MOVE_CAPTURE(move)        ( (move & move_mask_capture) >> 16)
#define MOVE_CAPTURED_TYPE(move)  ( (move & move_mask_captured)>> 17)
//NULLの指し手を表現
#define MOVE_NULL                 0

//捕獲した駒の種類を格納する位置に対応させるためのビットシフト
#define CAPTURED_2_MOVE(cap)      ( cap << 17 )

//ビットボード生成時に，それぞれ該当する桁にそろえるようにビットシフトする. 
//駒を取らない場合の指し手を表すビットボードを生成する
#define MOVE(v1, v2, v3, v4, v5) ( (v1) + (v2 << 5) + (v3 << 10) +\
                                   (v4 << 15) + (v5 << 16) + (0 << 17) )
//駒を取る場合の指し手を表すビット列を生成する．
#define MOVE_C(v1, v2, v3, v4, v5, v6)\
                                   ( (v1) + (v2 << 5) + (v3 << 10) +\
                                   (v4 << 15) + (v5 << 16) + (v6 << 17) )

//指し手を反映させるマクロ
//現在の手番のプレイヤのmake_moveを呼び出す．
#define MAKE_MOVE(move)       TURN ? make_move_b( move ) : make_move_w( move );
#define UNMAKE_MOVE           TURN ? unmake_move_w() : unmake_move_b();
//手番を入れ替える．black^blackならwhiteに。white^blackならblackになる．
#define FLIP_TURN             TURN ^= black;

//calc_occupied_sq()にてbb_mask初期化
//make_move_wにて，駒の移動先にて
extern unsigned int nOccupiedW;
extern unsigned int nOccupiedB;
extern unsigned int Occupied0;

//starting_initialize()にてFILE_READされている．
extern unsigned int Attack_WPawn[32];
extern unsigned int Attack_WSilver[32];
extern unsigned int Attack_WGold[32];
extern unsigned int Attack_BPawn[32];
extern unsigned int Attack_BSilver[32];
extern unsigned int Attack_BGold[32];
extern unsigned int Attack_King[32];
extern unsigned int Attack_Rook[32][64];
extern unsigned int Attack_Rook_magic[32];
extern unsigned int Attack_Rook_mask[32];
extern const    int Attack_Rook_shift[32];
extern unsigned int Attack_Bishop[32][64];
extern unsigned int Attack_Bishop_magic[32];
extern unsigned int Attack_Bishop_mask[32];
extern const    int Attack_Bishop_shift[32];
extern unsigned int Pin_Rook[32][32][64][2];
extern unsigned int Pin_Bishop[32][32][64][2];
extern unsigned int DoublePawn[32];

//ここからライブラリの用意する関数群

//ゲーム状態の変更
int starting_initialize(); //利き情報の初期化
void clear_game(); //盤面・手番・持ち駒を初期状態にリセット
void make_move_w( unsigned int move );
void make_move_b( unsigned int move );
void unmake_move_w( );
void unmake_move_b( );
//指し手生成に関わる関数
int gen_legalmoves( unsigned legalmoves[] );
int gen_dicelegalmoves( unsigned legalmoves[][64], int nmove[] );
int gen_dicenum_legalmoves( unsigned int moves[], int dice );
int gen_evasion_w( unsigned int moves[], int count, int nAttacks,
                   unsigned int attack_pieces, unsigned int pin[] );
int gen_evasion_b( unsigned int moves[], int count, int nAttacks,
                   unsigned int attack_pieces, unsigned int pin[] );
int gen_cap_w( unsigned int moves[], int count, unsigned int pin[] );
int gen_cap_b( unsigned int moves[], int count, unsigned int pin[] );
int gen_nocap_w( unsigned int moves[], int count, unsigned int pin[] );
int gen_nocap_b( unsigned int moves[], int count, unsigned int pin[] );
int gen_drop_w( unsigned int moves[], int count );
int gen_drop_b( unsigned int moves[], int count );
int gen_attacks_to_w( unsigned int moves[], int count, int sq, int cap, unsigned int pin[] );
int gen_attacks_to_b( unsigned int moves[], int count, int sq, int cap, unsigned int pin[] );
int mate_by_dropping_pawn_w( int sq );
int mate_by_dropping_pawn_b( int sq );
//王手をかけている位置の数を返す
int attacks_to_w( int sq, unsigned int *attack_pieces );
int attacks_to_b( int sq, unsigned int *attack_pieces );
void pinInfo_w( unsigned int pin[32] );
void pinInfo_b( unsigned int pin[32] );
//始めにビットが立っている位置を返す.FIRSTONEマクロとの違いは？
int FirstOne( int bb );
//マスに存在する駒
int get_piece_on_sq_w( int sq );
int get_piece_on_sq_b( int sq );
int get_piece_on_sq( int sq );
//駒のある位置全てを計算して書き込む
void calc_occupied_sq();
int popuCount( int piece );
//現在の手番(TURN)をblackかwhiteで返す
int get_turn();
//N_PLYを返す
int get_nply();
//王手判定
int is_mated();
//historyにサイコロの目を追加
void history_dice( int dice );
