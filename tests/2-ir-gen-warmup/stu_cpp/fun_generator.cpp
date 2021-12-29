#include "BasicBlock.h"
#include "Constant.h"
#include "Function.h"
#include "IRBuilder.h"
#include "Module.h"
#include "Type.h"

#include <iostream>
#include <memory>

#ifdef DEBUG  // 用于调试信息,大家可以在编译过程中通过" -DDEBUG"来开启这一选项
#define DEBUG_OUTPUT std::cout << __LINE__ << std::endl;  // 输出行号的简单示例
#else
#define DEBUG_OUTPUT
#endif

#define CONST_INT(num) \
    ConstantInt::get(num, module)

#define CONST_FP(num) \
    ConstantFP::get(num, module) // 得到常数值的表示,方便后面多次用到

int main() {
    auto module = new Module("fun");  // module name是什么无关紧要
    auto builder = new IRBuilder(nullptr, module);
    Type *Int32Type = Type::get_int32_type(module);
    auto *arrayType = ArrayType::get(Int32Type, 10);

    // callee
    std::vector<Type *> IntParam(1, Int32Type);
    auto calleeType = FunctionType::get(Int32Type, IntParam);
    auto calleeFun = Function::create(calleeType, "callee", module);
    auto bb = BasicBlock::create(module, "callee", calleeFun);
    builder->set_insert_point(bb);

    std::vector<Value *> args;  // fetch params
    for (auto arg = calleeFun->arg_begin(); arg != calleeFun->arg_end(); arg++) {
        args.push_back(*arg);   // * 号运算符是从迭代器中取出迭代器当前指向的元素
    }

    auto res = builder->create_imul(CONST_INT(2), args[0]);
    builder->create_ret(res);

    // main函数
    auto mainFun = Function::create(FunctionType::get(Int32Type, {}),
                                    "main", module);
    bb = BasicBlock::create(module, "entry", mainFun);
    // BasicBlock的名字在生成中无所谓,但是可以方便阅读
    builder->set_insert_point(bb);

    auto call = builder->create_call(calleeFun, {CONST_INT(110)});

    builder->create_ret(call);
    std::cout << module->print();
    delete module;
    return 0;
}