/*
 * nodeRNNJoin.h
 *
 *  Created on: Aug 22, 2013
 *      Author: cafagna
 */

#ifndef NODERNNJOIN_H_
#define NODERNNJOIN_H_

#include "nodes/execnodes.h"

extern RNNJoinState *ExecInitRNNJoin(MergeJoin *node, EState *estate, int eflags);
extern TupleTableSlot *ExecRNNJoin(RNNJoinState *node);
extern void ExecEndRNNJoin(RNNJoinState *node);
extern void ExecReScanRNNJoin(RNNJoinState *node);

#endif /* NODERNNJEQJOIN_H_ */
