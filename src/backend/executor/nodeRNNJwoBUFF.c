/*-------------------------------------------------------------------------
 *
 * nodeMergejoin.c
 *	  routines supporting merge joins
 *
 * Portions Copyright (c) 1996-2012, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/executor/nodeMergejoin.c
 *
 *-------------------------------------------------------------------------
 */
/*
 * INTERFACE ROUTINES
 *		ExecMergeJoin			mergejoin outer and inner relations.
 *		ExecInitMergeJoin		creates and initializes run time states
 *		ExecEndMergeJoin		cleans up the node.
 *
 * NOTES
 *
 *		Merge-join is done by joining the inner and outer tuples satisfying
 *		join clauses of the form ((= outerKey innerKey) ...).
 *		The join clause list is provided by the query planner and may contain
 *		more than one (= outerKey innerKey) clause (for composite sort key).
 *
 *		However, the query executor needs to know whether an outer
 *		tuple is "greater/smaller" than an inner tuple so that it can
 *		"synchronize" the two relations. For example, consider the following
 *		relations:
 *
 *				outer: (0 ^1 1 2 5 5 5 6 6 7)	current tuple: 1
 *				inner: (1 ^3 5 5 5 5 6)			current tuple: 3
 *
 *		To continue the merge-join, the executor needs to scan both inner
 *		and outer relations till the matching tuples 5. It needs to know
 *		that currently inner tuple 3 is "greater" than outer tuple 1 and
 *		therefore it should scan the outer relation first to find a
 *		matching tuple and so on.
 *
 *		Therefore, rather than directly executing the merge join clauses,
 *		we evaluate the left and right key expressions separately and then
 *		compare the columns one at a time (see MJCompare).	The planner
 *		passes us enough information about the sort ordering of the inputs
 *		to allow us to determine how to make the comparison.  We may use the
 *		appropriate btree comparison function, since Postgres' only notion
 *		of ordering is specified by btree opfamilies.
 *
 *
 *		Consider the above relations and suppose that the executor has
 *		just joined the first outer "5" with the last inner "5". The
 *		next step is of course to join the second outer "5" with all
 *		the inner "5's". This requires repositioning the inner "cursor"
 *		to point at the first inner "5". This is done by "marking" the
 *		first inner 5 so we can restore the "cursor" to it before joining
 *		with the second outer 5. The access method interface provides
 *		routines to mark and restore to a tuple.
 *
 *
 *		Essential operation of the merge join algorithm is as follows:
 *
 *		Join {
 *			get initial outer and inner tuples				INITIALIZE
 *			do forever {
 *				while (outer != inner) {					SKIP_TEST
 *					if (outer < inner)
 *						advance outer						SKIPOUTER_ADVANCE
 *					else
 *						advance inner						SKIPINNER_ADVANCE
 *				}
 *				mark inner position							SKIP_TEST
 *				do forever {
 *					while (outer == inner) {
 *						join tuples							JOINTUPLES
 *						advance inner position				NEXTINNER
 *					}
 *					advance outer position					NEXTOUTER
 *					if (outer == mark)						TESTOUTER
 *						restore inner position to mark		TESTOUTER
 *					else
 *						break	// return to top of outer loop
 *				}
 *			}
 *		}
 *
 *		The merge join operation is coded in the fashion
 *		of a state machine.  At each state, we do something and then
 *		proceed to another state.  This state is stored in the node's
 *		execution state information and is preserved across calls to
 *		ExecMergeJoin. -cim 10/31/89
 */
#include "postgres.h"

#include "access/htup_details.h"
#include "access/nbtree.h"
#include "executor/execdebug.h"
#include "executor/nodeRNNJoinwoBUFF.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"

/*
 * States of the ExecMergeJoin state machine
 */
#define EXEC_MJ_INITIALIZE_OUTER		1
#define EXEC_MJ_INITIALIZE_INNER		2
#define EXEC_MJ_JOINTUPLES				3
#define EXEC_MJ_NEXTOUTER				4
#define EXEC_MJ_TESTOUTER				5
#define EXEC_MJ_NEXTINNER				6
#define EXEC_MJ_SKIP_TEST				7
#define EXEC_MJ_SKIPOUTER_ADVANCE		8
#define EXEC_MJ_SKIPINNER_ADVANCE		9
#define EXEC_MJ_ENDOUTER				10
#define EXEC_MJ_ENDINNER				11

/*
 * Runtime data for each mergejoin clause
 */
typedef struct MergeJoinClauseData {
	/* Executable expression trees */
	ExprState *lexpr; /* left-hand (outer) input expression */
	ExprState *rexpr; /* right-hand (inner) input expression */

	/*
	 * If we have a current left or right input tuple, the values of the
	 * expressions are loaded into these fields:
	 */
	Datum ldatum; /* current left-hand value */
	Datum rdatum; /* current right-hand value */
	bool lisnull; /* and their isnull flags */
	bool risnull;

	/*
	 * Everything we need to know to compare the left and right values is
	 * stored here.
	 */
	SortSupportData ssup;
} MergeJoinClauseData;

/* Result type for MJEvalOuterValues and MJEvalInnerValues */
typedef enum {
	MJEVAL_MATCHABLE, /* normal, potentially matchable tuple */
	MJEVAL_NONMATCHABLE, /* tuple cannot join because it has a null */
	MJEVAL_ENDOFJOIN
/* end of input (physical or effective) */
} MJEvalResult;

#define MarkInnerTuple(innerTupleSlot, mergestate) \
	ExecCopySlot((mergestate)->mj_MarkedTupleSlot, (innerTupleSlot))

/* forward declaration */

int RNNJWoBuffCompareSample(RNNJoinState *node, TupleTableSlot *outer,
	 TupleTableSlot *inner);

int
RNNJWoBuffCompareGroup(RNNJoinState *node, TupleTableSlot *outer,
		TupleTableSlot *inner);
int
RNNJWoBuffCompareTOuterInner(RNNJoinState *node, TupleTableSlot *outer,
		TupleTableSlot *inner);
int
RNNJWoBuffCompareTInnerInner(RNNJoinState *node, TupleTableSlot *inner1,
		TupleTableSlot *inner2);
int
RNNJWoBuffTimeDist(RNNJoinState *node, HeapTuple outer, TupleDesc outerDesc,
		TupleTableSlot *inner);

int RNNJWoBuffTimeDistOutInn(RNNJoinState *node, TupleTableSlot *outer,
		TupleTableSlot *inner);

int RNNJWoBuffCompareGroupBufInn(RNNJoinState *node, HeapTuple outer, TupleDesc outerDesc,
		TupleTableSlot *inner);

int RNNJWoBuffCompareGroupInnInn(RNNJoinState *node, TupleTableSlot *inner1,
		TupleTableSlot *inner);


static void MJReorderPositions(List *outer_tlist, List * inner_tlist, RNNJoinState *node);

/*
 * MJExamineQuals
 *
 * This deconstructs the list of mergejoinable expressions, which is given
 * to us by the planner in the form of a list of "leftexpr = rightexpr"
 * expression trees in the order matching the sort columns of the inputs.
 * We build an array of MergeJoinClause structs containing the information
 * we will need at runtime.  Each struct essentially tells us how to compare
 * the two expressions from the original clause.
 *
 * In addition to the expressions themselves, the planner passes the btree
 * opfamily OID, collation OID, btree strategy number (BTLessStrategyNumber or
 * BTGreaterStrategyNumber), and nulls-first flag that identify the intended
 * sort ordering for each merge key.  The mergejoinable operator is an
 * equality operator in the opfamily, and the two inputs are guaranteed to be
 * ordered in either increasing or decreasing (respectively) order according
 * to the opfamily and collation, with nulls at the indicated end of the range.
 * This allows us to obtain the needed comparison function from the opfamily.
 */
static MergeJoinClause MJExamineQuals(List *mergeclauses, Oid *mergefamilies,
		Oid *mergecollations, int *mergestrategies, bool *mergenullsfirst,
		PlanState *parent) {
	MergeJoinClause clauses;
	int nClauses = list_length(mergeclauses);
	int iClause;
	ListCell *cl;

	clauses = (MergeJoinClause) palloc0(nClauses * sizeof(MergeJoinClauseData));

	iClause = 0;
	foreach(cl, mergeclauses) {
		OpExpr *qual = (OpExpr *) lfirst(cl);
		MergeJoinClause clause = &clauses[iClause];
		Oid opfamily = mergefamilies[iClause];
		Oid collation = mergecollations[iClause];
		StrategyNumber opstrategy = mergestrategies[iClause];
		bool nulls_first = mergenullsfirst[iClause];
		int op_strategy;
		Oid op_lefttype;
		Oid op_righttype;
		Oid sortfunc;

		if (!IsA(qual, OpExpr))
			elog(ERROR, "mergejoin clause is not an OpExpr");

			/*
			 * Prepare the input expressions for execution.
			 */
			clause->lexpr = ExecInitExpr((Expr *) linitial(qual->args), parent);
			clause->rexpr = ExecInitExpr((Expr *) lsecond(qual->args), parent);

			/* Set up sort support data */
			clause->ssup.ssup_cxt = CurrentMemoryContext;
			clause->ssup.ssup_collation = collation;
			if (opstrategy == BTLessStrategyNumber)
			clause->ssup.ssup_reverse = false;
			else if (opstrategy == BTGreaterStrategyNumber)
			clause->ssup.ssup_reverse = true;
			else /* planner screwed up */
			elog(ERROR, "unsupported mergejoin strategy %d", opstrategy);
			clause->ssup.ssup_nulls_first = nulls_first;

			/* Extract the operator's declared left/right datatypes */
			get_op_opfamily_properties(qual->opno, opfamily, false,
					&op_strategy,
					&op_lefttype,
					&op_righttype);
			if (op_strategy != BTEqualStrategyNumber) /* should not happen */
			elog(ERROR, "cannot merge using non-equality operator %u",
					qual->opno);

			/* And get the matching support or comparison function */
			sortfunc = get_opfamily_proc(opfamily,
					op_lefttype,
					op_righttype,
					BTSORTSUPPORT_PROC);
			if (OidIsValid(sortfunc))
			{
				/* The sort support function should provide a comparator */
				OidFunctionCall1(sortfunc, PointerGetDatum(&clause->ssup));
				Assert(clause->ssup.comparator != NULL);
			}
			else
			{
				/* opfamily doesn't provide sort support, get comparison func */
				sortfunc = get_opfamily_proc(opfamily,
						op_lefttype,
						op_righttype,
						BTORDER_PROC);
				if (!OidIsValid(sortfunc)) /* should not happen */
				elog(ERROR, "missing support function %d(%u,%u) in opfamily %u",
						BTORDER_PROC, op_lefttype, op_righttype, opfamily);
				/* We'll use a shim to call the old-style btree comparator */
				PrepareSortSupportComparisonShim(sortfunc, &clause->ssup);
			}

			iClause++;
		}

		return clauses;
	}

			/*
			 * MJEvalOuterValues
			 *
			 * Compute the values of the mergejoined expressions for the current
			 * outer tuple.  We also detect whether it's impossible for the current
			 * outer tuple to match anything --- this is true if it yields a NULL
			 * input, since we assume mergejoin operators are strict.  If the NULL
			 * is in the first join column, and that column sorts nulls last, then
			 * we can further conclude that no following tuple can match anything
			 * either, since they must all have nulls in the first column.	However,
			 * that case is only interesting if we're not in FillOuter mode, else
			 * we have to visit all the tuples anyway.
			 *
			 * For the convenience of callers, we also make this routine responsible
			 * for testing for end-of-input (null outer tuple), and returning
			 * MJEVAL_ENDOFJOIN when that's seen.  This allows the same code to be used
			 * for both real end-of-input and the effective end-of-input represented by
			 * a first-column NULL.
			 *
			 * We evaluate the values in OuterEContext, which can be reset each
			 * time we move to a new tuple.
			 */


static MJEvalResult MJEvalOuterValues(RNNJoinState *mergestate) {
	ExprContext *econtext = mergestate->mj_OuterEContext;
	MJEvalResult result = MJEVAL_MATCHABLE;
	int i;
	MemoryContext oldContext;

	/* Check for end of outer subplan */
	if (TupIsNull(mergestate->mj_OuterTupleSlot))
		return MJEVAL_ENDOFJOIN;

	ResetExprContext(econtext);

	oldContext = MemoryContextSwitchTo(econtext->ecxt_per_tuple_memory);

	econtext->ecxt_outertuple = mergestate->mj_OuterTupleSlot;

	for (i = 0; i < mergestate->mj_NumClauses; i++) {
		MergeJoinClause clause = &mergestate->mj_Clauses[i];

		clause->ldatum = ExecEvalExpr(clause->lexpr, econtext,
				&clause->lisnull, NULL);
		if (clause->lisnull) {
			/* match is impossible; can we end the join early? */
			if (i == 0 && !clause->ssup.ssup_nulls_first
					&& !mergestate->mj_FillOuter)
				result = MJEVAL_ENDOFJOIN;
			else if (result == MJEVAL_MATCHABLE)
				result = MJEVAL_NONMATCHABLE;
		}
	}

	MemoryContextSwitchTo(oldContext);

	return result;
}

/*
 * MJCompare
 *
 * Compare the mergejoinable values of the current two input tuples
 * and return 0 if they are equal (ie, the mergejoin equalities all
 * succeed), >0 if outer > inner, <0 if outer < inner.
 *
 * MJEvalOuterValues and MJEvalInnerValues must already have been called
 * for the current outer and inner tuples, respectively.
 */
static int MJCompare(RNNJoinState *mergestate) {
	int result = 0;
	bool nulleqnull = false;
	ExprContext *econtext = mergestate->js.ps.ps_ExprContext;
	int i;
	MemoryContext oldContext;

	/*
	 * Call the comparison functions in short-lived context, in case they leak
	 * memory.
	 */ResetExprContext(econtext);

	oldContext = MemoryContextSwitchTo(econtext->ecxt_per_tuple_memory);

	for (i = 0; i < mergestate->mj_NumClauses; i++) {
		MergeJoinClause clause = &mergestate->mj_Clauses[i];

		/*
		 * Special case for NULL-vs-NULL, else use standard comparison.
		 */
		if (clause->lisnull && clause->risnull) {
			nulleqnull = true; /* NULL "=" NULL */
			continue;
		}

		result = ApplySortComparator(clause->ldatum, clause->lisnull,
				clause->rdatum, clause->risnull, &clause->ssup);

		if (result != 0)
			break;
	}

	/*
	 * If we had any NULL-vs-NULL inputs, we do not want to report that the
	 * tuples are equal.  Instead, if result is still 0, change it to +1. This
	 * will result in advancing the inner side of the join.
	 *
	 * Likewise, if there was a constant-false joinqual, do not report
	 * equality.  We have to check this as part of the mergequals, else the
	 * rescan logic will do the wrong thing.
	 */
	if (result == 0 && (nulleqnull || mergestate->mj_ConstFalseJoin))
		result = 1;

	MemoryContextSwitchTo(oldContext);

	return result;
}

/*
 * Generate a fake join tuple with nulls for the inner tuple,
 * and return it if it passes the non-join quals.
 */
static TupleTableSlot *
MJFillOuter(RNNJoinState *node) {
	ExprContext *econtext = node->js.ps.ps_ExprContext;
	List *otherqual = node->js.ps.qual;

	ResetExprContext(econtext);

	econtext->ecxt_outertuple = node->mj_OuterTupleSlot;
	econtext->ecxt_innertuple = node->mj_NullInnerTupleSlot;

	if (ExecQual(otherqual, econtext, false)) {
		/*
		 * qualification succeeded.  now form the desired projection tuple and
		 * return the slot containing it.
		 */
		TupleTableSlot *result;
		ExprDoneCond isDone;

		MJ_printf("ExecMergeJoin: returning outer fill tuple\n");

		result = ExecProject(node->js.ps.ps_ProjInfo, &isDone);

		if (isDone != ExprEndResult) {
			node->js.ps.ps_TupFromTlist = (isDone == ExprMultipleResult);
			return result;
		}
	} else
		InstrCountFiltered2(node, 1);

	return NULL;
}

/*
 * Generate a fake join tuple with nulls for the outer tuple,
 * and return it if it passes the non-join quals.
 */
static TupleTableSlot *
MJFillInner(RNNJoinState *node) {
	ExprContext *econtext = node->js.ps.ps_ExprContext;
	List *otherqual = node->js.ps.qual;

	ResetExprContext(econtext);

	econtext->ecxt_outertuple = node->mj_NullOuterTupleSlot;
	econtext->ecxt_innertuple = node->mj_InnerTupleSlot;

	if (ExecQual(otherqual, econtext, false)) {
		/*
		 * qualification succeeded.  now form the desired projection tuple and
		 * return the slot containing it.
		 */
		TupleTableSlot *result;
		ExprDoneCond isDone;

		MJ_printf("ExecMergeJoin: returning inner fill tuple\n");

		result = ExecProject(node->js.ps.ps_ProjInfo, &isDone);

		if (isDone != ExprEndResult) {
			node->js.ps.ps_TupFromTlist = (isDone == ExprMultipleResult);
			return result;
		}
	} else
		InstrCountFiltered2(node, 1);

	return NULL;
}

/*
 * Check that a qual condition is constant true or constant false.
 * If it is constant false (or null), set *is_const_false to TRUE.
 *
 * Constant true would normally be represented by a NIL list, but we allow an
 * actual bool Const as well.  We do expect that the planner will have thrown
 * away any non-constant terms that have been ANDed with a constant false.
 */
static bool check_constant_qual(List *qual, bool *is_const_false) {
	ListCell *lc;

	foreach(lc, qual) {
		Const *con = (Const *) lfirst(lc);

		if (!con || !IsA(con, Const))
			return false;
		if (con->constisnull || !DatumGetBool(con->constvalue))
			*is_const_false = true;
	}
	return true;
}

/* ----------------------------------------------------------------
 *		ExecMergeTupleDump
 *
 *		This function is called through the MJ_dump() macro
 *		when EXEC_MERGEJOINDEBUG is defined
 * ----------------------------------------------------------------
 */
#ifdef EXEC_MERGEJOINDEBUG

static void ExecMergeTupleDumpOuter(RNNJoinState *mergestate) {
	TupleTableSlot *outerSlot = mergestate->mj_OuterTupleSlot;

	printf("==== outer tuple ====\n");
	if (TupIsNull(outerSlot))
		printf("(nil)\n");
	else
		MJ_debugtup(outerSlot);
}

static void ExecMergeTupleDumpInner(RNNJoinState *mergestate) {
	TupleTableSlot *innerSlot = mergestate->mj_InnerTupleSlot;

	printf("==== Next inner tuple ====\n");
	if (TupIsNull(innerSlot))
		printf("(nil)\n");
	else
		MJ_debugtup(innerSlot);
}

static void ExecMergeTupleDumpPrevInner(RNNJoinState *mergestate) {
	TupleTableSlot *innerSlot = mergestate->mj_PrevInnerTupleSlot;

	printf("==== Prev inner tuple ====\n");
	if (TupIsNull(innerSlot))
		printf("(nil)\n");
	else
		MJ_debugtup(innerSlot);
}

static void ExecMergeTupleDumpCurrInner(RNNJoinState *mergestate) {
	TupleTableSlot *innerSlot = mergestate->mj_CurrInnerTupleSlot;

	printf("==== Cur inner tuple ====\n");
	if (TupIsNull(innerSlot))
		printf("(nil)\n");
	else
		MJ_debugtup(innerSlot);
}

static void ExecMergeTupleDumpMarked(RNNJoinState *mergestate) {
	TupleTableSlot *markedSlot = mergestate->mj_MarkedTempTupleSlot;

	printf("==== marked tuple ====\n");
	if (TupIsNull(markedSlot))
		printf("(nil)\n");
	else
		MJ_debugtup(markedSlot);
}

static void ExecMergeTupleDump(RNNJoinState *mergestate) {
	printf("******** ExecMergeTupleDump ********\n");

	ExecMergeTupleDumpOuter(mergestate);
	ExecMergeTupleDumpPrevInner(mergestate);
	ExecMergeTupleDumpCurrInner(mergestate);
	ExecMergeTupleDumpInner(mergestate);
	ExecMergeTupleDumpMarked(mergestate);

	printf("******** \n");
}
#endif

/* ----------------------------------------------------------------
 *		ExecMergeJoin
 * ----------------------------------------------------------------
 */
TupleTableSlot *
ExecRNNJoinWoBuff(RNNJoinState *node) {
	List *joinqual;
	List *otherqual;
	bool qualResult;
	int compareResult;
	PlanState *innerPlan;
	TupleTableSlot *innerTupleSlot;
	PlanState *outerPlan;
	TupleTableSlot *outerTupleSlot;
	ExprContext *econtext;
	bool doFillOuter;
	bool doFillInner;

	/*
	 * get information from node
	 */
	innerPlan = innerPlanState(node);
	outerPlan = outerPlanState(node);
	econtext = node->js.ps.ps_ExprContext;
	joinqual = node->js.joinqual;
	otherqual = node->js.ps.qual;
	doFillOuter = node->mj_FillOuter;
	doFillInner = node->mj_FillInner;
	innerTupleSlot = node->mj_InnerTupleSlot;
	outerTupleSlot = node->mj_OuterTupleSlot;

	/*
	 * Check to see if we're still projecting out tuples from a previous join
	 * tuple (because there is a function-returning-set in the projection
	 * expressions).  If so, try to project another one.
	 */
	if (node->js.ps.ps_TupFromTlist) {
		TupleTableSlot *result;
		ExprDoneCond isDone;

		result = ExecProject(node->js.ps.ps_ProjInfo, &isDone);
		if (isDone == ExprMultipleResult)
			return result;
		/* Done with that source tuple... */
		node->js.ps.ps_TupFromTlist = false;
	}

	/*
	 * Reset per-tuple memory context to free any expression evaluation
	 * storage allocated in the previous tuple cycle.  Note this can't happen
	 * until we're done projecting out tuples from a join tuple.
	 */ResetExprContext(econtext);

	/*
	 * ok, everything is setup.. let's go to work
	 */
	for (;;) {
		MJ_dump(node);

		/*
		 * get the current state of the join and do things accordingly.
		 */
		switch (node->mj_JoinState) {
		/*
		 * EXEC_MJ_INITIALIZE_OUTER means that this is the first time
		 * ExecMergeJoin() has been called and so we have to fetch the
		 * first matchable tuple for both outer and inner subplans. We
		 * do the outer side in INITIALIZE_OUTER state, then advance
		 * to INITIALIZE_INNER state for the inner subplan.
		 */
		case EXEC_MJ_INITIALIZE_OUTER:
			MJ_printf("ExecMergeJoin: EXEC_MJ_INITIALIZE_OUTER\n");

			outerTupleSlot = ExecProcNode(outerPlan);
			node->mj_OuterTupleSlot = outerTupleSlot;
			/* Compute join values and check for unmatchability */
			node->mj_JoinState = EXEC_MJ_INITIALIZE_INNER;

			break;

		case EXEC_MJ_INITIALIZE_INNER:
			MJ_printf("ExecMergeJoin: EXEC_MJ_INITIALIZE_INNER\n");


			innerTupleSlot = ExecProcNode(innerPlan);
			ExecCopySlot(node->mj_CurrInnerTupleSlot, innerTupleSlot);
			ExecCopySlot(node->mj_MarkedTempTupleSlot, node->mj_CurrInnerTupleSlot);
			ExecMarkPos(innerPlan);
			innerTupleSlot = ExecProcNode(innerPlan);
			node->mj_InnerTupleSlot = innerTupleSlot;

			if (!TupIsNull(node->mj_CurrInnerTupleSlot)) {

					node->agg_count = 0;
					node->agg_sum = 0;
					node->agg_local_count = 0;
					node->agg_local_sum = 0;
					node->aggSamples=NIL;
					node->localAggSamples=NIL;
			}

			node->mj_JoinState = EXEC_MJ_NEXTINNER;

			break;

			/*
			 * EXEC_MJ_JOINTUPLES means we have two tuples which satisfied
			 * the merge clause so we join them and then proceed to get
			 * the next inner tuple (EXEC_MJ_NEXTINNER).
			 */
		case EXEC_MJ_JOINTUPLES:
			MJ_printf("ExecMergeJoin: EXEC_MJ_JOINTUPLES\n");

				MemoryContext oldContext;
				TupleTableSlot *result;
				ExprDoneCond isDone;
				HeapTuple new_tup;
				bool isNull;

				oldContext = MemoryContextSwitchTo(
						node->mj_OuterTempTupleSlot->tts_mcxt);
				MemoryContextSwitchTo(oldContext);
				econtext->ecxt_outertuple = node->mj_OuterTupleSlot;
				econtext->ecxt_innertuple = node->mj_CurrInnerTupleSlot;


				MJ_printf("ExecMergeJoin: returning tuple\n");
				result = ExecProject(node->js.ps.ps_ProjInfo, &isDone);

				if (isDone != ExprEndResult) {
					node->js.ps.ps_TupFromTlist = (isDone
							== ExprMultipleResult);
					if (TupIsNull(node->mj_InnerTupleSlot) || RNNJWoBuffTimeDistOutInn(
							node, node->mj_OuterTupleSlot,
							node->mj_CurrInnerTupleSlot) < RNNJWoBuffTimeDistOutInn(
									node, node->mj_OuterTupleSlot,
									node->mj_InnerTupleSlot) || RNNJWoBuffCompareGroupInnInn(node, node->mj_CurrInnerTupleSlot, node->mj_InnerTupleSlot)!=0){
						node->mj_JoinState=EXEC_MJ_NEXTOUTER;
					}else{
						ExecCopySlot(node->mj_CurrInnerTupleSlot, node->mj_InnerTupleSlot);
						innerTupleSlot = ExecProcNode(innerPlan);
						node->mj_InnerTupleSlot = innerTupleSlot;
					}
					return result;

				}

			break;

			/*
			 * EXEC_MJ_NEXTINNER means advance the inner scan to the next
			 * tuple. If the tuple is not nil, we then proceed to test it
			 * against the join qualification.
			 *
			 * Before advancing, we check to see if we must emit an
			 * outer-join fill tuple for this inner tuple.
			 */
		case EXEC_MJ_NEXTINNER:
			MJ_printf("ExecMergeJoin: EXEC_MJ_NEXTINNER_RNNJwoBUFF\n");
			if ((RNNJWoBuffCompareGroup(node, outerTupleSlot,
					node->mj_CurrInnerTupleSlot) == 0)
					&& (RNNJWoBuffCompareSample(node, node->mj_OuterTupleSlot,
							node->mj_CurrInnerTupleSlot) == 0)) {
				MJ_printf("ExecMergeJoin: EQUIJOIN MATCH \n");
				MemoryContext oldContext;
				TupleTableSlot *result;
				ExprDoneCond isDone;
				oldContext = MemoryContextSwitchTo(
						node->mj_OuterTupleSlot->tts_mcxt);
				MemoryContextSwitchTo(oldContext);
				econtext->ecxt_outertuple = node->mj_OuterTupleSlot;
				econtext->ecxt_innertuple = node->mj_CurrInnerTupleSlot;
				/* qualification succeeded.  now form the desired
				 projection tuple and return the slot containing it.*/

				MJ_printf("ExecMergeJoin: returning tuple\n");

				result = ExecProject(node->js.ps.ps_ProjInfo, &isDone);

				if (isDone != ExprEndResult) {
					node->js.ps.ps_TupFromTlist =
							(isDone == ExprMultipleResult);
					if (!TupIsNull(node->mj_InnerTupleSlot) && (RNNJWoBuffCompareGroup(node, outerTupleSlot,
										node->mj_InnerTupleSlot) == 0)
										&& (RNNJWoBuffCompareSample(node, node->mj_OuterTupleSlot,
												node->mj_InnerTupleSlot) == 0)) {
						node->mj_JoinState= EXEC_MJ_NEXTINNER;
						ExecCopySlot(node->mj_CurrInnerTupleSlot,
								node->mj_InnerTupleSlot);
						innerTupleSlot = ExecProcNode(innerPlan);
						node->mj_InnerTupleSlot = innerTupleSlot;
					}else
						node->mj_JoinState = EXEC_MJ_NEXTOUTER;
					return result;
				}
			} else {
				if (!TupIsNull(node->mj_InnerTupleSlot)) {
					if (RNNJWoBuffTimeDistOutInn(node, node->mj_OuterTupleSlot,
							node->mj_CurrInnerTupleSlot)
							> RNNJWoBuffTimeDistOutInn(node,
									node->mj_OuterTupleSlot,
										node->mj_InnerTupleSlot)) {

							ExecCopySlot(node->mj_MarkedTempTupleSlot,
									node->mj_InnerTupleSlot);
							ExecMarkPos(innerPlan);
						}
						ExecCopySlot(node->mj_CurrInnerTupleSlot,
								node->mj_InnerTupleSlot);
						innerTupleSlot = ExecProcNode(innerPlan);
						node->mj_InnerTupleSlot = innerTupleSlot;
					}

					if (TupIsNull(node->mj_InnerTupleSlot)
							|| RNNJWoBuffTimeDistOutInn(node,
									node->mj_OuterTupleSlot,
									node->mj_CurrInnerTupleSlot)
									< RNNJWoBuffTimeDistOutInn(node,
											node->mj_OuterTupleSlot,
											node->mj_InnerTupleSlot)
											|| RNNJWoBuffCompareGroupInnInn(node,
													node->mj_CurrInnerTupleSlot,
													node->mj_InnerTupleSlot) != 0) {
						if (((RNNJWoBuffCompareGroup(node, outerTupleSlot,
								node->mj_CurrInnerTupleSlot) == 0)
								&& (RNNJWoBuffCompareSample(node, node->mj_OuterTupleSlot,
										node->mj_CurrInnerTupleSlot) == 0))) {
							MJ_printf("ExecMergeJoin: EQUIJOIN MATCH \n");
							MemoryContext oldContext;
							TupleTableSlot *result;
							ExprDoneCond isDone;
							oldContext = MemoryContextSwitchTo(
									node->mj_OuterTupleSlot->tts_mcxt);
							MemoryContextSwitchTo(oldContext);
							econtext->ecxt_outertuple = node->mj_OuterTupleSlot;
							econtext->ecxt_innertuple = node->mj_CurrInnerTupleSlot;
							/* qualification succeeded.  now form the desired
									 projection tuple and return the slot containing it.*/

							MJ_printf("ExecMergeJoin: returning tuple\n");

							result = ExecProject(node->js.ps.ps_ProjInfo, &isDone);

							if (isDone != ExprEndResult) {
								node->js.ps.ps_TupFromTlist =
										(isDone == ExprMultipleResult);
								node->mj_JoinState = EXEC_MJ_NEXTOUTER;
								return result;
							}
						} else {
							ExecRestrPos(innerPlan);
							ExecCopySlot(node->mj_CurrInnerTupleSlot,
									node->mj_MarkedTempTupleSlot);
							innerTupleSlot = ExecProcNode(innerPlan);
							node->mj_InnerTupleSlot = innerTupleSlot;

							node->mj_JoinState = EXEC_MJ_JOINTUPLES;
						}
					} else {
						node->mj_JoinState = EXEC_MJ_NEXTINNER;
					}
			}

			 /* XXX */
			break;

			/*-------------------------------------------
			 * EXEC_MJ_NEXTOUTER means
			 *
			 *				outer inner
			 * outer tuple -  5		5  - marked tuple
			 *				  5		5
			 *				  6		6  - inner tuple
			 *				  7		7
			 *
			 * we know we just bumped into the
			 * first inner tuple > current outer tuple (or possibly
			 * the end of the inner stream)
			 * so get a new outer tuple and then
			 * proceed to test it against the marked tuple
			 * (EXEC_MJ_TESTOUTER)
			 *
			 * Before advancing, we check to see if we must emit an
			 * outer-join fill tuple for this outer tuple.
			 *------------------------------------------------
			 */
		case EXEC_MJ_NEXTOUTER:
			MJ_printf("ExecMergeJoin: EXEC_MJ_NEXTOUTER\n");

			outerTupleSlot = ExecProcNode(outerPlan);
			node->mj_OuterTupleSlot = outerTupleSlot;
			if (!TupIsNull(node->mj_OuterTupleSlot)){
				if (TupIsNull(node->mj_InnerTupleSlot) || RNNJWoBuffTimeDistOutInn(
						node, node->mj_OuterTupleSlot,
						node->mj_CurrInnerTupleSlot) <= RNNJWoBuffTimeDistOutInn(
						node, node->mj_OuterTupleSlot,
						node->mj_InnerTupleSlot)){
					ExecRestrPos(innerPlan);
					ExecCopySlot(node->mj_CurrInnerTupleSlot, node->mj_MarkedTempTupleSlot);
					innerTupleSlot = ExecProcNode(innerPlan);
					node->mj_InnerTupleSlot = innerTupleSlot;
				}
				node->mj_JoinState = EXEC_MJ_NEXTINNER;
			}
			else
				return NULL;








			break;

			/*--------------------------------------------------------
			 * EXEC_MJ_TESTOUTER If the new outer tuple and the marked
			 * tuple satisfy the merge clause then we know we have
			 * duplicates in the outer scan so we have to restore the
			 * inner scan to the marked tuple and proceed to join the
			 * new outer tuple with the inner tuples.
			 *
			 * This is the case when
			 *						  outer inner
			 *							4	  5  - marked tuple
			 *			 outer tuple -	5	  5
			 *		 new outer tuple -	5	  5
			 *							6	  8  - inner tuple
			 *							7	 12
			 *
			 *				new outer tuple == marked tuple
			 *
			 * If the outer tuple fails the test, then we are done
			 * with the marked tuples, and we have to look for a
			 * match to the current inner tuple.  So we will
			 * proceed to skip outer tuples until outer >= inner
			 * (EXEC_MJ_SKIP_TEST).
			 *
			 *		This is the case when
			 *
			 *						  outer inner
			 *							5	  5  - marked tuple
			 *			 outer tuple -	5	  5
			 *		 new outer tuple -	6	  8  - inner tuple
			 *							7	 12
			 *
			 *				new outer tuple > marked tuple
			 *
			 *---------------------------------------------------------
			 */

			/*
			 * EXEC_MJ_ENDOUTER means we have run out of outer tuples, but
			 * are doing a right/full join and therefore must null-fill
			 * any remaining unmatched inner tuples.
			 */
		case EXEC_MJ_ENDOUTER:
			MJ_printf("ExecMergeJoin: EXEC_MJ_ENDOUTER\n");

			Assert(doFillInner);

			if (!node->mj_MatchedInner) {
				/*
				 * Generate a fake join tuple with nulls for the outer
				 * tuple, and return it if it passes the non-join quals.
				 */
				TupleTableSlot *result;

				node->mj_MatchedInner = true; /* do it only once */

				result = MJFillInner(node);
				if (result)
					return result;
			}

			/* Mark before advancing, if wanted */
			if (node->mj_ExtraMarks)
				ExecMarkPos(innerPlan);

			/*
			 * now we get the next inner tuple, if any
			 */
			innerTupleSlot = ExecProcNode(innerPlan);
			node->mj_InnerTupleSlot = innerTupleSlot;
			MJ_DEBUG_PROC_NODE(innerTupleSlot);
			node->mj_MatchedInner = false;

			if (TupIsNull(innerTupleSlot)) {
				MJ_printf("ExecMergeJoin: end of inner subplan\n");
				return NULL;
			}

			/* Else remain in ENDOUTER state and process next tuple. */
			break;

			/*
			 * EXEC_MJ_ENDINNER means we have run out of inner tuples, but
			 * are doing a left/full join and therefore must null- fill
			 * any remaining unmatched outer tuples.
			 */
		case EXEC_MJ_ENDINNER:
			MJ_printf("ExecMergeJoin: EXEC_MJ_ENDINNER\n");

			Assert(doFillOuter);

			if (!node->mj_MatchedOuter) {
				/*
				 * Generate a fake join tuple with nulls for the inner
				 * tuple, and return it if it passes the non-join quals.
				 */
				TupleTableSlot *result;

				node->mj_MatchedOuter = true; /* do it only once */

				result = MJFillOuter(node);
				if (result)
					return result;
			}

			/*
			 * now we get the next outer tuple, if any
			 */
			outerTupleSlot = ExecProcNode(outerPlan);
			node->mj_OuterTupleSlot = outerTupleSlot;
			MJ_DEBUG_PROC_NODE(outerTupleSlot);
			node->mj_MatchedOuter = false;

			if (TupIsNull(outerTupleSlot)) {
				MJ_printf("ExecMergeJoin: end of outer subplan\n");
				return NULL;
			}

			/* Else remain in ENDINNER state and process next tuple. */
			break;

			/*
			 * broken state value?
			 */
		default:
			elog(ERROR, "unrecognized mergejoin state: %d",
			(int) node->mj_JoinState);
		}
	}
}

			/* ----------------------------------------------------------------
			 *		ExecInitMergeJoin
			 * ----------------------------------------------------------------
			 */
RNNJoinState *
ExecInitRNNJoinWoBuff(MergeJoin *node, EState *estate, int eflags) {
	RNNJoinState *mergestate;

	/* check for unsupported flags */
	Assert(!(eflags & (EXEC_FLAG_BACKWARD | EXEC_FLAG_MARK)));

	MJ1_printf("ExecInitMergeJoin: %s\n",
			"initializing node");

	/*
	 * create state structure
	 */
	mergestate = makeNode(RNNJoinState);
	mergestate->js.ps.plan = (Plan *) node;
	mergestate->js.ps.state = estate;

	/*
	 * Miscellaneous initialization
	 *
	 * create expression context for node
	 */
	ExecAssignExprContext(estate, &mergestate->js.ps);

	/*
	 * we need two additional econtexts in which we can compute the join
	 * expressions from the left and right input tuples.  The node's regular
	 * econtext won't do because it gets reset too often.
	 */
	mergestate->mj_OuterEContext = CreateExprContext(estate);
	mergestate->mj_InnerEContext = CreateExprContext(estate);

	/*
	 * initialize child expressions
	 */
	mergestate->js.ps.targetlist = (List *) ExecInitExpr(
			(Expr *) node->join.plan.targetlist, (PlanState *) mergestate);
	mergestate->js.ps.qual = (List *) ExecInitExpr(
			(Expr *) node->join.plan.qual, (PlanState *) mergestate);
	mergestate->js.jointype = node->join.jointype;
	mergestate->js.joinqual = (List *) ExecInitExpr(
			(Expr *) node->join.joinqual, (PlanState *) mergestate);
	mergestate->mj_ConstFalseJoin = false;
	/* mergeclauses are handled below */

	/*
	 * initialize child nodes
	 *
	 * inner child must support MARK/RESTORE.
	 */outerPlanState(mergestate) = ExecInitNode(outerPlan(node), estate, eflags);
	innerPlanState(mergestate) = ExecInitNode(innerPlan(node), estate,
			eflags | EXEC_FLAG_MARK);


	/*
	 * For certain types of inner child nodes, it is advantageous to issue
	 * MARK every time we advance past an inner tuple we will never return to.
	 * For other types, MARK on a tuple we cannot return to is a waste of
	 * cycles.	Detect which case applies and set mj_ExtraMarks if we want to
	 * issue "unnecessary" MARK calls.
	 *
	 * Currently, only Material wants the extra MARKs, and it will be helpful
	 * only if eflags doesn't specify REWIND.
	 */
	if (IsA(innerPlan(node), Material) && (eflags & EXEC_FLAG_REWIND) == 0)
		mergestate->mj_ExtraMarks = true;
	else
		mergestate->mj_ExtraMarks = false;

	/*
	 * tuple table initialization
	 */
	ExecInitResultTupleSlot(estate, &mergestate->js.ps);

	mergestate->mj_MarkedTupleSlot = ExecInitExtraTupleSlot(estate);
	ExecSetSlotDescriptor(mergestate->mj_MarkedTupleSlot,
			ExecGetResultType(innerPlanState(mergestate)));

	switch (node->join.jointype) {
	case JOIN_INNER:
	case JOIN_SEMI:
	case JOIN_RNNJ:
		mergestate->mj_FillOuter = false;
		mergestate->mj_FillInner = false;
		break;
	case JOIN_LEFT:
	case JOIN_ANTI:
		mergestate->mj_FillOuter = true;
		mergestate->mj_FillInner = false;
		mergestate->mj_NullInnerTupleSlot = ExecInitNullTupleSlot(estate,
				ExecGetResultType(innerPlanState(mergestate)));
		break;
	case JOIN_RIGHT:
		mergestate->mj_FillOuter = false;
		mergestate->mj_FillInner = true;
		mergestate->mj_NullOuterTupleSlot = ExecInitNullTupleSlot(estate,
				ExecGetResultType(outerPlanState(mergestate)));

		/*
		 * Can't handle right or full join with non-constant extra
		 * joinclauses.  This should have been caught by planner.
		 */
		if (!check_constant_qual(node->join.joinqual,
				&mergestate->mj_ConstFalseJoin))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							errmsg("RIGHT JOIN is only supported with merge-joinable join conditions")));
		break;
	case JOIN_FULL:
		mergestate->mj_FillOuter = true;
		mergestate->mj_FillInner = true;
		mergestate->mj_NullOuterTupleSlot = ExecInitNullTupleSlot(estate,
				ExecGetResultType(outerPlanState(mergestate)));
		mergestate->mj_NullInnerTupleSlot = ExecInitNullTupleSlot(estate,
				ExecGetResultType(innerPlanState(mergestate)));

		/*
		 * Can't handle right or full join with non-constant extra
		 * joinclauses.  This should have been caught by planner.
		 */
		if (!check_constant_qual(node->join.joinqual,
				&mergestate->mj_ConstFalseJoin))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							errmsg("FULL JOIN is only supported with merge-joinable join conditions")));
		break;
	default:
		elog(ERROR, "unrecognized join type: %d",
		(int) node->join.jointype);
	}

	/*
	 * initialize tuple type and projection info
	 */
	ExecAssignResultTypeFromTL(&mergestate->js.ps);
	ExecAssignProjectionInfo(&mergestate->js.ps, NULL);

	/*
	 * preprocess the merge clauses
	 */
	mergestate->mj_NumClauses = list_length(node->mergeclauses);
	mergestate->mj_Clauses = MJExamineQuals(node->mergeclauses,
			node->mergeFamilies,
			node->mergeCollations,
			node->mergeStrategies,
			node->mergeNullsFirst,
			(PlanState *) mergestate);

	/*
	 * initialize join state
	 */
	mergestate->mj_JoinState = EXEC_MJ_INITIALIZE_OUTER;
	mergestate->js.ps.ps_TupFromTlist = false;
	mergestate->mj_MatchedOuter = false;
	mergestate->mj_MatchedInner = false;
	mergestate->mj_OuterTupleSlot = NULL;
	mergestate->mj_InnerTupleSlot = NULL;

	/*
	 * initialization successful
	 */
	MJ1_printf("ExecInitMergeJoin: %s\n",
			"node initialized");

	/*
	 *
	 */
	mergestate->outernnBuffer = NIL;
	mergestate->mj_CurrInnerTupleSlot = ExecInitExtraTupleSlot(estate);
	ExecSetSlotDescriptor(mergestate->mj_CurrInnerTupleSlot, ExecGetResultType(innerPlanState(mergestate)));
	mergestate->mj_PrevInnerTupleSlot = ExecInitExtraTupleSlot(estate);
	ExecSetSlotDescriptor(mergestate->mj_PrevInnerTupleSlot, ExecGetResultType(innerPlanState(mergestate)));
	mergestate->mj_OuterTempTupleSlot = ExecInitExtraTupleSlot(estate);
	ExecSetSlotDescriptor(mergestate->mj_OuterTempTupleSlot, ExecGetResultType(outerPlanState(mergestate)));
	mergestate->mj_InnerTempTupleSlot = ExecInitExtraTupleSlot(estate);
	ExecSetSlotDescriptor(mergestate->mj_InnerTempTupleSlot, ExecGetResultType(innerPlanState(mergestate)));
	mergestate->mj_MarkedTempTupleSlot = ExecInitExtraTupleSlot(estate);
	ExecSetSlotDescriptor(mergestate->mj_MarkedTempTupleSlot, ExecGetResultType(innerPlanState(mergestate)));

		/* static positions */

	mergestate->positions = (List*)copyObject(node->positions);
	mergestate->sample_pos_outer = (List*)copyObject(node->sample_pos_outer);
	mergestate->sample_pos_inner = (List*)copyObject(node->sample_pos_inner);
	mergestate->group_pos_outer = (List*)copyObject(node->group_pos_outer);
	mergestate->group_pos_inner = (List*)copyObject(node->group_pos_inner);
	mergestate->time_pos_outer = node->time_pos_outer;
	mergestate->time_pos_inner = node->time_pos_inner;
	mergestate->aggCheck = node->aggCheck;
	mergestate->buffCheck = node->buffCheck;


	/* replace  */
	mergestate->replValues = palloc(mergestate->mj_InnerTempTupleSlot->tts_tupleDescriptor->natts * sizeof(Datum));
	mergestate->replIsnull = palloc0(mergestate->mj_InnerTempTupleSlot->tts_tupleDescriptor->natts * sizeof(bool));
	mergestate->doReplace = palloc0(mergestate->mj_InnerTempTupleSlot->tts_tupleDescriptor->natts * sizeof(bool));

/*	pprint(mergestate->positions);
	printf("OUTER\n");
	printf("POS G: ");
	pprint(node->group_pos_outer);
	printf("\n POS T %d\n", (node->time_pos_outer));
	printf("POS E %d\n", lfirst_int((node->sample_pos_outer->head)));

	printf("INNER\n");
	printf("POS G: ");
	pprint(node->group_pos_inner);
	printf("\nPOS T %d\n", (node->time_pos_inner));
	printf("POS E %d\n", lfirst_int((node->sample_pos_inner->head)));
	printf("POS Mh %d \n ", lfirst_int(node->positions->head));
	printf("POS Mt %d \n ", lfirst_int(node->positions->tail)); */


	MJReorderPositions(outerPlan(node)->targetlist, innerPlan(node)->targetlist, mergestate);



	return mergestate;
}

		/* ----------------------------------------------------------------
		 *		ExecEndMergeJoin
		 *
		 * old comments
		 *		frees storage allocated through C routines.
		 * ----------------------------------------------------------------
		 */
void ExecEndRNNJJoinWoBuff(RNNJoinState *node) {
	MJ1_printf("ExecEndMergeJoin: %s\n",
			"ending node processing");

	/*
	 * Free the exprcontext
	 */
	ExecFreeExprContext(&node->js.ps);

	/*
	 * clean out the tuple table
	 */
	ExecClearTuple(node->js.ps.ps_ResultTupleSlot);
	ExecClearTuple(node->mj_MarkedTupleSlot);

	/*
	 * shut down the subplans
	 */
	ExecEndNode(innerPlanState(node));
	ExecEndNode(outerPlanState(node));

	MJ1_printf("ExecEndMergeJoin: %s\n",
			"node processing ended");
}

void ExecReScanRNNJoinWoBuff(RNNJoinState *node) {
	ExecClearTuple(node->mj_MarkedTupleSlot);

	node->mj_JoinState = EXEC_MJ_INITIALIZE_OUTER;
	node->js.ps.ps_TupFromTlist = false;
	node->mj_MatchedOuter = false;
	node->mj_MatchedInner = false;
	node->mj_OuterTupleSlot = NULL;
	node->mj_InnerTupleSlot = NULL;
	node->mj_CurrInnerTupleSlot = ExecClearTuple(node->mj_CurrInnerTupleSlot);
	node->mj_PrevInnerTupleSlot = ExecClearTuple(node->mj_PrevInnerTupleSlot);
	node->mj_InnerTempTupleSlot = ExecClearTuple(node->mj_InnerTempTupleSlot);
	node->mj_OuterTempTupleSlot = ExecClearTuple(node->mj_OuterTempTupleSlot);
	node->mj_MarkedTempTupleSlot = ExecClearTuple(node->mj_MarkedTempTupleSlot);



	/*
	 * if chgParam of subnodes is not null then plans will be re-scanned by
	 * first ExecProcNode.
	 */

	if (node->js.ps.lefttree->chgParam == NULL)
		ExecReScan(node->js.ps.lefttree);
	if (node->js.ps.righttree->chgParam == NULL)
		ExecReScan(node->js.ps.righttree);

}


int RNNJWoBuffCompareGroupInnInn(RNNJoinState *node, TupleTableSlot *inner1,
		TupleTableSlot *inner) {
	ListCell *ele1, *ele2;
	int result = 0;
	bool nulleqnull = false;
	ExprContext *econtext = node->js.ps.ps_ExprContext;
	int i;
	MemoryContext oldContext;

	/*
	 * Call the comparison functions in short-lived context, in case they leak
	 * memory.
	 */ResetExprContext(econtext);

	oldContext = MemoryContextSwitchTo(econtext->ecxt_per_tuple_memory);

	i = 0;
	forboth(ele1, node->group_pos_inner, ele2, node->group_pos_inner) {
		MergeJoinClause clause = &node->mj_Clauses[i];
		Datum ldatum, rdatum;
		bool lisnull, risnull;

		ldatum = slot_getattr(inner1, lfirst_int(ele1), &lisnull);
		rdatum = slot_getattr(inner, lfirst_int(ele2), &risnull);

		/*
		 * Special case for NULL-vs-NULL, else use standard comparison.
		 */
		if (lisnull && risnull) {
			nulleqnull = true; /* NULL "=" NULL */
			continue;
		}

		result = ApplySortComparator(ldatum, lisnull, rdatum, risnull,
				&clause->ssup);

		if (result != 0)
			break;
		i++;
	}

	/*
	 * If we had any NULL-vs-NULL inputs, we do not want to report that the
	 * tuples are equal.  Instead, if result is still 0, change it to +1. This
	 * will result in advancing the inner side of the join.
	 *
	 * Likewise, if there was a constant-false joinqual, do not report
	 * equality.  We have to check this as part of the mergequals, else the
	 * rescan logic will do the wrong thing.
	 */
	if (result == 0 && (nulleqnull || node->mj_ConstFalseJoin))
		result = 1;

	MemoryContextSwitchTo(oldContext);

	return result;
}



int RNNJWoBuffCompareGroup(RNNJoinState *node, TupleTableSlot *outer,
		TupleTableSlot *inner) {
	ListCell *ele1, *ele2;
	int result = 0;
	bool nulleqnull = false;
	ExprContext *econtext = node->js.ps.ps_ExprContext;
	int i;
	MemoryContext oldContext;

	/*
	 * Call the comparison functions in short-lived context, in case they leak
	 * memory.
	 */ResetExprContext(econtext);

	oldContext = MemoryContextSwitchTo(econtext->ecxt_per_tuple_memory);

	i = 0;
	forboth(ele1, node->group_pos_outer, ele2, node->group_pos_inner) {
		MergeJoinClause clause = &node->mj_Clauses[i];
		Datum ldatum, rdatum;
		bool lisnull, risnull;

		ldatum = slot_getattr(outer, lfirst_int(ele1), &lisnull);
		rdatum = slot_getattr(inner, lfirst_int(ele2), &risnull);

		/*
		 * Special case for NULL-vs-NULL, else use standard comparison.
		 */
		if (lisnull && risnull) {
			nulleqnull = true; /* NULL "=" NULL */
			continue;
		}

		result = ApplySortComparator(ldatum, lisnull, rdatum, risnull,
				&clause->ssup);

		if (result != 0)
			break;
		i++;
	}

	/*
	 * If we had any NULL-vs-NULL inputs, we do not want to report that the
	 * tuples are equal.  Instead, if result is still 0, change it to +1. This
	 * will result in advancing the inner side of the join.
	 *
	 * Likewise, if there was a constant-false joinqual, do not report
	 * equality.  We have to check this as part of the mergequals, else the
	 * rescan logic will do the wrong thing.
	 */
	if (result == 0 && (nulleqnull || node->mj_ConstFalseJoin))
		result = 1;

	MemoryContextSwitchTo(oldContext);
	return result;
}

int RNNJWoBuffCompareSample(RNNJoinState *node, TupleTableSlot *outer,
	 TupleTableSlot *inner) {
	ListCell *ele1, *ele2;
	int result = 0;
	bool nulleqnull = false;
	ExprContext *econtext = node->js.ps.ps_ExprContext;
	int i;
	MemoryContext oldContext;

	/*
	 * Call the comparison functions in short-lived context, in case they leak
	 * memory.
	 */ResetExprContext(econtext);

	oldContext = MemoryContextSwitchTo(econtext->ecxt_per_tuple_memory);

	i = list_length(node->group_pos_outer) + 1;
	forboth(ele1, node->sample_pos_outer, ele2, node->sample_pos_inner) {
		MergeJoinClause clause = &node->mj_Clauses[i];
		Datum ldatum, rdatum;
		bool lisnull, risnull;

		ldatum = slot_getattr(outer, lfirst_int(ele1), &lisnull);
		rdatum = slot_getattr(inner, lfirst_int(ele2), &risnull);

		/*
		 * Special case for NULL-vs-NULL, else use standard comparison.
		 */
		if (lisnull && risnull) {
			nulleqnull = true; /* NULL "=" NULL */
			continue;
		}

		result = ApplySortComparator(ldatum, lisnull, rdatum, risnull,
				&clause->ssup);

		if (result != 0)
			break;
		i++;
	}

	/*
	 * If we had any NULL-vs-NULL inputs, we do not want to report that the
	 * tuples are equal.  Instead, if result is still 0, change it to +1. This
	 * will result in advancing the inner side of the join.
	 *
	 * Likewise, if there was a constant-false joinqual, do not report
	 * equality.  We have to check this as part of the mergequals, else the
	 * rescan logic will do the wrong thing.
	 */
	if (result == 0 && (nulleqnull || node->mj_ConstFalseJoin))
		result = 1;

	MemoryContextSwitchTo(oldContext);

	return result;
}

int RNNJWoBuffCompareTOuterInner(RNNJoinState *node, TupleTableSlot *outer,
		TupleTableSlot *inner) {
	int result = 0;
	bool nulleqnull = false;
	ExprContext *econtext = node->js.ps.ps_ExprContext;
	int i;
	MemoryContext oldContext;
	MergeJoinClause clause;
	Datum ldatum, rdatum;
	bool lisnull, risnull;

	/*
	 * Call the comparison functions in short-lived context, in case they leak
	 * memory.
	 */ResetExprContext(econtext);


	oldContext = MemoryContextSwitchTo(econtext->ecxt_per_tuple_memory);

	i = list_length(node->group_pos_outer);
	 	clause = &node->mj_Clauses[i];
	 	ldatum = slot_getattr(outer, node->time_pos_outer, &lisnull);
	 	rdatum = slot_getattr(inner, node->time_pos_inner, &risnull);


	/*
	 * Special case for NULL-vs-NULL, else use standard comparison.
	 */
	if (lisnull && risnull) {
		nulleqnull = true; /* NULL "=" NULL */
	}

	result = ApplySortComparator(ldatum, lisnull, rdatum, risnull,
			&clause->ssup);

	/*
	 * If we had any NULL-vs-NULL inputs, we do not want to report that the
	 * tuples are equal.  Instead, if result is still 0, change it to +1. This
	 * will result in advancing the inner side of the join.
	 *
	 * Likewise, if there was a constant-false joinqual, do not report
	 * equality.  We have to check this as part of the mergequals, else the
	 * rescan logic will do the wrong thing.
	 */
	if (result == 0 && (nulleqnull || node->mj_ConstFalseJoin))
		result = 1;

	MemoryContextSwitchTo(oldContext);

	return result;
}

int RNNJWoBuffCompareTInnerInner(RNNJoinState *node, TupleTableSlot *inner1,
		TupleTableSlot *inner2) {
	int result = 0;
	bool nulleqnull = false;
	ExprContext *econtext = node->js.ps.ps_ExprContext;
	int i;
	MemoryContext oldContext;
	MergeJoinClause clause;
	Datum ldatum, rdatum;
	bool lisnull, risnull;

	/*
	 * Call the comparison functions in short-lived context, in case they leak
	 * memory.
	 */ResetExprContext(econtext);

	oldContext = MemoryContextSwitchTo(econtext->ecxt_per_tuple_memory);

	i = list_length(node->group_pos_inner);
	clause = &node->mj_Clauses[i];

	ldatum = slot_getattr(inner1, node->time_pos_inner, &lisnull);
	rdatum = slot_getattr(inner2, node->time_pos_inner, &risnull);

	/*
	 * Special case for NULL-vs-NULL, else use standard comparison.
	 */
	if (lisnull && risnull) {
		nulleqnull = true; /* NULL "=" NULL */
	}

	result = ApplySortComparator(ldatum, lisnull, rdatum, risnull,
			&clause->ssup);

	/*
	 * If we had any NULL-vs-NULL inputs, we do not want to report that the
	 * tuples are equal.  Instead, if result is still 0, change it to +1. This
	 * will result in advancing the inner side of the join.
	 *
	 * Likewise, if there was a constant-false joinqual, do not report
	 * equality.  We have to check this as part of the mergequals, else the
	 * rescan logic will do the wrong thing.
	 */
	if (result == 0 && (nulleqnull || node->mj_ConstFalseJoin))
		result = 1;

	MemoryContextSwitchTo(oldContext);

	return result;
}

int RNNJWoBuffTimeDist(RNNJoinState *node, HeapTuple outer, TupleDesc outerDesc,
		TupleTableSlot *inner) {
	bool outer_is_null, inner_is_null;
	Datum	outerT = heap_getattr(outer, node->time_pos_outer, outerDesc, &outer_is_null);
	Datum innerT = slot_getattr(inner, node->time_pos_inner, &inner_is_null);


	return abs(DatumGetInt32(outerT) - DatumGetInt32(innerT));
}

int RNNJWoBuffCompareGroupBufInn(RNNJoinState *node, HeapTuple outer, TupleDesc outerDesc,
		TupleTableSlot *inner) {
	ListCell *ele1, *ele2;
	int result = 0;
	bool nulleqnull = false;
	ExprContext *econtext = node->js.ps.ps_ExprContext;
	int i;
	MemoryContext oldContext;

	/*
	 * Call the comparison functions in short-lived context, in case they leak
	 * memory.
	 */ResetExprContext(econtext);

	oldContext = MemoryContextSwitchTo(econtext->ecxt_per_tuple_memory);

	i = 0;
	forboth(ele1, node->group_pos_outer, ele2, node->group_pos_inner) {
		MergeJoinClause clause = &node->mj_Clauses[i];
		Datum ldatum, rdatum;
		bool lisnull, risnull;

		ldatum = heap_getattr(outer, lfirst_int(ele1), outerDesc, &lisnull);
		rdatum = slot_getattr(inner, lfirst_int(ele2), &risnull);

		/*
		 * Special case for NULL-vs-NULL, else use standard comparison.
		 */
		if (lisnull && risnull) {
			nulleqnull = true; /* NULL "=" NULL */
			continue;
		}

		result = ApplySortComparator(ldatum, lisnull, rdatum, risnull,
				&clause->ssup);

		if (result != 0)
			break;
		i++;
	}

	/*
	 * If we had any NULL-vs-NULL inputs, we do not want to report that the
	 * tuples are equal.  Instead, if result is still 0, change it to +1. This
	 * will result in advancing the inner side of the join.
	 *
	 * Likewise, if there was a constant-false joinqual, do not report
	 * equality.  We have to check this as part of the mergequals, else the
	 * rescan logic will do the wrong thing.
	 */
	if (result == 0 && (nulleqnull || node->mj_ConstFalseJoin))
		result = 1;

	MemoryContextSwitchTo(oldContext);

	return result;
}


int RNNJWoBuffTimeDistOutInn(RNNJoinState *node, TupleTableSlot *outer,
		TupleTableSlot *inner) {
	bool outer_is_null, inner_is_null;
	Datum	outerT = slot_getattr(outer, node->time_pos_outer, &outer_is_null);
	Datum innerT = slot_getattr(inner, node->time_pos_inner, &inner_is_null);

	return abs(DatumGetInt32(outerT) - DatumGetInt32(innerT));
}

static void MJReorderPositions(List *outer_tlist, List * inner_tlist, RNNJoinState *node) {
	int iClause;
	ListCell *cl;

	int new_group_pos_inner[list_length(node->group_pos_inner)];
	int new_group_pos_outer[list_length(node->group_pos_outer)];

	int new_sample_pos_inner[list_length(node->sample_pos_inner)]; /*positions of attribute sample in the outer relation*/
	int new_sample_pos_outer[list_length(node->sample_pos_outer)]; /*positions of attribute sample in the inner relation */

	int new_time_pos_inner;
	int new_time_pos_outer;


	int new_pos_M; /* position of attribute M in inner relation */
	int new_positions[node->positions->length];
	int j;
	for (j = 0; j<node->positions->length;j++){
		new_positions[j]=0;
	}




	iClause = 0;
	foreach(cl, outer_tlist) {
		TargetEntry *qual = (TargetEntry *) lfirst(cl);

		if (!IsA(qual, TargetEntry))
			elog(ERROR, "mergejoin tlist is not a TargetEntry");

		/*
		 * Prepare the input expressions for execution.
		 */
		Expr * leftExpr = (Expr *) qual->expr;
		Var * variab;
		ListCell *c;
		int i=0;
		variab = NULL;
		switch (nodeTag(leftExpr))
		{

		case T_Var:
			variab = ((Var *) leftExpr);
			i=0;
			foreach (c, node->group_pos_outer)
			{
				if(lfirst_int(c)==variab->varoattno)
					new_group_pos_outer[i]=variab->varattno;
				i++;
			}
			i=0;
			foreach (c, node->sample_pos_outer)
			{
				if(lfirst_int(c)==variab->varoattno)
					new_sample_pos_outer[i]=variab->varattno;
				i++;
			}
			if(node->time_pos_outer==variab->varoattno)
				new_time_pos_outer=variab->varattno;


			break;
		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(leftExpr));
			break;
		}
	}


	foreach(cl, inner_tlist) {
		TargetEntry *qual = (TargetEntry *) lfirst(cl);
		if (!IsA(qual, TargetEntry))
			elog(ERROR, "mergejoin tlist is not a TargetEntry");

		/*
		 * Prepare the input expressions for execution.
		 */

		Expr * rightExpr = (Expr *) qual->expr;
		Var * variab;
		ListCell *c;
		int i=0;
		variab = NULL;
		switch (nodeTag(rightExpr))
		{
		case T_RelabelType:
			variab =  ((Var *) ((RelabelType *)rightExpr)->arg);
		case T_Var:
			if(variab == NULL)
				variab = ((Var *) rightExpr);
			i=0;
			foreach (c, node->group_pos_inner)
			{
				if(lfirst_int(c)==variab->varoattno)
					new_group_pos_inner[i]=variab->varattno;
				i++;
			}
			i=0;
			foreach (c, node->sample_pos_inner)
			{
				if(lfirst_int(c)==variab->varoattno)
					new_sample_pos_inner[i]=variab->varattno;
				i++;
			}
			i=0;
			foreach (c,node->positions)
			{

				if(lfirst_int(c)==variab->varoattno)
					new_positions[i]=variab->varattno;
				i++;
			}


			if(node->time_pos_inner==variab->varoattno)
				new_time_pos_inner=variab->varattno;
			if(node->pos_M==variab->varoattno)
				new_pos_M=variab->varattno;
			break;

		default:
			elog(ERROR, "unrecognized node type: %d", (int) nodeTag(rightExpr));
			break;
		}

	}

	int i=0;
	ListCell *c;
	foreach (c, node->group_pos_outer)
	{
		lfirst_int(c)=new_group_pos_outer[i];
		i++;
	}
	i=0;
	foreach (c, node->group_pos_inner)
	{
		lfirst_int(c)=new_group_pos_inner[i];
		i++;
	}
	i=0;
	foreach (c, node->sample_pos_outer)
	{
		lfirst_int(c)=new_sample_pos_outer[i];
		i++;
	}
	i=0;
	foreach (c, node->sample_pos_inner)
	{
		lfirst_int(c)=new_sample_pos_inner[i];
		i++;
	}
	i=0;
	foreach (c,node->positions)
	{
		lfirst_int(c)=new_positions[i];
		i++;
	}


	node->time_pos_outer=new_time_pos_outer;
	node->time_pos_inner=new_time_pos_inner;
	node->pos_M=new_pos_M;

/*	printf("fOUTER\n");

	printf("POS G: ");
	pprint(node->group_pos_outer);

	printf("\nPOS T %d\n", (node->time_pos_outer));
	printf("POS E %d\n", lfirst_int(node->sample_pos_outer->head));

	printf("INNER\n");

	printf("POS G: ");
	pprint(node->group_pos_inner);
	printf("\nPOS T %d\n", (node->time_pos_inner));
	printf("POS E %d\n", lfirst_int((node->sample_pos_inner->head)));
	ListCell *val;
	foreach(val,node->positions) {
		int k = lfirst_int(val);
		printf(" position = %d  \n", k);
	}
 	 */

}



