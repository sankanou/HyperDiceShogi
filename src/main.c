#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "header.h"


int main( int argc, char *argv[] )
{
  int ret;

  out(" %s  ver. %s  \n\n", PROGRAM_NAME, VERSION );

  if( starting_initialize() )
    { exit(1); }
  game_initialize();

  out_position();
  
  while( 1 )
    {
      ret = cmd_prompt();

      if( ret == -1 )
        close_program();
    }
}


void close_program()
{
  game_finalize();
  exit(0);
}
