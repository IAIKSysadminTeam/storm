/**
  @file

  @ingroup cudd

  @brief Apply functions for ADDs and their operators.

  @author Fabio Somenzi

  @copyright@parblock
  Copyright (c) 1995-2015, Regents of the University of Colorado

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  Neither the name of the University of Colorado nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
  @endparblock

*/

#include "util.h"
#include "cuddInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/** \cond */

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/** \endcond */


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**
  @brief Applies op to the corresponding discriminants of f and g.

  @return a pointer to the result if succssful; NULL otherwise.

  @sideeffect None

  @see Cudd_addMonadicApply Cudd_addPlus Cudd_addTimes
  Cudd_addThreshold Cudd_addSetNZ Cudd_addDivide Cudd_addMinus Cudd_addMinimum
  Cudd_addMaximum Cudd_addOneZeroMaximum Cudd_addDiff Cudd_addAgreement
  Cudd_addOr Cudd_addNand Cudd_addNor Cudd_addXor Cudd_addXnor

*/
DdNode *
Cudd_addApply(
  DdManager * dd /**< manager */,
  DD_AOP op /**< operator */,
  DdNode * f /**< first operand */,
  DdNode * g /**< second operand */)
{
    DdNode *res;

    do {
	dd->reordered = 0;
	res = cuddAddApplyRecur(dd,op,f,g);
    } while (dd->reordered == 1);
    if (dd->errorCode == CUDD_TIMEOUT_EXPIRED && dd->timeoutHandler) {
        dd->timeoutHandler(dd, dd->tohArg);
    }

    return(res);

} /* end of Cudd_addApply */

/**
 @brief Applies op to the corresponding discriminants of f and g and produces a BDD as a result.
 
 @return a pointer to the result if succssful; NULL otherwise.
 
 @sideeffect None
 
 @see Cudd_addMonadicApply Cudd_addPlus Cudd_addTimes
 Cudd_addThreshold Cudd_addSetNZ Cudd_addDivide Cudd_addMinus Cudd_addMinimum
 Cudd_addMaximum Cudd_addOneZeroMaximum Cudd_addDiff Cudd_addAgreement
 Cudd_addOr Cudd_addNand Cudd_addNor Cudd_addXor Cudd_addXnor
 
 added 23/08/2016 by Christian Dehnert
 
 */
DdNode *
Cudd_addToBddApply(
              DdManager * dd /**< manager */,
              DD_AOP op /**< operator */,
              DdNode * f /**< first operand */,
              DdNode * g /**< second operand */)
{
    DdNode *res;
    
    do {
        dd->reordered = 0;
        res = cuddAddToBddApplyRecur(dd,op,f,g);
    } while (dd->reordered == 1);
    if (dd->errorCode == CUDD_TIMEOUT_EXPIRED && dd->timeoutHandler) {
        dd->timeoutHandler(dd, dd->tohArg);
    }
    
    return(res);
    
} /* end of Cudd_addToBddApply */

/**
  @brief Integer and floating point addition.

  @return NULL if not a terminal case; f+g otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addPlus(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *res;
    DdNode *F, *G;
    CUDD_VALUE_TYPE value;

    F = *f; G = *g;
    if (F == DD_ZERO(dd)) return(G);
    if (G == DD_ZERO(dd)) return(F);
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
	value = cuddV(F)+cuddV(G);
	res = cuddUniqueConst(dd,value);
	return(res);
    }
    if (F > G) { /* swap f and g */
	*f = G;
	*g = F;
    }
    return(NULL);

} /* end of Cudd_addPlus */


/**
  @brief Integer and floating point multiplication.

  @details This function can be used also to take the AND of two 0-1
  ADDs.

  @return NULL if not a terminal case; f * g otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addTimes(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *res;
    DdNode *F, *G;
    CUDD_VALUE_TYPE value;

    F = *f; G = *g;
    if (F == DD_ZERO(dd) || G == DD_ZERO(dd)) return(DD_ZERO(dd));
    if (F == DD_ONE(dd)) return(G);
    if (G == DD_ONE(dd)) return(F);
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
	value = cuddV(F)*cuddV(G);
	res = cuddUniqueConst(dd,value);
	return(res);
    }
    if (F > G) { /* swap f and g */
	*f = G;
	*g = F;
    }
    return(NULL);

} /* end of Cudd_addTimes */


/**
  @brief f if f&ge;g; 0 if f&lt;g.

  @details Threshold operator for Apply (f if f &ge;g; 0 if f&lt;g).

  @return NULL if not a terminal case; f op g otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addThreshold(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *F, *G;

    F = *f; G = *g;
    if (F == G || F == DD_PLUS_INFINITY(dd)) return(F);
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
	if (cuddV(F) >= cuddV(G)) {
	    return(F);
	} else {
	    return(DD_ZERO(dd));
	}
    }
    return(NULL);

} /* end of Cudd_addThreshold */


/**
  @brief This operator sets f to the value of g wherever g != 0.

  @return NULL if not a terminal case; f op g otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addSetNZ(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *F, *G;

    F = *f; G = *g;
    if (F == G) return(F);
    if (F == DD_ZERO(dd)) return(G);
    if (G == DD_ZERO(dd)) return(F);
    if (cuddIsConstant(G)) return(G);
    return(NULL);

} /* end of Cudd_addSetNZ */


/**
  @brief Integer and floating point division.

  @return NULL if not a terminal case; f / g otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addDivide(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *res;
    DdNode *F, *G;
    CUDD_VALUE_TYPE value;

    F = *f; G = *g;
    /* We would like to use F == G -> F/G == 1, but F and G may
    ** contain zeroes. */
    if (F == DD_ZERO(dd)) return(DD_ZERO(dd));
    if (G == DD_ONE(dd)) return(F);
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
	value = cuddV(F)/cuddV(G);
	res = cuddUniqueConst(dd,value);
	return(res);
    }
    return(NULL);

} /* end of Cudd_addDivide */


/**
  @brief Integer and floating point subtraction.

  @return NULL if not a terminal case; f - g otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addMinus(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *res;
    DdNode *F, *G;
    CUDD_VALUE_TYPE value;

    F = *f; G = *g;
    if (F == G) return(DD_ZERO(dd));
    // CHANGED BY CHRISTIAN DEHNERT.
    // Commented out this case to avoid issues with dynamic reordering (fix suggested by Fabio Somenzi).
    // if (F == DD_ZERO(dd)) return(cuddAddNegateRecur(dd,G));
    if (G == DD_ZERO(dd)) return(F);
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
	value = cuddV(F)-cuddV(G);
	res = cuddUniqueConst(dd,value);
	return(res);
    }
    return(NULL);

} /* end of Cudd_addMinus */


/**
  @brief Integer and floating point min.

  @details Integer and floating point min for Cudd_addApply.
  
  @return NULL if not a terminal case; min(f,g) otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addMinimum(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *F, *G;

    F = *f; G = *g;
    if (F == DD_PLUS_INFINITY(dd)) return(G);
    if (G == DD_PLUS_INFINITY(dd)) return(F);
    if (F == G) return(F);
#if 0
    /* These special cases probably do not pay off. */
    if (F == DD_MINUS_INFINITY(dd)) return(F);
    if (G == DD_MINUS_INFINITY(dd)) return(G);
#endif
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
	if (cuddV(F) <= cuddV(G)) {
	    return(F);
	} else {
	    return(G);
	}
    }
    if (F > G) { /* swap f and g */
	*f = G;
	*g = F;
    }
    return(NULL);

} /* end of Cudd_addMinimum */

/**
 @brief Integer and floating point min.
 
 @details Integer and floating point min for Cudd_addApply.
 
 @return NULL if not a terminal case; min(f,g) otherwise.
 
 @sideeffect None
 
 @see Cudd_addApply
 
 added 24/08/2016
 
 */
DdNode *
Cudd_addMinimumExcept0(
                DdManager * dd,
                DdNode ** f,
                DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == DD_ZERO(dd)) return(G);
    if (G == DD_ZERO(dd)) return(F);
    if (F == DD_PLUS_INFINITY(dd)) return(G);
    if (G == DD_PLUS_INFINITY(dd)) return(F);
    if (F == G) return(F);
#if 0
    /* These special cases probably do not pay off. */
    if (F == DD_MINUS_INFINITY(dd)) return(F);
    if (G == DD_MINUS_INFINITY(dd)) return(G);
#endif
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
        if (cuddV(F) <= cuddV(G)) {
            return(F);
        } else {
            return(G);
        }
    }
    if (F > G) { /* swap f and g */
        *f = G;
        *g = F;
    }
    return(NULL);
    
} /* end of Cudd_addMinimumExcept0 */

/**
  @brief Integer and floating point max.

  @details Integer and floating point max for Cudd_addApply.

  @return NULL if not a terminal case; max(f,g) otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addMaximum(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *F, *G;

    F = *f; G = *g;
    if (F == G) return(F);
    if (F == DD_MINUS_INFINITY(dd)) return(G);
    if (G == DD_MINUS_INFINITY(dd)) return(F);
#if 0
    /* These special cases probably do not pay off. */
    if (F == DD_PLUS_INFINITY(dd)) return(F);
    if (G == DD_PLUS_INFINITY(dd)) return(G);
#endif
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
	if (cuddV(F) >= cuddV(G)) {
	    return(F);
	} else {
	    return(G);
	}
    }
    if (F > G) { /* swap f and g */
	*f = G;
	*g = F;
    }
    return(NULL);

} /* end of Cudd_addMaximum */


/**
  @brief Returns 1 if f &gt; g and 0 otherwise.

  @details Used in conjunction with Cudd_addApply.

  @return NULL if not a terminal case.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addOneZeroMaximum(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{

    if (*f == *g) return(DD_ZERO(dd));
    if (*g == DD_PLUS_INFINITY(dd))
	return DD_ZERO(dd);
    if (cuddIsConstant(*f) && cuddIsConstant(*g)) {
	if (cuddV(*f) > cuddV(*g)) {
	    return(DD_ONE(dd));
	} else {
	    return(DD_ZERO(dd));
	}
    }

    return(NULL);

} /* end of Cudd_addOneZeroMaximum */


/**
  @brief Returns plusinfinity if f=g; returns min(f,g) if f!=g.

  @return NULL if not a terminal case; f op g otherwise, where f op g
  is plusinfinity if f=g; min(f,g) if f!=g.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addDiff(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *F, *G;

    F = *f; G = *g;
    if (F == G) return(DD_PLUS_INFINITY(dd));
    if (F == DD_PLUS_INFINITY(dd)) return(G);
    if (G == DD_PLUS_INFINITY(dd)) return(F);
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
	if (cuddV(F) != cuddV(G)) {
	    if (cuddV(F) < cuddV(G)) {
		return(F);
	    } else {
		return(G);
	    }
	} else {
	    return(DD_PLUS_INFINITY(dd));
	}
    }
    return(NULL);

} /* end of Cudd_addDiff */


/**
  @brief f if f==g; background if f!=g.

  @return NULL if not a terminal case; f op g otherwise, where f op g
  is f if f==g; background if f!=g.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addAgreement(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *F, *G;

    F = *f; G = *g;
    if (F == G) return(F);
    if (F == dd->background) return(F);
    if (G == dd->background) return(G);
    if (cuddIsConstant(F) && cuddIsConstant(G)) return(dd->background);
    return(NULL);

} /* end of Cudd_addAgreement */


/**
  @brief Disjunction of two 0-1 ADDs.

  @return NULL if not a terminal case; f OR g otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addOr(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *F, *G;

    F = *f; G = *g;
    if (F == DD_ONE(dd) || G == DD_ONE(dd)) return(DD_ONE(dd));
    if (cuddIsConstant(F)) return(G);
    if (cuddIsConstant(G)) return(F);
    if (F == G) return(F);
    if (F > G) { /* swap f and g */
	*f = G;
	*g = F;
    }
    return(NULL);

} /* end of Cudd_addOr */


/**
  @brief NAND of two 0-1 ADDs.

  @return NULL if not a terminal case; f NAND g otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addNand(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *F, *G;

    F = *f; G = *g;
    if (F == DD_ZERO(dd) || G == DD_ZERO(dd)) return(DD_ONE(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) return(DD_ZERO(dd));
    if (F > G) { /* swap f and g */
	*f = G;
	*g = F;
    }
    return(NULL);

} /* end of Cudd_addNand */


/**
  @brief NOR of two 0-1 ADDs.

  @return NULL if not a terminal case; f NOR g otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addNor(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *F, *G;

    F = *f; G = *g;
    if (F == DD_ONE(dd) || G == DD_ONE(dd)) return(DD_ZERO(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) return(DD_ONE(dd));
    if (F > G) { /* swap f and g */
	*f = G;
	*g = F;
    }
    return(NULL);

} /* end of Cudd_addNor */


/**
  @brief XOR of two 0-1 ADDs.

  @return NULL if not a terminal case; f XOR g otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addXor(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *F, *G;

    F = *f; G = *g;
    if (F == G) return(DD_ZERO(dd));
    if (F == DD_ONE(dd) && G == DD_ZERO(dd)) return(DD_ONE(dd));
    if (G == DD_ONE(dd) && F == DD_ZERO(dd)) return(DD_ONE(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) return(DD_ZERO(dd));
    if (F > G) { /* swap f and g */
	*f = G;
	*g = F;
    }
    return(NULL);

} /* end of Cudd_addXor */


/**
  @brief XNOR of two 0-1 ADDs.

  @return NULL if not a terminal case; f XNOR g otherwise.

  @sideeffect None

  @see Cudd_addApply

*/
DdNode *
Cudd_addXnor(
  DdManager * dd,
  DdNode ** f,
  DdNode ** g)
{
    DdNode *F, *G;

    F = *f; G = *g;
    if (F == G) return(DD_ONE(dd));
    if (F == DD_ONE(dd) && G == DD_ONE(dd)) return(DD_ONE(dd));
    if (G == DD_ZERO(dd) && F == DD_ZERO(dd)) return(DD_ONE(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) return(DD_ZERO(dd));
    if (F > G) { /* swap f and g */
	*f = G;
	*g = F;
    }
    return(NULL);

} /* end of Cudd_addXnor */


/**
  @brief Applies op to the discriminants of f.

  @return a pointer to the result if succssful; NULL otherwise.

  @sideeffect None

  @see Cudd_addApply Cudd_addLog

*/
DdNode *
Cudd_addMonadicApply(
  DdManager * dd,
  DD_MAOP op,
  DdNode * f)
{
    DdNode *res;

    do {
	dd->reordered = 0;
	res = cuddAddMonadicApplyRecur(dd,op,f);
    } while (dd->reordered == 1);
    if (dd->errorCode == CUDD_TIMEOUT_EXPIRED && dd->timeoutHandler) {
        dd->timeoutHandler(dd, dd->tohArg);
    }

    return(res);

} /* end of Cudd_addMonadicApply */


/**
  @brief Natural logarithm of an %ADD.

  @details The discriminants of f must be positive double's.

  @return NULL if not a terminal case; log(f) otherwise.

  @sideeffect None

  @see Cudd_addMonadicApply

*/
DdNode *
Cudd_addLog(
  DdManager * dd,
  DdNode * f)
{
    if (cuddIsConstant(f)) {
	CUDD_VALUE_TYPE value = log(cuddV(f));
	DdNode *res = cuddUniqueConst(dd,value);
	return(res);
    }
    return(NULL);

} /* end of Cudd_addLog */

/**Function********************************************************************
 
 Synopsis    [Floor of an ADD.]
 
 Description [Floor of an ADD. Returns NULL
 if not a terminal case; floor(f) otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addMonadicApply]
 
 ******************************************************************************/
DdNode *
Cudd_addFloor(
              DdManager * dd,
              DdNode * f)
{
    if (cuddIsConstant(f)) {
        CUDD_VALUE_TYPE value = floor(cuddV(f));
        DdNode *res = cuddUniqueConst(dd,value);
        return(res);
    }
    return(NULL);
    
} /* end of Cudd_addFloor */


/**Function********************************************************************
 
 Synopsis    [Ceiling of an ADD.]
 
 Description [Ceiling of an ADD. Returns NULL
 if not a terminal case; ceil(f) otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addMonadicApply]
 
 ******************************************************************************/
DdNode *
Cudd_addCeil(
             DdManager * dd,
             DdNode * f)
{
    if (cuddIsConstant(f)) {
        CUDD_VALUE_TYPE value = ceil(cuddV(f));
        DdNode *res = cuddUniqueConst(dd,value);
        return(res);
    }
    return(NULL);
    
} /* end of Cudd_addCeiling */

/**Function********************************************************************
 
 Synopsis    [1 if f==g; 0 otherwise.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is 1 if f==g; 0 otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 ******************************************************************************/
DdNode *
Cudd_addEquals(
               DdManager * dd,
               DdNode ** f,
               DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == G) return(DD_ONE(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) return(DD_ZERO(dd));
    if (F > G) { /* swap f and g */
        *f = G;
        *g = F;
    }
    return(NULL);
    
} /* end of Cudd_addEquals */

/**Function********************************************************************
 
 Synopsis    [1 if f==g; 0 otherwise.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is 1 if f==g; 0 otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 Added 23/08/2016 by Christian Dehnert
 
 ******************************************************************************/
DdNode *
Cudd_addToBddEquals(
               DdManager * dd,
               DdNode ** f,
               DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == G) return(DD_ONE(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) return(Cudd_Not(DD_ONE(dd)));
    if (F > G) { /* swap f and g */
        *f = G;
        *g = F;
    }
    return(NULL);
    
} /* end of Cudd_addToBddEquals */

/**Function********************************************************************
 
 Synopsis    [1 if f!=g; 0 otherwise.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is 1 if f!=g; 0 otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 ******************************************************************************/
DdNode *
Cudd_addNotEquals(
                  DdManager * dd,
                  DdNode ** f,
                  DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == G) return(DD_ZERO(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) return(DD_ONE(dd));
    if (F > G) { /* swap f and g */
        *f = G;
        *g = F;
    }
    return(NULL);
    
} /* end of Cudd_addNotEquals */

/**Function********************************************************************
 
 Synopsis    [1 if f!=g; 0 otherwise.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is 1 if f!=g; 0 otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 Added 23/08/2016 by Christian Dehnert

 ******************************************************************************/
DdNode *
Cudd_addToBddNotEquals(
                  DdManager * dd,
                  DdNode ** f,
                  DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == G) return(Cudd_Not(DD_ONE(dd)));
    if (cuddIsConstant(F) && cuddIsConstant(G)) return(DD_ONE(dd));
    if (F > G) { /* swap f and g */
        *f = G;
        *g = F;
    }
    return(NULL);
    
} /* end of Cudd_addToBddNotEquals */

/**Function********************************************************************
 
 Synopsis    [1 if f>g; 0 otherwise.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is 1 if f>g; 0 otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 ******************************************************************************/
DdNode *
Cudd_addGreaterThan(
                    DdManager * dd,
                    DdNode ** f,
                    DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == G) return(DD_ZERO(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
        if (cuddV(F)>cuddV(G)) return (DD_ONE(dd)); else return (DD_ZERO(dd));
    }
    return(NULL);
    
} /* end of Cudd_addGreaterThan */

/**Function********************************************************************
 
 Synopsis    [1 if f>g; 0 otherwise.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is 1 if f>g; 0 otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 Added 23/08/2016 by Christian Dehnert

 ******************************************************************************/
DdNode *
Cudd_addToBddGreaterThan(
                    DdManager * dd,
                    DdNode ** f,
                    DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == G) return(Cudd_Not(DD_ONE(dd)));
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
        if (cuddV(F)>cuddV(G)) return (DD_ONE(dd)); else return (Cudd_Not(DD_ONE(dd)));
    }
    return(NULL);
    
} /* end of Cudd_addToBddGreaterThan */


/**Function********************************************************************
 
 Synopsis    [1 if f>=g; 0 otherwise.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is 1 if f>=g; 0 otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 ******************************************************************************/
DdNode *
Cudd_addGreaterThanEquals(
                          DdManager * dd,
                          DdNode ** f,
                          DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == G) return(DD_ONE(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
        if (cuddV(F)>=cuddV(G)) return (DD_ONE(dd)); else return (DD_ZERO(dd));
    }
    return(NULL);
    
} /* end of Cudd_addGreaterThanEquals */

/**Function********************************************************************
 
 Synopsis    [1 if f>=g; 0 otherwise.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is 1 if f>=g; 0 otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 Added 23/08/2016 by Christian Dehnert

 ******************************************************************************/
DdNode *
Cudd_addToBddGreaterThanEquals(
                          DdManager * dd,
                          DdNode ** f,
                          DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == G) return(DD_ONE(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
        if (cuddV(F)>=cuddV(G)) return (DD_ONE(dd)); else return (Cudd_Not(DD_ONE(dd)));
    }
    return(NULL);
    
} /* end of Cudd_addToBddGreaterThanEquals */

/**Function********************************************************************
 
 Synopsis    [1 if f<g; 0 otherwise.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is 1 if f<g; 0 otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 ******************************************************************************/
DdNode *
Cudd_addLessThan(
                 DdManager * dd,
                 DdNode ** f,
                 DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == G) return(DD_ZERO(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
        if (cuddV(F)<cuddV(G)) return (DD_ONE(dd)); else return (DD_ZERO(dd));
    }
    return(NULL);
    
} /* end of Cudd_addLessThan */

/**Function********************************************************************
 
 Synopsis    [1 if f<g; 0 otherwise.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is 1 if f<g; 0 otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 Added 23/08/2016 by Christian Dehnert

 ******************************************************************************/
DdNode *
Cudd_addToBddLessThan(
                 DdManager * dd,
                 DdNode ** f,
                 DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == G) return(Cudd_Not(DD_ONE(dd)));
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
        if (cuddV(F)<cuddV(G)) return (DD_ONE(dd)); else return (Cudd_Not(DD_ONE(dd)));
    }
    return(NULL);
    
} /* end of Cudd_addToBddLessThan */

/**Function********************************************************************
 
 Synopsis    [1 if f<=g; 0 otherwise.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is 1 if f<=g; 0 otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 ******************************************************************************/
DdNode *
Cudd_addLessThanEquals(
                       DdManager * dd,
                       DdNode ** f,
                       DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == G) return(DD_ONE(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
        if (cuddV(F)<=cuddV(G)) return (DD_ONE(dd)); else return (DD_ZERO(dd));
    }
    return(NULL);
    
} /* end of Cudd_addLessThanEquals */

/**Function********************************************************************
 
 Synopsis    [1 if f<=g; 0 otherwise.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is 1 if f<=g; 0 otherwise.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 Added 23/08/2016 by Christian Dehnert

 ******************************************************************************/
DdNode *
Cudd_addToBddLessThanEquals(
                       DdManager * dd,
                       DdNode ** f,
                       DdNode ** g)
{
    DdNode *F, *G;
    
    F = *f; G = *g;
    if (F == G) return(DD_ONE(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
        if (cuddV(F)<=cuddV(G)) return (DD_ONE(dd)); else return (Cudd_Not(DD_ONE(dd)));
    }
    return(NULL);
    
} /* end of Cudd_addToBddLessThanEquals */

/**Function********************************************************************
 
 Synopsis    [f to the power of g.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is f to the power of g.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 ******************************************************************************/
DdNode *
Cudd_addPow(
            DdManager * dd,
            DdNode ** f,
            DdNode ** g)
{
    DdNode *res;
    DdNode *F, *G;
    CUDD_VALUE_TYPE value;
    
    F = *f; G = *g;
    if (G == DD_ZERO(dd)) return(DD_ONE(dd));
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
        value = pow(cuddV(F), cuddV(G));
        res = cuddUniqueConst(dd,value);
        return(res);
    }
    return(NULL);
    
} /* end of Cudd_addPow */


/**Function********************************************************************
 
 Synopsis    [f modulo g.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is f modulo g.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 ******************************************************************************/
DdNode *
Cudd_addMod(
            DdManager * dd,
            DdNode ** f,
            DdNode ** g)
{
    DdNode *res;
    DdNode *F, *G;
    int rem;
    CUDD_VALUE_TYPE value;
    
    F = *f; G = *g;
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
        // If g is <=0, then result is NaN
        if (cuddV(G) <= 0) value = (NAN);
        // Take care of negative case (% is remainder, not modulo)
        else {
            rem = ((int)cuddV(F) % (int)cuddV(G));
            if (rem < 0) rem += (int)cuddV(G);
            value = rem;
        }
        // Create/return result
        res = cuddUniqueConst(dd,value);
        return(res);
    }
    return(NULL);
    
} /* end of Cudd_addMod */


/**Function********************************************************************
 
 Synopsis    [log f base g.]
 
 Description [Returns NULL if not a terminal case; f op g otherwise,
 where f op g is log f base g.]
 
 SideEffects [None]
 
 SeeAlso     [Cudd_addApply]
 
 ******************************************************************************/
DdNode *
Cudd_addLogXY(
              DdManager * dd,
              DdNode ** f,
              DdNode ** g)
{
    DdNode *res;
    DdNode *F, *G;
    CUDD_VALUE_TYPE value;
    
    F = *f; G = *g;
    if (cuddIsConstant(F) && cuddIsConstant(G)) {
        // If base is <=0 or ==1 (or +Inf/NaN), then result is NaN
        if (cuddV(G) <= 0 || cuddV(G) == 1.0 || G==DD_PLUS_INFINITY(dd) || cuddV(G) != cuddV(G)) value = (NAN);
        // If arg is <0 or NaN, then result is NaN
        else if (cuddV(F) < 0 || cuddV(F) != cuddV(F)) value = (NAN);
        // If arg is +Inf, then result is +Inf
        else if (F==DD_PLUS_INFINITY(dd)) return DD_PLUS_INFINITY(dd);
        // If arg is (positive/negative) 0, then result is -Inf
        else if (cuddV(F) == 0.0 || cuddV(F) == -0.0) return DD_MINUS_INFINITY(dd);
        // Default case: normal log
        else value = log(cuddV(F)) / log(cuddV(G));
        // Create/return result
        res = cuddUniqueConst(dd,value);
        return(res);
    }
    return(NULL);
    
} /* end of Cudd_addLogXY */

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/


/**
  @brief Performs the recursive step of Cudd_addApply.

  @return a pointer to the result if successful; NULL otherwise.

  @sideeffect None

  @see cuddAddMonadicApplyRecur

*/
DdNode *
cuddAddApplyRecur(
  DdManager * dd,
  DD_AOP op,
  DdNode * f,
  DdNode * g)
{
    DdNode *res,
	   *fv, *fvn, *gv, *gvn,
	   *T, *E;
    int ford, gord;
    unsigned int index;
    DD_CTFP cacheOp;

    /* Check terminal cases. Op may swap f and g to increase the
     * cache hit rate.
     */
    statLine(dd);
    res = (*op)(dd,&f,&g);
    if (res != NULL) return(res);

    /* Check cache. */
    cacheOp = (DD_CTFP) op;
    res = cuddCacheLookup2(dd,cacheOp,f,g);
    if (res != NULL) return(res);

    checkWhetherToGiveUp(dd);

    /* Recursive step. */
    ford = cuddI(dd,f->index);
    gord = cuddI(dd,g->index);
    if (ford <= gord) {
	index = f->index;
	fv = cuddT(f);
	fvn = cuddE(f);
    } else {
	index = g->index;
	fv = fvn = f;
    }
    if (gord <= ford) {
	gv = cuddT(g);
	gvn = cuddE(g);
    } else {
	gv = gvn = g;
    }

    T = cuddAddApplyRecur(dd,op,fv,gv);
    if (T == NULL) return(NULL);
    cuddRef(T);

    E = cuddAddApplyRecur(dd,op,fvn,gvn);
    if (E == NULL) {
	Cudd_RecursiveDeref(dd,T);
	return(NULL);
    }
    cuddRef(E);

    res = (T == E) ? T : cuddUniqueInter(dd,(int)index,T,E);
    if (res == NULL) {
	Cudd_RecursiveDeref(dd, T);
	Cudd_RecursiveDeref(dd, E);
	return(NULL);
    }
    cuddDeref(T);
    cuddDeref(E);

    /* Store result. */
    cuddCacheInsert2(dd,cacheOp,f,g,res);

    return(res);

} /* end of cuddAddApplyRecur */

/**
 @brief Performs the recursive step of Cudd_addToBddApply.
 
 @return a pointer to the result if successful; NULL otherwise.
 
 @sideeffect None
 
 @see cuddAddMonadicApplyRecur
 
 added 23/08/2016 by Christian Dehnert
 
 */
DdNode *
cuddAddToBddApplyRecur(
                  DdManager * dd,
                  DD_AOP op,
                  DdNode * f,
                  DdNode * g)
{
    DdNode *res,
	   *fv, *fvn, *gv, *gvn,
	   *T, *E;
    int ford, gord;
    unsigned int index;
    DD_CTFP cacheOp;
    
    /* Check terminal cases. Op may swap f and g to increase the
     * cache hit rate.
     */
    statLine(dd);
    res = (*op)(dd,&f,&g);
    if (res != NULL) return(res);
    
    /* Check cache. */
    cacheOp = (DD_CTFP) op;
    res = cuddCacheLookup2(dd,cacheOp,f,g);
    if (res != NULL) return(res);
    
    checkWhetherToGiveUp(dd);
    
    /* Recursive step. */
    ford = cuddI(dd,f->index);
    gord = cuddI(dd,g->index);
    if (ford <= gord) {
        index = f->index;
        fv = cuddT(f);
        fvn = cuddE(f);
    } else {
        index = g->index;
        fv = fvn = f;
    }
    if (gord <= ford) {
        gv = cuddT(g);
        gvn = cuddE(g);
    } else {
        gv = gvn = g;
    }
    
    T = cuddAddToBddApplyRecur(dd,op,fv,gv);
    if (T == NULL) return(NULL);
    cuddRef(T);
    
    E = cuddAddToBddApplyRecur(dd,op,fvn,gvn);
    if (E == NULL) {
        Cudd_IterDerefBdd(dd,T);
        return(NULL);
    }
    cuddRef(E);
    
    int complT = Cudd_IsComplement(T);
    
    if (T == E) {
        res = T;
    } else {
        res = cuddUniqueInter(dd,(int)index,Cudd_Regular(T),complT ? Cudd_Not(E) : E);
        if (complT) {
            res = Cudd_Not(res);
        }
    }
    if (res == NULL) {
        Cudd_IterDerefBdd(dd, T);
        Cudd_IterDerefBdd(dd, E);
        return(NULL);
    }
    cuddRef(res);
    cuddDeref(T);
    cuddDeref(E);
    
    /* Store result. */
    cuddCacheInsert2(dd,cacheOp,f,g,res);
    
    cuddDeref(res);
    return(res);
    
} /* end of cuddAddToBddApplyRecur */

/**
  @brief Performs the recursive step of Cudd_addMonadicApply.

  @return a pointer to the result if successful; NULL otherwise.

  @sideeffect None

  @see cuddAddApplyRecur

*/
DdNode *
cuddAddMonadicApplyRecur(
  DdManager * dd,
  DD_MAOP op,
  DdNode * f)
{
    DdNode *res, *ft, *fe, *T, *E;
    unsigned int index;

    /* Check terminal cases. */
    statLine(dd);
    res = (*op)(dd,f);
    if (res != NULL) return(res);

    /* Check cache. */
    res = cuddCacheLookup1(dd,op,f);
    if (res != NULL) return(res);

    checkWhetherToGiveUp(dd);

    /* Recursive step. */
    index = f->index;
    ft = cuddT(f);
    fe = cuddE(f);

    T = cuddAddMonadicApplyRecur(dd,op,ft);
    if (T == NULL) return(NULL);
    cuddRef(T);

    E = cuddAddMonadicApplyRecur(dd,op,fe);
    if (E == NULL) {
	Cudd_RecursiveDeref(dd,T);
	return(NULL);
    }
    cuddRef(E);

    res = (T == E) ? T : cuddUniqueInter(dd,(int)index,T,E);
    if (res == NULL) {
	Cudd_RecursiveDeref(dd, T);
	Cudd_RecursiveDeref(dd, E);
	return(NULL);
    }
    cuddDeref(T);
    cuddDeref(E);

    /* Store result. */
    cuddCacheInsert1(dd,op,f,res);

    return(res);

} /* end of cuddAddMonadicApplyRecur */


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
