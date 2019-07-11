/****************************************************************************
 ************                                                    ************
 ************                   Z73_SIMP                         ************
 ************                                                    ************
 ****************************************************************************/
/*!
 *         \file z73_simp.c
 *       \author Christian.Schuster@men.de
 *
 *       \brief  Simple example program for the Z73 driver
 *
 *               This tool has two modes:
 *               - interrupt based:  \n
 *                    the interrupts of the module are enabled and the tool
 *                    will display the data everytime a signal is received and
 *                    data is read
 *               - polling mode:  \n
 *                    the interrupts of the module are disabled and the tool
 *                    requests and displays data from the tool once a second
 *
 *     Required: libraries: mdis_api
 *     \switches Z73_POSCNT_24 (enable 24 Bit position)
 *
 *
 *---------------------------------------------------------------------------
 * Copyright 2010-2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/

 /*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_api.h>
#include <MEN/mdis_err.h>
#include <MEN/z73_drv.h>

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   TYPDEFS                             |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   EXTERNALS                           |
+--------------------------------------*/
/* none */

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static int32 G_Z73_sigUosCnt[2];	/**< signal counters */
static int8  G_Z73_endMe = 0;		/**< signals application to terminate */
static MDIS_PATH G_Z73_path = 0;	/**< path to opened device */

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void PrintError(char *info);
static u_int32 getStatus( void );
static void __MAPILIB SigHandler( u_int32 sigCode );

/********************************* main ************************************/
/** Program main function
 *
 *  \param argc       \IN  argument counter
 *  \param argv       \IN  argument vector
 *
 *  \return           success (0) or error (1)
 */
int main( int argc, char *argv[] )
{
	char	*device, *str;
	u_int32 error = 0;
	u_int32 looptime = 0;
	u_int32 mode = 0;	/* Basic tool mode
						 * 0: interrupt/signal based
						 * 1: polling mode */
	u_int32 flag_QFull = 0;

	if (argc < 2 || strcmp(argv[1],"-?")==0) {
		printf("Syntax: z73_simp <device> [opts]\n");
		printf("Function: Z73 example\n");
		printf("Options:\n");
		printf("    device       device name\n");
		printf("    [-t]         loop time in milliseconds [1000]\n");
		printf("    [-p]         perform test in polling mode\n");
		printf("\n");
		return(1);
	}

	device = argv[1];

	looptime = ((str = UTL_TSTOPT("t=")) ? atoi(str) : 1000);
	mode = UTL_TSTOPT("p") ? 1 : 0;

	/*--------------------+
	|  open path          |
	+--------------------*/
	if ((G_Z73_path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}

	/*----------------------+
	|  config and do tests  |
	+----------------------*/
	if( !mode )
	{
		/* init signals */
		UOS_SigInit( SigHandler );
		if( UOS_SigInstall( UOS_SIG_USR1 ) || UOS_SigInstall( UOS_SIG_USR2 ) ){
			PrintError("install signals");
			error = 1;
			goto ERR_EXIT_SIG_HDL;
		}

		if( M_setstat(G_Z73_path, Z073_SIG_PRS_REL, UOS_SIG_USR1) )
		{
			PrintError("M_setstat signal Z073_SIG_PRS_REL");
			error = 1;
			goto ERR_EXIT_SIGS;
		}
		if( M_setstat(G_Z73_path, Z073_SIG_MOVE, UOS_SIG_USR2) )
		{
			PrintError("M_setstat signal Z073_SIG_MOVE");
			error = 1;
			goto ERR_EXIT_SIGS;
		}

		/* enable interrupts */
		if ( M_setstat(G_Z73_path, Z073_INT_UP, 1) ||
			 M_setstat(G_Z73_path, Z073_INT_DWN, 1) )
		{            
			PrintError("enable up/down interrupts");
			error = 1;
			goto ERR_EXIT_SIGS;
		}

		printf( "Z073_QDEC(getStatus):\n"
				"         |  press  | release |  move   |   cnt   |\n");

		/* enable interrupts */
		if ((M_setstat(G_Z73_path, M_MK_IRQ_ENABLE, 1)) < 0) {
			PrintError("setstat M_MK_IRQ_ENABLE");
			error = 1;
			goto ERR_EXIT_SIGS;
		}

		while( !G_Z73_endMe && UOS_KeyPressed() == -1 ) {
			if( G_Z73_sigUosCnt[0] ||
				G_Z73_sigUosCnt[1] )
			{

				/* get all status messages */
				do {
					error = getStatus();
					if( error && (error == Z073_ERR_STATUSQ_FULL) ) {
						/* queue overflow, interrupts have to be enabled again */
						flag_QFull = 1;
					}
				} while( error != Z073_ERR_NO_STATUS );

				if( flag_QFull && (M_setstat(G_Z73_path, M_MK_IRQ_ENABLE, 1) < 0) )
				{
					PrintError("setstat M_MK_IRQ_ENABLE");
					G_Z73_endMe = TRUE;
				}
				flag_QFull = 0; /* queue emptied? reset flag */

				/* everything read, reset counters */
				G_Z73_sigUosCnt[0] = 0;
				G_Z73_sigUosCnt[1] = 0;

			}
			UOS_Delay(looptime);
		}

		if( M_setstat(G_Z73_path, Z073_SIG_PRS_REL, 0) ||
			M_setstat(G_Z73_path, Z073_SIG_MOVE, 0) )
		{
			PrintError("M_setstat delete signals");
			error = 1;
			goto ERR_EXIT_SIGS;
		}

	} else { /* polling mode */
		printf( "Z073_QDEC(getStatus):\n"
				"         |  press  | release |  move   |   cnt   |\n");

		while(  ( !error ||
				  error == Z073_ERR_NO_STATUS ) &&
				UOS_KeyPressed() == -1 ) {
			error = getStatus();
			UOS_Delay(looptime);
		}
		goto ERR_EXIT;
	}

	/*--------------------+
	|  cleanup            |
	+--------------------*/
ERR_EXIT_SIGS:
	if( UOS_SigRemove( UOS_SIG_USR1 ) || UOS_SigRemove( UOS_SIG_USR2 ) ){
		PrintError("uninstall signals");
		error = 1;
	}
ERR_EXIT_SIG_HDL:
	UOS_SigExit();
ERR_EXIT:
	if (M_close(G_Z73_path) < 0)
		PrintError("close");

	return( error );
}

/********************************* getStatus *******************************/
/** Get status from driver
*/
static u_int32 getStatus( void )
{
	u_int32 retVal = 0, posShift;
	u_int32 error = 0;

	#ifdef Z73_POSCNT_24
	posShift = 8; /* 24 bits used */
	#else
	posShift = 16; /* 16 bits used */
	#endif

	if( M_getstat(G_Z73_path, Z073_STATUS, (int32*)&retVal) )
	{
		error = UOS_ErrnoGet();

		/* ignore ERR_NO_STATUS here */
		if( error != Z073_ERR_NO_STATUS ) {
			/* error occured, return */
			UOS_ErrnoSet( error );
			PrintError("M_Getstat(Z073_STATUS)");
		}
		goto ERR_EXIT;
	}

	printf( "         |    %s    |    %s    |    %s    |   %8d   |\n",
			(retVal & Z073_STATUS_PRS) ? "1" : " ",
			(retVal & Z073_STATUS_REL) ? "1" : " ",
			(retVal & Z073_STATUS_MOV) ? "1" : " ",
			((int)((retVal & Z073_STATUS_CNT)<<posShift))>>posShift);

ERR_EXIT:
	return( error );
}

/********************************* SigHandler ******************************/
/** Handle received signals
 *
 *  \param sigCode       \IN  signal number from device
*/
static void __MAPILIB SigHandler( u_int32 sigCode )
{
	switch( sigCode ){
	case UOS_SIG_USR1:
		G_Z73_sigUosCnt[0]++;
		break;
	case UOS_SIG_USR2:
		G_Z73_sigUosCnt[1]++;
		break;
	default:
		G_Z73_endMe = TRUE;
		return;
	}

	return;

}

/********************************* PrintError ******************************/
/** Print MDIS error message
 *
 *  \param info       \IN  info string
*/
static void PrintError(char *info)
{
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}



