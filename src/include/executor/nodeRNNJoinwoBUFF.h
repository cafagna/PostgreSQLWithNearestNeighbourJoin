/*
 * nodeRNNJoin.h
 *
 *  Created on: Aug 22, 2013
 *      Author: cafagna
 */

#ifndef NODERNNJOINWOBUFF_H_
#define NODERNNJOINWOBUFF_H_

#include "nodes/execnodes.h"

extern TupleTableSlot *ExecRNNJoinWoBuff(RNNJoinState *node);
extern void ExecEndRNNJJoinWoBuff(RNNJoinState *node);
extern void ExecReScanRNNJoinWoBuff(RNNJoinState *node);
extern RNNJoinState *ExecInitRNNJoinWoBuff(MergeJoin *node, EState *estate, int eflags);

#endif /* NODERNNJOINWOBUFF_H_ */
