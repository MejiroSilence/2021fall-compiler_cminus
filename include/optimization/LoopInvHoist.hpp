#pragma once

#include <unordered_map>
#include <unordered_set>
#include "PassManager.hpp"
#include "Module.h"
#include "Function.h"
#include "BasicBlock.h"

class LoopInvHoist : public Pass
{
public:
    LoopInvHoist(Module *m) : Pass(m) {}

    bool isModified(Instruction *ins)
    {
        return ins->isBinary() || ins->is_phi() || ins->is_fp2si() || ins->is_si2fp() || ins->is_cmp() || ins->is_fcmp() || ins->is_call() || ins->is_zext();
    }

    bool move(Instruction *ins, BBset_t *loop, BasicBlock *father, LoopSearch ls)
    {
        auto base = ls.get_loop_base(loop);
        BasicBlock *pre;
        for (auto bb : base->get_pre_basic_blocks())
            if (loop->count(bb) == 0)
            {
                pre = bb;
                break;
            }
        auto &preIns = pre->get_instructions();
        preIns.emplace(--preIns.end(), ins);
        father->get_instructions().remove(ins);
        return true;
    }

    void run() override;
};
