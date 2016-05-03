/***********************  I n c l u d e  -  F i l e  ***********************/
/*!
 *        \file  z73_int.h
 *
 *      \author  Christian.Schuster@men.de
 *        $Date: 2012/01/09 11:34:46 $
 *    $Revision: 1.4 $
 *
 *       \brief  Header file for Z73 driver containing
 *               Z73 specific status codes and
 *               Z73 function prototypes
 *
 *    \switches  _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 *               Z73_POSCNT_24
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: z73_int.h,v $
 * Revision 1.4  2012/01/09 11:34:46  GLeonhardt
 * R: 1.) New variant with 24 bit position
 *    2.) Only 8 bit position of 16 bit counter is used
 * M: 1.) Add switch Z73_POSCNT_24 to enable 24 bit suport
 *    2.) Return 16 bit position or 24 bit
 *
 * Revision 1.3  2010/04/21 15:49:21  amorbach
 * R: Porting to MDIS5
 * M: changed according to MDIS Porting Guide 0.8
 *
 * Revision 1.2  2006/06/01 17:08:45  cs
 * changed ADDRSPACE_SIZE to 16 (Chameleon BBIS reports real size)
 *
 * Revision 1.1  2005/11/29 16:13:19  cs
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2005 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#ifndef _Z73_INT_H
#define _Z73_INT_H

#ifdef __cplusplus
      extern "C" {
#endif

#define _NO_LL_HANDLE       /* ll_defs.h: don't define LL_HANDLE struct */

#include <MEN/men_typs.h>   /* system dependent definitions   */
#include <MEN/maccess.h>    /* hw access macros and types     */
#include <MEN/dbg.h>        /* debug functions                */
#include <MEN/oss.h>        /* oss functions                  */
#include <MEN/desc.h>       /* descriptor functions           */
#include <MEN/modcom.h>     /* ID PROM functions              */
#include <MEN/mdis_api.h>   /* MDIS global defs               */
#include <MEN/mdis_com.h>   /* MDIS common defs               */
#include <MEN/mdis_err.h>   /* MDIS error codes               */
#include <MEN/ll_defs.h>    /* low-level driver definitions   */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* general defines */
#define CH_NUMBER           1           /**< number of device channels */
#define USE_IRQ             TRUE        /**< interrupt required  */
#define ADDRSPACE_COUNT     1           /**< nbr of required address spaces */
#define ADDRSPACE_SIZE      16          /**< size of address space */

#define Z073_STATUSQ_SIZE_DEF 0x10      /**< default size of status queue */
/* debug defines */
#define DBG_MYLEVEL         llHdl->dbgLevel   /**< debug level */
#define DBH                 llHdl->dbgHdl     /**< debug handle */

/** \name 16Z073_QDEC register defines
 *  \anchor reg_defines
 */
/**@{*/
#define Z073_POS_CNT    0x00
        /**< read:  get position and status, set reg to all 0 */
#define Z073_POS_CNT_STS        0x80000000
        /**< Status of position counter (changed/initial value) */
#ifdef Z73_POSCNT_24
 #define Z073_POS_CNT_CNT       0x00FFFFFF
#else
 #define Z073_POS_CNT_CNT       0x0000FFFF
#endif
        /**<  State of position counter */
#define Z073_IRQ_EN             0x08
        /**< Interrupt enable register */
#define Z073_IRQ_EN_ALL         0x0F
        /**< Enable interrupt for button release event */
#define Z073_IRQ_EN_REL         0x08
        /**< Enable interrupt for button release event */
#define Z073_IRQ_EN_PRS         0x04
        /**< Enable interrupt for button press event */
#define Z073_IRQ_EN_DWN         0x02
        /**< Enable interrupt for button move down event */
#define Z073_IRQ_EN_UP          0x01
        /**< Enable interrupt for button move up event */

#define Z073_IRQ                0x0C
        /**< Interrupt request register */
        /*!< read:  get status if interrupt requests and input lines\n
         *   write: clear interrupt */
#define Z073_IRQ_STS_PRSREL     0x00000040
        /**< Interrupt request for push button release */
        /*!< 0: PRESS_N input is low \n
         *   1: PRESS_N input is high */
#define Z073_IRQ_STS_A          0x00000020
        /**< Enable interrupt for push button press */
        /*!< 0: INPUT_A input is low \n
         *   1: INPUT_A input is high */
#define Z073_IRQ_STS_B          0x00000010
        /**< Enable interrupt for button move down */
        /*!< 0: INPUT_B input is low \n
         *   1: INPUT_B input is high */
#define Z073_IRQ_REL            0x00000008
        /**< Interrupt request for push button release */
#define Z073_IRQ_PRS            0x00000004
        /**< Enable interrupt for push button press */
#define Z073_IRQ_DWN            0x00000002
        /**< Enable interrupt for button move down */
#define Z073_IRQ_UP             0x00000001
        /**< Enable interrupt for button move up */

/**@}*/

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/** low-level handle */
typedef struct {
    /* general */
    int32           memAlloc;       /**< size allocated for the handle */
    OSS_HANDLE      *osHdl;         /**< oss handle */
    OSS_IRQ_HANDLE  *irqHdl;        /**< irq handle */
    DESC_HANDLE     *descHdl;       /**< desc handle */
    MACCESS         ma;             /**< hw access handle */
    MDIS_IDENT_FUNCT_TBL idFuncTbl; /**< id function table */
    /* debug */
    u_int32         dbgLevel;       /**< debug level */
    DBG_HANDLE      *dbgHdl;        /**< debug handle */
    /* misc */
    u_int32         irqCount;       /**< interrupt counter */
    u_int32         irqEn;          /**< interrupts to enable */

    OSS_SIG_HANDLE  *prsRelSig;     /**< signal f. button press/release events*/
    OSS_SIG_HANDLE  *upDwnSig;      /**< signal f. button move events*/

    /* status data queue */
    u_int32         *statusQ;       /**< FIFO for status */
                                    /*!< filled when push button events are
                                     *   detected */
    u_int32         statusQIn;      /**< position of first free field in Q */
    u_int32         statusQOut;     /**< position of first filled field in Q */
    u_int32         statusQDepth;       /**< depth of Q */

    int32           statusQSizeGot; /**< size of block actually allocated */

    int32           error;          /**< error detected, transmitted first */
} LL_HANDLE;

/* include files which need LL_HANDLE */
#include <MEN/ll_entry.h>   /* low-level driver jump table  */
#include <MEN/z73_drv.h>    /* Z73 driver header file */

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static int32 Z73_Init(DESC_SPEC *descSpec, OSS_HANDLE *osHdl,
                       MACCESS *ma, OSS_SEM_HANDLE *devSemHdl,
                       OSS_IRQ_HANDLE *irqHdl, LL_HANDLE **llHdlP);
static int32 Z73_Exit(LL_HANDLE **llHdlP );
static int32 Z73_Read(LL_HANDLE *llHdl, int32 ch, int32 *value);
static int32 Z73_Write(LL_HANDLE *llHdl, int32 ch, int32 value);
static int32 Z73_SetStat(LL_HANDLE *llHdl,int32 ch, int32 code, INT32_OR_64 value32_or_64);
static int32 Z73_GetStat(LL_HANDLE *llHdl, int32 ch, int32 code, INT32_OR_64 *value32_or_64P);
static int32 Z73_BlockRead(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
                            int32 *nbrRdBytesP);
static int32 Z73_BlockWrite(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
                             int32 *nbrWrBytesP);
static int32 Z73_Irq(LL_HANDLE *llHdl );
static int32 Z73_Info(int32 infoType, ... );

static char* Ident( void );
static int32 Cleanup(LL_HANDLE *llHdl, int32 retCode);

static int32 getStatus( LL_HANDLE *llHdl );

#ifdef __cplusplus
      }
#endif

#endif /* _Z73_INT_H */
