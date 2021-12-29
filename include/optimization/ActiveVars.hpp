#ifndef ACTIVEVARS_HPP
#define ACTIVEVARS_HPP
#define use bb_use[bb]
#define def bb_def[bb]

#define use_(bbname) bb_use[bbname]
#define def_(bbname) bb_def[bbname]

#define insuse bb_use[bb].insert(op)
#define insdef bb_def[bb].insert(op)
#define insuse_(inst) bb_use[bb].insert(inst)
#define insdef_(inst) bb_def[bb].insert(inst)

#define initop(op_num) auto op = inst->get_operand(op_num)

#define defnotfound def.find(op) == def.end()
#define usenotfound use.find(op) == use.end()
#define defnotfound_(inst) def.find(inst) == def.end()
#define usenotfound_(inst) use.find(inst) == use.end()
#define sucnotfound_(inst) bb_succ_in.find(inst) == bb_succ_in.end()
#define phiisfound bb_succ_phi.find(op) != bb_succ_phi.end()

#define forop(i, j) for(int op_num = i; op_num < inst->get_num_operand(); op_num += j)


#define inssuc(in) bb_succ_in.insert(in)
#define erasuc(in) bb_succ_in.erase(in)
#define insphi(in) bb_succ_phi.insert(in)
#define eraphi(in) bb_succ_phi.erase(in)

#define check if(!(dynamic_cast<ConstantFP *>(op)||dynamic_cast<ConstantInt *>(op)))

#include "PassManager.hpp"
#include "Constant.h"
#include "Instruction.h"
#include "Module.h"

#include "Value.h"
#include "IRBuilder.h"
#include <vector>
#include <stack>
#include <unordered_map>
#include <map>
#include <queue>
#include <fstream>

class ActiveVars : public Pass
{
public:
    ActiveVars(Module *m) : Pass(m) {}
    void run();
    std::string print();
private:
    Function *func_;
    std::map<BasicBlock *, std::set<Value *>> live_in, live_out;
};

#endif
