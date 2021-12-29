#include "ConstPropagation.hpp"
#include "logging.hpp"
#include <vector>
#include <utility>
// debug use, delete this before handin
#include <assert.h>

using namespace std;


vector<pair<Instruction *, Value *>> get_const_instruction(Function *func);
pair<Instruction *, Value *> check_binary(Instruction *inst);
pair<Instruction *, Value *> check_cmp(Instruction *inst);
pair<Instruction *, Value *> check_fcmp(Instruction *inst);
pair<Instruction *, Value *> check_gep(Instruction *inst);
pair<Instruction *, Value *> check_branch(Instruction *inst);
pair<Instruction *, Value *> check_load(Instruction *inst);
pair<Instruction *, Value *> check_zext(Instruction *inst);
pair<Instruction *, Value *> check_fp2si(Instruction *inst);
pair<Instruction *, Value *> check_si2fp(Instruction *inst);
pair<Instruction *, Value *> check_phi(Instruction *inst);
vector<Instruction *> get_unused_instruction(Function *func);

// 给出了返回整形值的常数折叠实现，大家可以参考，在此基础上拓展
// 当然如果同学们有更好的方式，不强求使用下面这种方式
ConstantInt *ConstFolder::compute(
    Instruction::OpID op,
    ConstantInt *value1,
    ConstantInt *value2)
{

    int c_value1 = value1->get_value();
    int c_value2 = value2->get_value();
    switch (op)
    {
    case Instruction::add:
        return ConstantInt::get(c_value1 + c_value2, module_);
        break;
    case Instruction::sub:
        return ConstantInt::get(c_value1 - c_value2, module_);
        break;
    case Instruction::mul:
        return ConstantInt::get(c_value1 * c_value2, module_);
        break;
    case Instruction::sdiv:
        return ConstantInt::get((int)(c_value1 / c_value2), module_);
        break;
    default:
        return nullptr;
        break;
    }
}

ConstantFP *ConstFolder::compute(
    Instruction::OpID op,
    ConstantFP *value1,
    ConstantFP *value2)
{
    double c_value1 = value1->get_value();
    double c_value2 = value2->get_value();
    switch (op)
    {
    case Instruction::fadd:
        return ConstantFP::get(c_value1 + c_value2, module_);

        break;
    case Instruction::fsub:
        return ConstantFP::get(c_value1 - c_value2, module_);
        break;
    case Instruction::fmul:
        return ConstantFP::get(c_value1 * c_value2, module_);
        break;
    case Instruction::fdiv:
        // will result in UB if c_value2 == 0
        return ConstantFP::get(c_value1 / c_value2, module_);
        break;
    default:
        return nullptr;
        break;
    }
}

// 用来判断value是否为ConstantFP，如果不是则会返回nullptr
ConstantFP *cast_constantfp(Value *value)
{
    auto constant_fp_ptr = dynamic_cast<ConstantFP *>(value);
    if (constant_fp_ptr)
    {
        return constant_fp_ptr;
    }
    else
    {
        return nullptr;
    }
}
ConstantInt *cast_constantint(Value *value)
{
    auto constant_int_ptr = dynamic_cast<ConstantInt *>(value);
    if (constant_int_ptr)
    {
        return constant_int_ptr;
    }
    else
    {
        return nullptr;
    }
}

// 1. 遍历整个函数，找到所有操作数都是常数的指令
// 2. 找到这条指令结果的使用者
// 3. 把这个参数替换成常数
// 4. 删除原指令
// 5. loop
void ConstPropagation::run()
{
    // 从这里开始吧！
    auto func_list = m_->get_functions();
    for (auto func: func_list)
    {
        if (func->get_basic_blocks().size() == 0)
        {
            continue;
        }
        else
        {
            auto const_insts = get_const_instruction(func);
            while (const_insts.size() != 0)
            {
                LOG(DEBUG) << const_insts.size();
                for (auto item: const_insts)
                {
                    auto inst = item.first;
                    auto val = item.second;
                    LOG(DEBUG) << "inst: " << inst->print();
                    LOG(DEBUG) << "val: " << val->print();
                    inst->replace_all_use_with(val);
                    LOG(DEBUG) << "replace finish";
                    inst->get_parent()->delete_instr(inst);
                    LOG(DEBUG) << "delete finish";
                }
                const_insts = get_const_instruction(func);
            }

            // remove unused **PURE** function call
            auto unused_insts = get_unused_instruction(func);
            for (auto inst: unused_insts)
            {
                inst->get_parent()->delete_instr(inst);
            }
        }

    }
}

// used to remove function call with no actual effect
vector<Instruction *> get_unused_instruction(Function *func)
{
    vector<Instruction *>unused_insts;
    for (auto bb: func->get_basic_blocks())
    {
        // basic block will not be empty, it should have at least one ret or br
        for (auto inst: bb->get_instructions())
        {
            if (inst->is_call())
            {
                auto call = static_cast<CallInst *>(inst);
                // return value is not used
                if (!call->get_function_type()->get_return_type()->is_void_type() &&
                    call->get_use_list().size() == 0)
                {
                    bool flag = true;
                    for (auto bb: static_cast<Function *>(call->get_operand(0))->get_basic_blocks())
                    {
                        for (auto inst: bb->get_instructions())
                        {
                            // no global variable operation
                            if (inst->is_store()) {
                                flag = false;

                            }
                        }
                    }
                    if (flag) unused_insts.push_back(inst);
                }
            }
        }
    }
    return unused_insts;
}

vector<pair<Instruction *, Value *>> get_const_instruction(Function *func)
{
    ConstFolder folder = ConstFolder(func->get_parent());
    vector<pair<Instruction *, Value *>> const_insts;
    for (auto bb: func->get_basic_blocks())
    {
        for (auto inst: bb->get_instructions())
        {
            pair<Instruction *, Value *> pair = make_pair(nullptr, nullptr);
            if (inst->isBinary())
            {
                pair = check_binary(inst);
            }
            else if (inst->is_cmp())
            {
                pair = check_cmp(inst);
            }
            else if (inst->is_fcmp())
            {
                pair = check_fcmp(inst);
            }
            else if (inst->is_gep())
            {
                // TODO:
                pair = check_gep(inst);
            }
            else if (inst->is_br())
            {
                pair = check_branch(inst);
            }
            else if (inst->is_load())
            {
                pair = check_load(inst);
            }
            else if (inst->is_zext())
            {
                pair = check_zext(inst);
            }
            else if (inst->is_fp2si())
            {
                pair = check_fp2si(inst);
            }
            else if (inst->is_si2fp())
            {
                pair = check_si2fp(inst);
            }
            else if (inst->is_phi())
            {
                pair = check_phi(inst);
            }
            if (pair.first != nullptr) const_insts.push_back(pair);
        }
    }
    return const_insts;
}

pair<Instruction *, Value *> check_binary(Instruction *inst)
{
    auto folder = ConstFolder(inst->get_module());
    // isBinary检查了操作数个数
    auto left = inst->get_operand(0);
    auto right = inst->get_operand(1);
    if (cast_constantint(left) != nullptr && cast_constantint(right) != nullptr)
    {
        auto val = folder.compute(inst->get_instr_type(),
            cast_constantint(left),
            cast_constantint(right)
        );
        return make_pair(inst, static_cast<Value *>(val));
    }
    else if (cast_constantfp(left) != nullptr && cast_constantfp(right) != nullptr)
    {
        auto val = folder.compute(inst->get_instr_type(),
            cast_constantfp(left),
            cast_constantfp(right)
        );
        return make_pair(inst, static_cast<Value *>(val));
    }
    return make_pair(nullptr, nullptr);
}

pair<Instruction *, Value *> check_cmp(Instruction *inst)
{
    auto mod = inst->get_module();
    auto folder = ConstFolder(mod);
    auto op = static_cast<CmpInst *>(inst)->get_cmp_op();
    auto lhs = cast_constantint(inst->get_operand(0));
    auto rhs = cast_constantint(inst->get_operand(1));
    if (lhs != nullptr && rhs != nullptr)
    {
        Value *flag = nullptr;
        switch (op)
        {
            case CmpInst::EQ:
                flag = static_cast<Value *>(ConstantInt::get(
                    lhs->get_value() == rhs->get_value(), mod
                ));
                break;
            case CmpInst::NE:
                flag = static_cast<Value *>(ConstantInt::get(
                    lhs->get_value() != rhs->get_value(), mod
                ));
                break;
            case CmpInst::GE:
                flag = static_cast<Value *>(ConstantInt::get(
                    lhs->get_value() >= rhs->get_value(), mod
                ));
                break;
            case CmpInst::GT:
                flag = static_cast<Value *>(ConstantInt::get(
                    lhs->get_value() > rhs->get_value(), mod
                ));
                break;
            case CmpInst::LE:
                flag = static_cast<Value *>(ConstantInt::get(
                    lhs->get_value() <= rhs->get_value(), mod
                ));
                break;
            case CmpInst::LT:
                flag = static_cast<Value *>(ConstantInt::get(
                    lhs->get_value() < rhs->get_value(), mod
                ));
                break;
            default:
                LOG(DEBUG) << "unexpected CmpOp";
        }
        return make_pair(inst, flag);
    }
    return make_pair(nullptr, nullptr);
}

pair<Instruction *, Value *> check_fcmp(Instruction *inst)
{
    auto mod = inst->get_module();
    auto folder = ConstFolder(mod);
    auto op = static_cast<FCmpInst *>(inst)->get_cmp_op();
    auto lhs = cast_constantfp(inst->get_operand(0));
    auto rhs = cast_constantfp(inst->get_operand(1));
    if (lhs != nullptr && rhs != nullptr)
    {
        Value *flag = nullptr;
        switch (op)
        {
            case FCmpInst::EQ:
                flag = static_cast<Value *>(ConstantFP::get(
                    lhs->get_value() == rhs->get_value(), mod
                ));
                break;
            case FCmpInst::NE:
                flag = static_cast<Value *>(ConstantFP::get(
                    lhs->get_value() != rhs->get_value(), mod
                ));
                break;
            case FCmpInst::GE:
                flag = static_cast<Value *>(ConstantFP::get(
                    lhs->get_value() >= rhs->get_value(), mod
                ));
                break;
            case FCmpInst::GT:
                flag = static_cast<Value *>(ConstantFP::get(
                    lhs->get_value() > rhs->get_value(), mod
                ));
                break;
            case FCmpInst::LE:
                flag = static_cast<Value *>(ConstantFP::get(
                    lhs->get_value() <= rhs->get_value(), mod
                ));
                break;
            case FCmpInst::LT:
                flag = static_cast<Value *>(ConstantFP::get(
                    lhs->get_value() < rhs->get_value(), mod
                ));
                break;
            default:
                LOG(DEBUG) << "unexpected FCmpOp";
        }
        return make_pair(inst, flag);
    }
    return make_pair(nullptr, nullptr);
}

pair<Instruction *, Value *> check_gep(Instruction *inst)
{
    // I think probably no one will use const as address......
    return make_pair(nullptr, nullptr);
}

pair<Instruction *, Value *> check_branch(Instruction *inst)
{
    auto mod = inst->get_module();
    auto bb = inst->get_parent();
    auto br = dynamic_cast<BranchInst *>(inst);
    if (br->is_cond_br())
    {
        auto cond = cast_constantint(br->get_operand(0));
        if (cond != nullptr)
        {
            auto if_true = dynamic_cast<BasicBlock *>(br->get_operand(1));
            auto if_false = dynamic_cast<BasicBlock *>(br->get_operand(2));
            auto label = cond->get_value() ? if_true : if_false;
            auto uncond_br = BranchInst::create_br(label, bb);
            // TODO: why delete and add uncond_br make things all work?
            bb->delete_instr(uncond_br);
            bb->delete_instr(inst);
            bb->add_instruction(uncond_br);
            // branch is definitely the last instruction of the block
        }
    }
    
    // we won't delete branch instruction
    return make_pair(nullptr, nullptr);
}

pair<Instruction *, Value *> check_load(Instruction *inst)
{
    auto load = dynamic_cast<LoadInst *>(inst);
    auto mod = inst->get_module();
    auto folder = ConstFolder(mod);
    auto bb = inst->get_parent();
    auto addr = load->get_lval();
    StoreInst *store = nullptr;
    for (auto inst_before: bb->get_instructions())
    {
        if (inst_before == inst) break;
        if (inst_before->is_store() && inst_before->get_operand(1) == addr)
        {
            store = dynamic_cast<StoreInst *>(inst_before);
        }
    }
    if (store != nullptr)
    {
        if (cast_constantint(store->get_rval()) != nullptr)
        {
            return make_pair(inst, ConstantInt::get(cast_constantint(store->get_rval())->get_value(), mod));
        }
        if (cast_constantfp(store->get_rval()) != nullptr)
        {
            return make_pair(inst, ConstantFP::get(cast_constantfp(store->get_rval())->get_value(), mod));
        }
    }
    return make_pair(nullptr, nullptr);
}

pair<Instruction *, Value *> check_zext(Instruction *inst)
{
    auto mod = inst->get_module();
    auto folder = ConstFolder(mod);
    auto val = cast_constantint(inst->get_operand(0));
    if (val != nullptr)
    {
        return make_pair(inst, static_cast<Value *>(val));
    }
    return make_pair(nullptr, nullptr);
}

pair<Instruction *, Value *> check_fp2si(Instruction *inst)
{
    auto mod = inst->get_module();
    auto folder = ConstFolder(mod);
    auto val = cast_constantfp(inst->get_operand(0));
    if (val != nullptr)
    {
        return make_pair(
            inst, 
            static_cast<Value *>(ConstantInt::get((int)val->get_value(), mod))
        );
    }
    return make_pair(nullptr, nullptr);
}

pair<Instruction *, Value *> check_si2fp(Instruction *inst)
{
    auto mod = inst->get_module();
    auto folder = ConstFolder(mod);
    auto val = cast_constantint(inst->get_operand(0));
    if (val != nullptr)
    {
        return make_pair(
            inst, 
            static_cast<Value *>(ConstantFP::get((float)val->get_value(), mod))
        );
    }
    return make_pair(nullptr, nullptr);
}

pair<Instruction *, Value *> check_phi(Instruction *inst)
{
    // only if all of the val is same and res type is one of (i1, i32, float)
    // should we do this propogation
    // TODO: This optimization could cause pre succ relation change
    // FIXME: neg optimization
    auto mod = inst->get_module();
    auto phi = dynamic_cast<PhiInst *>(inst);
    auto type = phi->get_lval()->get_type();
    auto n = inst->get_num_operand();

    if (type->is_integer_type())
    {
        auto val = cast_constantint(inst->get_operand(0));
        if (val == nullptr) return make_pair(nullptr, nullptr);
        for (int i = 0; i < n; i += 2)
        {
            auto next = cast_constantint(inst->get_operand(i));
            if (!next || val->get_value() != next->get_value())
                return make_pair(nullptr, nullptr);
        }
        return make_pair(inst, static_cast<Value *>(val));

    }
    else if (type->is_float_type())
    {
        auto val = cast_constantfp(inst->get_operand(0));
        if (val == nullptr) return make_pair(nullptr, nullptr);
        for (int i = 0; i < n; i += 2)
        {
            auto next = cast_constantfp(inst->get_operand(i));
            if (!next || val->get_value() != next->get_value())
                return make_pair(nullptr, nullptr);
        }
        return make_pair(inst, static_cast<Value *>(val));
    }
    
    return make_pair(nullptr, nullptr);
}