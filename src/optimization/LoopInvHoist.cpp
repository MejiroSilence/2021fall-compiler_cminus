#include <algorithm>
#include "logging.hpp"
#include "LoopSearch.hpp"
#include "LoopInvHoist.hpp"

#define LOOPSHOW false

void LoopInvHoist::run()
{
    // 先通过LoopSearch获取循环的相关信息
    LoopSearch loop_searcher(m_, LOOPSHOW);
    loop_searcher.run();

    // 接下来由你来补充啦！
    for (auto loop : loop_searcher)
    {
        std::set<Value *> modified;
        for (auto bb : *loop)
            for (auto ins : bb->get_instructions())
                if (isModified(ins))
                    modified.insert(ins);
        bool nextTry = true;
        while (nextTry)
        {
            nextTry = false;
            for (auto bb : *loop)
            {
                for (auto ins : bb->get_instructions())
                {
                    if (ins->is_call() || ins->is_phi())
                        continue;
                    if (isModified(ins))
                    {
                        if (ins->get_num_operand() == 2)
                        {
                            if (modified.count(ins->get_operand(0)) == 0 && modified.count(ins->get_operand(1)) == 0)
                            {
                                nextTry = move(ins, loop, bb, loop_searcher);
                                modified.erase(ins);
                            }
                        }
                        else if (ins->get_num_operand() == 1)
                        {
                            if (modified.count(ins->get_operand(0)) == 0)
                            {
                                nextTry = move(ins, loop, bb, loop_searcher);
                                modified.erase(ins);
                            }
                        }
                        if (nextTry)
                            break;
                    }
                }
                if (nextTry)
                    break;
            }
        }
        modified.clear();
    }
}
