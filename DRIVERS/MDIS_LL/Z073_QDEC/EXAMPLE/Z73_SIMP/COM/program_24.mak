#***************************  M a k e f i l e  *******************************
#
#         Author: gl
#          $Date: 2012/01/09 11:34:55 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for the Z73 example program (24Bit)
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program_24.mak,v $
#   Revision 1.1  2012/01/09 11:34:55  GLeonhardt
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2011 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=z73_simp

MAK_SWITCH=$(SW_PREFIX)Z73_POSCNT_24

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_utl$(LIB_SUFFIX)	\

MAK_INCL=$(MEN_INC_DIR)/z73_drv.h	\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/mdis_err.h	\
         $(MEN_INC_DIR)/usr_utl.h	\
         $(MEN_INC_DIR)/usr_oss.h	\


MAK_INP1=z73_simp$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
