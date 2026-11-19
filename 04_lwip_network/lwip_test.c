/*=============================================================================
 * lwip_test.c --- the two addresses every test file needs
 *===========================================================================*/

#include "lwip_test.h"

char *local_IP  = BOARD_IP;     /* Loongson 1B, GMAC0  */
char *remote_IP = PC_IP;        /* the PC used as peer */
