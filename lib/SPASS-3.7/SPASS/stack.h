/**************************************************************/
/* ********************************************************** */
/* *                                                        * */
/* *              GLOBAL SYSTEM STACK                       * */
/* *                                                        * */
/* *  $Module:      STACK                                   * */ 
/* *                                                        * */
/* *  Copyright (C) 1996, 1998, 1999, 2001                  * */
/* *  MPI fuer Informatik                                   * */
/* *                                                        * */
/* *  This program is free software; you can redistribute   * */
/* *  it and/or modify it under the terms of the FreeBSD    * */
/* *  Licence.                                              * */
/* *                                                        * */
/* *  This program is distributed in the hope that it will  * */
/* *  be useful, but WITHOUT ANY WARRANTY; without even     * */
/* *  the implied warranty of MERCHANTABILITY or FITNESS    * */
/* *  FOR A PARTICULAR PURPOSE.  See the LICENCE file       * */ 
/* *  for more details.                                     * */
/* *                                                        * */
/* *                                                        * */
/* $Revision: 1.2 $                                     * */
/* $State: Exp $                                            * */
/* $Date: 2010-02-22 14:09:59 $                             * */
/* $Author: weidenb $                                         * */
/* *                                                        * */
/* *             Contact:                                   * */
/* *             Christoph Weidenbach                       * */
/* *             MPI fuer Informatik                        * */
/* *             Stuhlsatzenhausweg 85                      * */
/* *             66123 Saarbruecken                         * */
/* *             Email: spass@mpi-inf.mpg.de                * */
/* *             Germany                                    * */
/* *                                                        * */
/* ********************************************************** */
/**************************************************************/


/* $RCSfile: stack.h,v $ */

#ifndef _STACK_
#define _STACK_

/**************************************************************/
/* Includes                                                   */
/**************************************************************/

#include "misc.h"

/**************************************************************/
/* More basic types and macros                                */
/**************************************************************/


#define stack_SIZE 10000

typedef POINTER STACK[stack_SIZE];


/**************************************************************/
/* Global Variables                                           */
/**************************************************************/

extern STACK stack_STACK;
extern NAT   stack_POINTER;


/**************************************************************/
/* Inline Functions                                           */
/**************************************************************/

static __inline__ void stack_Init(void)
{
  stack_POINTER = 0;
}

static __inline__ void stack_Push(POINTER Entry)
{
#ifdef CHECK
  if (stack_POINTER >= stack_SIZE) {
    misc_StartErrorReport();
    misc_ErrorReport("\n In stack_Push: Stack Overflow.");
    misc_FinishErrorReport();
  }
#endif
  
  stack_STACK[stack_POINTER++]= Entry;
}

static __inline__ int stack_Pop(void)
{
  return --stack_POINTER;
}

static __inline__ POINTER stack_PopResult(void)
{
  return stack_STACK[--stack_POINTER];
}

static __inline__ void stack_NPop(NAT N)
{
  stack_POINTER -= N;
}

static __inline__ POINTER stack_Top(void)
{
  return stack_STACK[stack_POINTER-1];
}

static __inline__ POINTER stack_NthTop(NAT N)
{
  return stack_STACK[stack_POINTER-(1+N)];
}

static __inline__ void stack_RplacTop(POINTER Entry)
{
  stack_STACK[stack_POINTER-1] = Entry;
}

static __inline__ void stack_RplacNthTop(NAT N, POINTER Entry)
{
  stack_STACK[stack_POINTER-(1+N)] = Entry;
}

static __inline__ void stack_RplacNth(NAT N, POINTER Entry)
{
  stack_STACK[N] = Entry;
}

static __inline__ NAT stack_Bottom(void)
{
  return stack_POINTER;
}

static __inline__ void stack_SetBottom(NAT Ptr)
{
  stack_POINTER = Ptr;
}

static __inline__ BOOL stack_Empty(NAT Ptr)
{
  return stack_POINTER == Ptr;
}


#endif


