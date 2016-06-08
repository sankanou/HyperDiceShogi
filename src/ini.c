#include <stdio.h>
#include <string.h>
#include "header.h"


void game_initialize()
{
  int i, iret;

  game_finalize();
  clear_game();
  
  char str_file[ SIZE_FILENAME ];
  for ( i = 0; i < 1000; i++ )
    {
      snprintf( str_file, SIZE_FILENAME, "../log/game%03d.log", i );
      file_log = fopen( str_file, "r" );
      if ( file_log == NULL ) { break; }
      iret = fclose( file_log );
      if ( iret < 0 )
        {
          file_log = NULL;
          i = -1;
          break;
        }
    }

  if( i >= 0 )
    { file_log = fopen( str_file, "w" ); }

  return;
}

void game_finalize()
{
  
  if( file_log )
    { fclose( file_log ); }

  return;
}
