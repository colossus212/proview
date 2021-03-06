/** 
 * Proview   Open Source Process Control.
 * Copyright (C) 2005-2016 SSAB EMEA AB.
 *
 * This file is part of Proview.
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation, either version 2 of 
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with Proview. If not, see <http://www.gnu.org/licenses/>
 *
 * Linking Proview statically or dynamically with other modules is
 * making a combined work based on Proview. Thus, the terms and 
 * conditions of the GNU General Public License cover the whole 
 * combination.
 *
 * In addition, as a special exception, the copyright holders of
 * Proview give you permission to, from the build function in the
 * Proview Configurator, combine Proview with modules generated by the
 * Proview PLC Editor to a PLC program, regardless of the license
 * terms of these modules. You may copy and distribute the resulting
 * combined work under the terms of your choice, provided that every 
 * copy of the combined work is accompanied by a complete copy of 
 * the source code of Proview (the version used to produce the 
 * combined work), being distributed under the terms of the GNU 
 * General Public License plus this exception.
 **/

/* co_dcli_input.c 
   Commandline input. */


/*_Include files_________________________________________________________*/

#if defined(OS_ELN)
# include stdio
# include stdlib
# include chfdef
# include string
# include starlet
# include iodef   
# include descrip
# include stdarg
# include $vaxelnc
# include $elnmsg
# include $kernelmsg
# include $dda_utility

# define STDIN_IDX  1
# define STDOUT_IDX 2
# define STDERR_IDX 3
#elif defined(OS_VMS)
# include <stdio.h>
# include <stdlib.h>
# include <chfdef.h>
# include <string.h>
# include <starlet.h>
# include <iodef.h>   
# include <descrip.h>
# include <ssdef.h>
# include <stdarg.h>
#elif defined OS_POSIX
# include <stdio.h>
# include <string.h>
# include <termios.h>
# include <sys/types.h>
# include <sys/file.h>
# include <unistd.h>
# include <fcntl.h>
# include <stdarg.h>
#endif

#define DCLI_QIO_RETRY 10

#include "pwr.h"
#include "pwr_class.h"
#include "co_dcli.h"
#include "co_dcli_input.h"
#include "co_dcli_msg.h"

/***********************  D E F I N E ' S *******************************/
#define ODD(a)	(((int)(a) & 1) != 0)
#define EVEN(a)	(((int)(a) & 1) == 0)

#define	DCLI_K_MAXLEN		0

#define DCLI_TERM		1000
#define	DCLI_K_NONE	 	0
#define DCLI_K_DELETE		127
#define	DCLI_K_ARROW_UP 	274
#define	DCLI_K_ARROW_DOWN 	275
#define	DCLI_K_ARROW_RIGHT 	276
#define	DCLI_K_ARROW_LEFT 	277
#define	DCLI_K_TIMEOUT		282
#if defined OS_POSIX
# define DCLI_K_RETURN 		10
#else
# define DCLI_K_RETURN 		13
#endif
#define	DCLI_K_BACKSPACE	8
#define	DCLI_K_CTRLC		3
#define	DCLI_K_ESCAPE		27

static unsigned short state_table[3][256];

/***********************  T Y P E D E F ' S *****************************/

/*__Local function prototypes_________________________________________*/

static int	r_print( dcli_sChannel *chn, char *format, ...)
{
	char	buff[200];
	int	sts;
	va_list ap;
	int	len;

	va_start( ap, format);
	len = vsprintf( buff, format, ap);
	va_end( ap);
	sts = dcli_qio_writew( chn, buff, len);
	return sts;
}

/*************************************************************************
*
* Name:		init_state_table()
*
* Type		int
*
* Type		Parameter	IOGF	Description
*
* Description:
*	Initialization of the stat_table used by dcli_get_input.
*
**************************************************************************/

static int init_state_table()
{
  static int done = 0;

  if ( !done)
  {
    state_table[0][DCLI_K_RETURN] = DCLI_TERM + DCLI_K_RETURN;
    state_table[0][DCLI_K_CTRLC] = DCLI_TERM + DCLI_K_CTRLC;
    state_table[0][DCLI_K_DELETE] = DCLI_TERM + DCLI_K_DELETE;
    state_table[0][DCLI_K_BACKSPACE] = DCLI_TERM + DCLI_K_BACKSPACE;
    state_table[0][DCLI_K_ESCAPE] = 1;
    state_table[0][155] = 2;
    state_table[1][91] = 2;		/* state ESCAPE */
    state_table[2][65] = DCLI_TERM + DCLI_K_ARROW_UP;
    state_table[2][66] = DCLI_TERM + DCLI_K_ARROW_DOWN;
    state_table[2][67] = DCLI_TERM + DCLI_K_ARROW_RIGHT;
    state_table[2][68] = DCLI_TERM + DCLI_K_ARROW_LEFT;
    done = 1;
  }
  return DCLI__SUCCESS;
}		

static void	store_cursorpos( dcli_sChannel *chn)
{
  char	char_ins[]={27, 55, 0};
	
  r_print( chn,"%s", char_ins);
}

static void	restore_cursorpos( dcli_sChannel *chn)
{
  char	char_ins[]={27, 56, 0};
	
  r_print( chn,"%s", char_ins);
}

static void	eofline_erase(dcli_sChannel *chn)
{
	char	char_ins[]={27, 91, 48, 75, 0};

	r_print( chn,"%s", char_ins);
}

static void	cursor_rel(	dcli_sChannel *chn,
			 	int	x,
				int	y)
{
  char	cursor_f[]={27, 91, 0, 0, 67, 0};
  char	cursor_b[]={27, 91, 0, 0, 68, 0};
  char	cursor_u[]={27, 91, 0, 0, 65, 0};
  char	cursor_d[]={27, 91, 0, 0, 66, 0};
	
  if ( y > 0 )
  {
    cursor_f[2] = y / 10 + 48;
    cursor_f[3] = y - (y / 10) * 10 + 48;
    r_print( chn,"%s", cursor_f);
  }
  else if ( y < 0 )
  {
    cursor_b[2] = - y / 10 + 48;
    cursor_b[3] = - y + (y / 10) * 10 + 48;
    r_print( chn,"%s", cursor_b);
  }
  if ( x > 0 )
  {
    cursor_u[2] = x / 10 + 48;
    cursor_u[3] = x - (x / 10) * 10 + 48;
    r_print( chn,"%s", cursor_u);
  }
  else if ( x < 0 )
  {
    cursor_d[2] = - x / 10 + 48;
    cursor_d[3] = - x + (x / 10) * 10 + 48;
    r_print( chn,"%s", cursor_d);
  }
}

static void	char_delete( dcli_sChannel *chn, int	n)
{
  char	char_del[5]={27, 91, 0, 80, 0};
	
  char_del[2] = n;
  r_print( chn,"%s", char_del);
}

static void	char_insert_nob( dcli_sChannel *chn, int	n)
{
	char	char_ins[]={27, 91, 0, 0, 64, 0};
	
	char_ins[2] = n / 10 + 48;
	char_ins[3] = n - (n / 10) * 10 + 48;
	r_print( chn,"%s", char_ins);
}

int	dcli_get_input( dcli_sChannel	*chn,
			char		*input_str,
			unsigned long	*terminator,
			int		maxlen,
			unsigned long	option,
			int		timeout)
{
  unsigned char	c;
  char	*input_ptr;
  int	i;
  int	sts;
  int	state;

	  
  input_ptr = input_str;

  for ( i = 0; i < maxlen; i++)
  {
    if ( (option & DCLI_OPT_TIMEOUT) == 0)
      dcli_qio_readw( chn, (char *) &c, 1);
    else
    {
      sts = dcli_qio_read( chn, timeout, (char *) &c, 1);
      if ( !sts)
      {
        /* Timeout */
        *terminator = DCLI_K_TIMEOUT;
        *input_ptr = '\0';
        return 1;
      }
    }	   
 
    state = DCLI_TERM;
    while ( state > 0 )
    {
      if ( state == DCLI_TERM)
        /* first time */
        state = 0;

      state = state_table[ state ][ c ];	  
      if ( state > DCLI_TERM )
      {
        *terminator = state - DCLI_TERM;
        switch ( *terminator)
        {
          case DCLI_K_RETURN:
          case DCLI_K_CTRLC:
          {
            *input_ptr = '\0';
            if ( ((option & DCLI_OPT_NOECHO) == 0) &&
  	         ((option & DCLI_OPT_NOSCROLL) == 0)) 
              r_print( chn,"\n\r");
            return 1;
          }	        
          default:
          {
            *input_ptr = '\0';
            return 1;
          }	        
        }
      }
      else if ( state > 0)
        dcli_qio_readw( chn, (char *) &c, 1);
    }

    if ( c > 31)
    {
      /* Some ordinary charachter */
      *input_ptr = c;
      input_ptr++;
      if ( (option & DCLI_OPT_NOECHO) == 0)
      {
        char_insert_nob( chn, 1);
        r_print( chn, "%c", c);
      }
    }
  }
  *terminator = DCLI_K_MAXLEN;
  *input_ptr = '\0';
  if ( (option & DCLI_OPT_NOECHO) == 0)
    r_print( chn,"\n\r");
  return 1;
}

/*************************************************************************
*
* Name:		dcli_recall_create()
*
* Type		int
*
* Type		Parameter	IOGF	Description
* dcli_sRecall **recall		O	recall buffer.
*
* Description:
*	Create a recall buffer.	
*		
**************************************************************************/

int dcli_recall_create( dcli_sRecall **recall)
{
	*recall = calloc( 1, sizeof(dcli_sRecall)); 
	if ( *recall == 0)
	  return DCLI__NOMEMORY;
	(*recall)->first_command = -1;
	return DCLI__SUCCESS;
}

void dcli_recall_free( dcli_sRecall *recall)
{
  free( (char *) recall);
}

/*************************************************************************
*
* Name:		dcli_recall_insert()
*
* Type		int
*
* Type		Parameter	IOGF	Description
* dcli_sRecall 	*recall		I	recall buffer.
* char		*command	I	string to insert in recall.
*
* Description:
*	Inserts a command in the recall buffer.
*
**************************************************************************/

static int	dcli_recall_insert( 	dcli_sRecall 	*recall,
					char		*command)
{
	if ( *command == 0)
	  return DCLI__SUCCESS;
	if ( strcmp( (char *) recall->command[recall->last_command], command) == 0)
	  return DCLI__SUCCESS;
	
	recall->last_command++;
	if ( recall->last_command >= DCLI_RECALL_MAX)
	  recall->last_command = 0;
	strncpy( &(recall->command[recall->last_command][0]), command, 200);
	if ( recall->first_command == recall->last_command)
	  recall->first_command++;
	if ( recall->first_command > DCLI_RECALL_MAX)
	  recall->first_command = 0;
	if ( recall->first_command == -1)
	  recall->first_command = 0;
	return DCLI__SUCCESS;
}

/*************************************************************************
*
* Name:		dcli_recall_getcommand()
*
* Type		int
*
* Type		Parameter	IOGF	Description
* dcli_sRecall 	*recall		I	recall buffer.
* int		nr		I	index of returned command.
* char		*command	O	command.
*
* Description:
*	Returns a command from the recall buffer.
*
**************************************************************************/

static int	dcli_recall_getcommand( 	dcli_sRecall 	*recall,
					int		nr,
					char		*command)
{
	int	index;
	
	if ( (nr >= DCLI_RECALL_MAX) || ( nr < 0))
	{
	  *command = 0;
	  return 1;
	}
	index = recall->last_command - nr;
	if ( index < 0)
	  index += DCLI_RECALL_MAX; 
	strcpy( command, &(recall->command[index][0]));	
	return 1;
}

int	dcli_input_init( dcli_sChannel *chn, dcli_sRecall **recall_buf)
{
  int sts;

  init_state_table();

  sts = dcli_qio_assign( "stdin", chn);
  if ( EVEN(sts)) return sts;

  if ( recall_buf)
  {
    sts = dcli_recall_create( recall_buf);
    if ( EVEN(sts)) return sts;
  }
  return DCLI__SUCCESS;
}

int	dcli_input_end( dcli_sChannel *chn, dcli_sRecall *recall_buf)
{
  if ( recall_buf)
    dcli_recall_free( recall_buf);
  dcli_qio_reset( chn);
  return DCLI__SUCCESS;
}


int	dcli_get_input_command( dcli_sChannel *chn, const char *prompt, char *cmd, 
		int maxlen, dcli_sRecall *recall_buf)
{
  unsigned long	option = 0;
  unsigned long	terminator;
  int		sts;

  sts = dcli_get_input_string( chn, cmd, &terminator,
		maxlen,  recall_buf, option, 0, 0, 0, prompt);
  return sts;
}

/*************************************************************************
*
* Name:		dcli_get_input_string()
*
* Type		int
*
* Type		Parameter	IOGF	Description
* char		*chn		I	channel.
* char		*out_string	O	input string
* unsigned long	*out_terminator	O	terminator
* int		out_maxlen	I	max charachters.
* unsigned long	recall		I	recall buffer.
* unsigned long	option		I	option mask.
* int		timeout		I	timeout time
* int		(* timeout_func) () I	timeout function
* unsigned long	timeout_arg	I	timeout function argument
* char		*prompt		I	prompt string.
*
* Description:
*	Read a input string.
*
**************************************************************************/

int	dcli_get_input_string( 	dcli_sChannel	*chn,
				char		*out_string,
				unsigned long	*out_terminator,
				int		out_maxlen,
				dcli_sRecall 	*recall,
				unsigned long	option,
				int		timeout,
				int		(* timeout_func) (),
				void		*timeout_arg,
				const char     	*prompt)
{
	char		input_str[200];
	char		out_str[200];
	char		dum_str[200];
	int		maxlen = 199;
	unsigned long	terminator;
	int		index;
	int		recall_index = 0;
	
	if ( prompt != NULL)
#if defined OS_VMS
	  r_print( chn, "\n%s", prompt);
#else
	  r_print( chn, "%s", prompt);
#endif

	terminator = 0;
	index = 0;
	out_str[0] = 0;
	while ( 1)
	{	
	  dcli_get_input( chn, input_str, &terminator, maxlen, option, timeout);

	  if (terminator == DCLI_K_RETURN)
	    break;
	  if ((terminator == DCLI_K_ARROW_LEFT) && ((option & DCLI_OPT_NOEDIT) != 0))
	    break;
	  if ((terminator == DCLI_K_ARROW_RIGHT) && ((option & DCLI_OPT_NOEDIT) != 0))
	    break;
	  if ((terminator == DCLI_K_DELETE) && ((option & DCLI_OPT_NOEDIT) != 0))
	    break;
	  if ((terminator == DCLI_K_BACKSPACE) && ((option & DCLI_OPT_NOEDIT) != 0))
	    break;
	  if ((terminator == DCLI_K_ARROW_UP) && ((option & DCLI_OPT_NORECALL) != 0))
	    break;
	  if ((terminator == DCLI_K_ARROW_DOWN) && ((option & DCLI_OPT_NORECALL) != 0))
	    break;
	  if ((terminator == DCLI_K_TIMEOUT) && ((option & DCLI_OPT_NOEDIT) != 0))
	  {
	    if ( timeout_func != NULL)
	      (timeout_func) ( timeout_arg);
	    else
	      break;
	  }

	  if ( (option & DCLI_OPT_NOEDIT) == 0)
	  {
	    switch ( terminator) 
	    {
	      case DCLI_K_TIMEOUT:
	        strcpy( dum_str, (char *) &out_str[index]);
	        strcpy( (char *) &out_str[index], input_str);
	        index += strlen(input_str);
	        strcpy( (char *) &out_str[index], dum_str);
	        if ( timeout_func != NULL)
	        {
	 	  store_cursorpos( chn);
	          (timeout_func) ( timeout_arg);
		  restore_cursorpos( chn);
	        }
	        break;
	      case DCLI_K_ARROW_LEFT:
	        strcpy( dum_str, (char *) &out_str[index]);
	        strcpy( &out_str[index], input_str);
	        index += strlen(input_str);
	        strcpy( &out_str[index], dum_str);
	        if ( index > 0)
	        {
	          index--;
	          cursor_rel( chn, 0, -1);
	        }
	        break;
	      case DCLI_K_ARROW_RIGHT:
	        strcpy( dum_str, (char *) &out_str[index]);
	        strncpy( (char *) &out_str[index], input_str, strlen(input_str));
	        index += strlen(input_str);
	        strcpy( (char *) &out_str[index], dum_str);
	        if ( index < (int)strlen( out_str))
	        {
	          index++;
	          cursor_rel( chn, 0, 1);
	        }
	        break;
	      case DCLI_K_BACKSPACE:
	        strcpy( dum_str, (char *) &out_str[index]);
	        strncpy( &out_str[index], input_str, strlen(input_str));
	        index += strlen(input_str);
	        strcpy( &out_str[index], dum_str);
	        if ( index > 0)
	        {
	          cursor_rel( chn, 0, - index);
	          index = 0;
	        }
	        break;
	      case DCLI_K_DELETE:
	        strcpy( dum_str, (char *) &out_str[index]);
	        strncpy( &out_str[index], input_str, strlen(input_str));
	        index += strlen(input_str);
	        strcpy( &out_str[index], dum_str);
	        if ( index > 0)
	        {
	          strcpy( dum_str, &out_str[index]);
	          index--;
	          cursor_rel( chn,  0, -1);
	          strcpy( &out_str[index], dum_str);
	          char_delete( chn, 1);

  	        }
	        break;
	    }
	  }
	  if ( (option & DCLI_OPT_NORECALL) == 0)
	  {
	    switch ( terminator) 
	    {
	      case DCLI_K_ARROW_UP:
	        if ( !recall)
                  break;
	        index += strlen(input_str);
	        recall_index++;
	        if ( recall_index > DCLI_RECALL_MAX)
	          recall_index = DCLI_RECALL_MAX + 1;
	        dcli_recall_getcommand( recall, recall_index - 1, out_str);
	        cursor_rel( chn, 0, -index);
	        eofline_erase( chn);
	        index = strlen(out_str);
	        r_print( chn,"%s", out_str);
	        break;
	      case DCLI_K_ARROW_DOWN:
	        if ( !recall)
                  break;
	        index += strlen(input_str);
	        recall_index--;
	        if ( recall_index < 0)
	          recall_index = 0;
	        dcli_recall_getcommand( recall, recall_index - 1, out_str);
	        cursor_rel( chn, 0, -index);
	        eofline_erase( chn);
	        index = strlen( out_str);
	        r_print( chn,"%s", out_str);
	        break;
	    }
	  }
	}
	strcpy( dum_str, (char *) &out_str[index]);
	strncpy( &out_str[index], input_str, strlen(input_str));
	index += strlen(input_str);
	strcpy( &out_str[index], dum_str);
	strcpy( out_string, out_str);
	if ( (option & DCLI_OPT_NORECALL) == 0)
	{	
	  /* Save in recall buffer */
	  recall_index = 0;
	  dcli_recall_insert( recall, out_string);
	}
	*out_terminator = terminator;
	return DCLI__SUCCESS;
}



/************************************************************************
*
* Name:	dcli_qio_assign(char *s, dcli_sChannel *chn)
*
* Type:	int
*
* TYPE		PARAMETER	IOGF	DESCRIPTION
* char		*s		     I	    String
* int		*chn		 O	    Kanal
*
* Description:	G�r en assign av s till kanalnummer chn
*************************************************************************/
int dcli_qio_assign( char *s, dcli_sChannel *chn)
#ifdef OS_VMS
{
  static $DESCRIPTOR(device,"");
  static char *stin = "SYS$OUTPUT:";
  char   *devn;

  /* Om input ska vara stdin, s�tt stdinput */
  if ( strcmp(s,"stdin") == 0) 
    devn = stin; 
  else
    devn = s;

  device.dsc$a_pointer = devn;
  device.dsc$w_length  = strlen(devn);

  return sys$assign(&device, (int *) chn,0,0);
}
#elif  OS_ELN
{
  static	 $DESCRIPTOR(device,"");
  static	 char *stdin = "CONSOLE$";
  static	 char *stdinsufx = "ACCESS";
  int	    	nbbyte,bytebuff;
  unsigned char	typaheadbuf[200];

  DDA$_EXTENDED_STATUS_INFO	ext_sts;

  VARYING_STRING(255)	varyingstdin;
  static   char devn[40];
  static   unsigned int sts;
  static   PORT tmp;
  static   char *stp;

  /* Om stdin , ta eln-argumentet f�r detta */
  if ( strcmp(s,"stdin") == 0)

    /* Kolla om programmet startades med parametrar enl. ELN */
    if (eln$program_argument_count() != 0) {
      eln$program_argument( &varyingstdin, STDIN_IDX);
			VARYING_TO_CSTRING(varyingstdin, stdin);
    }
    else return 0;
  else
    /* Inte stdin, ta s */
    stdin = s;

  /* G�r om "stdin:" till "stdin$ACCESS" */
  strcpy(devn,stdin);
  strcat(devn,stdinsufx);
  if ( (stp = strchr(devn,':')) != NULL)
    *stp = '$';
  else
    if ((stp = strchr(devn,'$')) == NULL)
      return 0;

  device.dsc$a_pointer = devn;
  device.dsc$w_length  = strlen(devn);

  ker$create_port( &sts, (PORT *) chn, NULL);
  if ( sts ) 
    ker$translate_name( &sts, &tmp, &device, NAME$LOCAL);
  if ( sts ) 
    ker$connect_circuit( &sts, (PORT *) chn, NULL, &device, NULL, NULL, NULL);
  /* 
    sylvie's modifications the 12.12.91 :
    When the terminal to which you want to redirect input/output is connected
    to the rt vax via a terminal driver(DH driver) you may get a lot of unwanted
    characters at the first read request because the DH driver keeps the
    characters in a type ahead buffer .
    So before requesting the user for some characters empty the typeahead
    driver:
  */

  bytebuff = sizeof(typaheadbuf);
  do
  {
     
    eln$tty_read(&sts,	/* Status		     */
      		  (PORT *) chn,	/* DDA port 		     */
		  bytebuff,	/* Antal tecken att l�sa     */
		  &typaheadbuf,	/* Meddelande buffer	     */
		  &nbbyte,	/* Antal l�sta tecken	     */
		  1,		/* L�s option		     */
		  NULL,		/* Termineringsmask	     */
		  NULL,		/* Timeout 		     */
		  0,		/* Minsta antal l�sbyte	     */
		  &ext_sts,	/* Extended status	     */
		  NULL,		/* Message objekt	     */
	          NULL);	/* Message pointer	     */

    if(!sts)return(sts);
  } while( nbbyte >= bytebuff );		
  return sts;
}
#elif defined OS_POSIX
{
  int chan = -1;
  int sts;

  if ( strcmp( s,"stdin") == 0)
    chan = STDIN_FILENO; 
  else
  {
    chan = open( s, O_RDWR | O_NOCTTY);
    if ( chan == -1)
    {
      printf( "No such device\n");
      return 0;
    }
  }
  sts = dcli_qio_set_attr( &chan, 10);
  *chn = (dcli_sChannel) chan;
  return 1;
}
#endif




/************************************************************************
*
* Name:	dcli_qio_set_attr( dcli_sChannel *chn, int tmo)
*
* Type:	int
*
* TYPE		PARAMETER	IOGF	DESCRIPTION
* char		*s		     I	    String
* int		*chn		 O	    Kanal
*
* Description:	Set attributes to a tty
*************************************************************************/
int dcli_qio_set_attr( dcli_sChannel *chn, int tmo)
#if defined(OS_VMS) || defined(OS_ELN)
{
  return DCLI__SUCCESS;
}
#elif defined OS_POSIX
{
  int chan;
  int sts;
  struct termios t;

  chan = *(int *) chn; 

  sts = tcgetattr( chan, &t);
  if ( sts != 0) return 0;

  t.c_cc[VMIN] = 0;
  t.c_cc[VTIME] = tmo;
  t.c_lflag &= ~ICANON;
  t.c_lflag &= ~ECHO;
  /* t.c_iflag ...*/
  sts = tcflush( chan, TCIFLUSH);
  sts = tcsetattr( chan, TCSAFLUSH, &t);  

  return 1;
}
#endif

/************************************************************************
*
* Name:	dcli_qio_reset(char *s, dcli_sChannel *chn)
*
* Type:	int
*
* TYPE		PARAMETER	IOGF	DESCRIPTION
* char		*s		     I	    String
* int		*chn		 O	    Kanal
*
* Description:	Reset the channel before exit
*************************************************************************/
int dcli_qio_reset( dcli_sChannel *chn)
#if defined(OS_VMS) || defined(OS_ELN)
{
  return DCLI__SUCCESS;
}
#elif defined OS_POSIX
{
  int chan;
  int sts;
  struct termios t;

  chan = *(int *) chn; 

  sts = tcgetattr( chan, &t);
  if ( sts != 0) return 0;

  t.c_cc[VMIN] = 0;
  t.c_cc[VTIME] = 0;
  t.c_lflag |= ICANON;
  t.c_lflag |= ECHO;
  /* t.c_iflag ...*/
  sts = tcflush( chan, TCIFLUSH);
  sts = tcsetattr( chan, TCSAFLUSH, &t);  

  return 1;
}
#endif

/************************************************************************
*
* Name:	dcli_qio_readw( dcli_sChannel chn, char *buf, int len)
*
* Type:	int
*
* TYPE		PARAMETER	IOGF	DESCRIPTION
* int		chn         I       Kanal
* char		*buf         O      L�st buffer
* int		len         I       Antal tecken som f�r l�sas
*
* Description:	L�ser med qiow fr�n chn till buf
*************************************************************************/
int dcli_qio_readw( dcli_sChannel *chn, char *buf, int len)
#ifdef OS_VMS
{
  struct statusblk {
		short condval;
		short transcount;
		int   devstat;
		} stsblk;
  static unsigned int code;
  int	i;
  int 	sts;

  code  = IO$_READLBLK | IO$M_NOECHO | IO$M_NOFILTR;

  sts = sys$qiow( 1, *chn, code, &stsblk,0,0,buf,len,0,0,0,0);
  return sts;
}
#elif  OS_ELN
{
  static int                       nob=1;
  static int                       opt=1;
  static unsigned int 			 sts;
  static DDA$BREAK_MASK            code={0,0,0,0,0,0,0,0};
  static DDA$_EXTENDED_STATUS_INFO stsblk;
  int	i;
	

  eln$tty_read( &sts, (PORT *) chn, 1, buf, &nob, opt, code, NULL,
	 	1, stsblk,NULL, NULL);
  return sts;
}
#elif defined OS_POSIX
{
  int n = 0;

  while( n == 0)
    n = read( *chn, buf, len);
  return 1;
}
#endif

/************************************************************************
*
* Name:	dcli_qio_read( dcli_sChannel chn, int tmo, char *buf, int len)
*
* Type:	int
*
* TYPE		PARAMETER	IOGF	DESCRIPTION
* int		chn         I       Kanal
* int		tmo         I       Timout-tid
* char		*buf         O      L�st buffer
* int		len         I       Antal tecken som f�r l�sas
*
* Description:	L�ser med qio fr�n chn till buf med timout-tid tmo (ms)
*************************************************************************/
int dcli_qio_read( dcli_sChannel *chn, int tmo, char *buf, int len)
#ifdef OS_VMS
{
  struct statusblk {
		short condval;
		short transcount;
		int   devstat;
		} stsblk;
  static unsigned int code;
  static unsigned int sts;
  int	i;

  if ( tmo < 1000)
    tmo = 1000;

  code = IO$_READLBLK | IO$M_NOECHO | IO$M_NOFILTR | IO$M_TIMED;
  sts = sys$qiow( 1, * (int *) chn, code, &stsblk,0,0,buf,len,tmo/1000,0,0,0);
  if ( stsblk.condval == SS$_TIMEOUT) return 0;
  if (ODD(sts)) return 1;
  return sts;
}
#elif  OS_ELN
{
  static LARGE_INTEGER	timout;
  static int                       nob=1;
  static int                       opt=2;
  static unsigned int 			 sts;
  static DDA$BREAK_MASK            code={0,0,0,0,0,0,0,0};
  static DDA$_EXTENDED_STATUS_INFO stsblk;
  int			i;

  if ( tmo < 1000)
    tmo = 1000;

  timout.low  = - (tmo * 10000);
  timout.high = -1;

  eln$tty_read( &sts, (PORT *) chn, 1, buf, &nob, opt, NULL, &timout,
 		1, stsblk, NULL, NULL);
  if ( sts == ELN$_TIMEOUT ) return 0;
  if (ODD(sts)) return 1;
  return sts;
}
#elif defined OS_POSIX
{
  int n;

  n = read( * (int *)chn, buf, len);
  if ( n == 0)
    /* Timeout */
    return 0;
  return 1;
}
#endif

/************************************************************************
*
* Name:	dcli_qio_writew(dcli_sChannel *chn, char *buf, int len)
*
* Type:	int
*
* TYPE		PARAMETER	IOGF	DESCRIPTION
* int		chn         I       Kanal
* char		*buf        I       Buffer
* int		len         I       Antal tecken som ska skrivas
*
* Description:	Skriver med qiow fr�n buf till chn
*************************************************************************/
int dcli_qio_writew( dcli_sChannel *chn, char *buf, int len)
#ifdef OS_VMS
{
  struct statusblk {
		short condval;
		short transcount;
		int   devstat;
		} stsblk;
  static unsigned int code;

  code  = IO$_WRITELBLK | IO$M_NOECHO | IO$M_NOFILTR;
  return sys$qiow( 0, *(int *)chn, code, &stsblk,0,0,buf,len,0,0,0,0);

}
#elif  OS_ELN
{
  static int                       nob;
  static unsigned int 		 sts;

  while (len > 0)
  {
    eln$tty_write(&sts,
			(PORT *) chn,
			len,
			buf,
			&nob,
			NULL,
			NULL);
    if (sts == KER$_DISCONNECT || sts == KER$_NO_SUCH_PORT) dcli_exit_now(1, sts);
    len -= nob;
    buf += nob;
  }

  return sts;

}
#elif defined OS_POSIX
{
  if ( *(int *) chn == STDIN_FILENO)
    write( STDOUT_FILENO, buf, len);
  else
    write( *(int *)chn, buf, len);
  return 1;
}
#endif

/************************************************************************
*
* Name:	dcli_qio_write(dcli_sChannel *chn, int tmo, char *buf, int len)
*
* Type:	int
*
* TYPE		PARAMETER	IOGF	DESCRIPTION
* int		chn         I       Kanal
* int		tmo         I       Timout-tid
* char		*buf         O      L�st buffer
* int		len         I       Antal tecken som f�r l�sas
*
* Description:	Skriver med qio fr�n buf till chn med timout-tid tmo (ms)
*************************************************************************/
int dcli_qio_write( dcli_sChannel *chn, int tmo, char *buf, int len)
#ifdef OS_VMS
{
  struct statusblk {
		short condval;
		short transcount;
		int   devstat;
		} stsblk;
  static unsigned int code;
  static unsigned int sts;

  if ( tmo < 1000)
    tmo = 1000;

  code  = IO$_WRITELBLK | IO$M_NOECHO | IO$M_NOFILTR | IO$M_TIMED;
  sts   = sys$qiow( 0, (int *)chn, code, &stsblk,0,0,buf,len,tmo/1000,0,0,0);
  return stsblk.condval == SS$_TIMEOUT ? 0 : 1;        

}
#elif  OS_ELN
{
  static LARGE_INTEGER	timout;
	static int                       nob;
	static int                       opt=2;
	static unsigned int 			 sts;
  static DDA$BREAK_MASK            code={0,0,0,0,0,0,0,0};
  static DDA$_EXTENDED_STATUS_INFO stsblk;

  if ( tmo < 1000)
    tmo = 1000;

  nob = len;	
  timout.low  = - (tmo * 10000);
  timout.high = -1;

  eln$tty_write(&sts, 
                  (PORT *) chn,
		  1,
                  buf,
                 &nob,
                  opt,
                 NULL,
              &timout,
                    1,
               stsblk,
                 NULL,
                 NULL);

  if (sts == KER$_DISCONNECT || sts == KER$_NO_SUCH_PORT) dcli_exit_now(1, sts);
  return sts == ELN$_TIMEOUT ? 0 : 1;

}
#elif defined OS_POSIX
{
  if ( *(int *)chn == STDIN_FILENO)
    write( STDOUT_FILENO, buf, len);
  else
    write( *(int *)chn, buf, len);
  return 1;
}
#endif



