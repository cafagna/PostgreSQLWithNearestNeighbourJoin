/*
 * nodeRNNJoin.h
 *
 *  Created on: Aug 22, 2013
 *      Author: cafagna
 */

#ifndef NODERNNJOINWOAGG_H_
#define NODERNNJOINWOAGG_H_

#include "nodes/execnodes.h"

extern RNNJoinState *ExecInitRNNJoinWoAgg(MergeJoin *node, EState *estate, int eflags);
extern TupleTableSlot *ExecRNNJoinWoAgg(RNNJoinState *node);
extern void ExecEndRNNJoinWoAgg(RNNJoinState *node);
extern void ExecReScanRNNJoinWoAgg(RNNJoinState *node);

#endif /* NODERNNJOIN_H_ */
