# Lab4 实验报告

小组成员 姓名 学号  

队长姓名:应宇峰  

队长学号:PB19000260  

队员1姓名:孙若培  

队员1学号:PB19000244  

队员2姓名:梁奕涵  

队员2学号:PB19000066  

## 实验要求

完成常量传播与死代码删除，循环不变式外提，活跃变量分析3个PASS

常量传播与死代码删除要求：将常量指令在编译期求值，用结果替代原来的常量操作。不考虑跨基本块的全局变量优化。

循环不变式外提要求：将与循环无关的表达式提取到循环的外面，以提高代码效率，避免冗余执行。不用考虑数组与全局变量。

活跃变量分析要求: 通过对每个bb块的每条指令进行分析，对每个bb块生成def和use两个集合。然后根据这些集合通过迭代算法计算出整个流图每个结点的IN和OUT。结果可供后续其他模块分析。

## 实验难点

常量传播与死代码删除：branch指令的修改会引发前驱后继关系变化，需要同步修改`use_list`，phi指令同理

循环不变式外提：判断指令是否可以无副作用的被提到循环外，将需要外提的指令提到base块的哪一个前驱

活跃变量分析: 将phi多存进去的变量剔除，应该增加一个条件--当前bb块==phi指令的参数，否则将从集合中删除。

另一个难点是对每种指令的分析，需要分析出什么指令需要单独判断，什么指令可以放在最后一并判断（根据操作数的多少，作用等），进而生成出正确的use和def集合。

## 实验设计

* 常量传播
    实现思路：

    依次检查整个IR中的所有指令，对于可以在编译期计算出值的指令（即所有操作数均为常数的指令），计算其值并用这个值来代替指令结果在代码中的所有引用，直到无法继续进行这种替换。

    相应代码：

    相关代码过长（500+行），这里仅展示代码框架，完整代码见ConstPropagation.cpp
    ```c++

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
                    for (auto item: const_insts)
                    {
                        auto inst = item.first;
                        auto val = item.second;
                        inst->replace_all_use_with(val);
                        inst->get_parent()->delete_instr(inst);
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
    ```

    以下所有代码结构相同，均为检查对应指令是否符合常量传播条件，若符合计算出对应的结果值，因此以`check_binary`为例，其余用函数定义代替
    
    注：`cast_constantfp`和`ConstFolder::compute`的浮点实现是我仿照助教给的整型的对应操作实现的
    ```c++
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

    pair<Instruction *, Value *> check_cmp(Instruction *inst);
    pair<Instruction *, Value *> check_fcmp(Instruction *inst);
    pair<Instruction *, Value *> check_gep(Instruction *inst);
    pair<Instruction *, Value *> check_branch(Instruction *inst);
    pair<Instruction *, Value *> check_load(Instruction *inst);
    pair<Instruction *, Value *> check_zext(Instruction *inst);
    pair<Instruction *, Value *> check_fp2si(Instruction *inst);
    pair<Instruction *, Value *> check_si2fp(Instruction *inst);
    pair<Instruction *, Value *> check_phi(Instruction *inst);
    ```

    优化前后的IR对比（举一个例子）并辅以简单说明：

    以ConstPropagation-testcase1为例
    ```C
    void main(void){
        int i;
        int idx;

        i = 0;
        idx = 0;

        while(i < 100000000)
        {
            idx = 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 ;
            i=i+idx*idx*idx*idx*idx*idx*idx*idx/(idx*idx*idx*idx*idx*idx*idx*idx);
        }
        output(idx*idx);
        return ;
    }
    ```
    
    优化前IR（`cminusfc testcase-1.cminus -mem2reg -emit-llvm`）
    ```llvm
    define void @main() {
    label_entry:
    br label %label2
    label2:                                                ; preds = %label_entry, %label7
    %op78 = phi i32 [ 0, %label_entry ], [ %op73, %label7 ]
    %op79 = phi i32 [ 0, %label_entry ], [ %op40, %label7 ]
    %op4 = icmp slt i32 %op78, 100000000
    %op5 = zext i1 %op4 to i32
    %op6 = icmp ne i32 %op5, 0
    br i1 %op6, label %label7, label %label74
    label7:                                                ; preds = %label2
    %op8 = add i32 1, 1
    %op9 = add i32 %op8, 1
    %op10 = add i32 %op9, 1
    %op11 = add i32 %op10, 1
    %op12 = add i32 %op11, 1
    %op13 = add i32 %op12, 1
    %op14 = add i32 %op13, 1
    %op15 = add i32 %op14, 1
    %op16 = add i32 %op15, 1
    %op17 = add i32 %op16, 1
    %op18 = add i32 %op17, 1
    %op19 = add i32 %op18, 1
    %op20 = add i32 %op19, 1
    %op21 = add i32 %op20, 1
    %op22 = add i32 %op21, 1
    %op23 = add i32 %op22, 1
    %op24 = add i32 %op23, 1
    %op25 = add i32 %op24, 1
    %op26 = add i32 %op25, 1
    %op27 = add i32 %op26, 1
    %op28 = add i32 %op27, 1
    %op29 = add i32 %op28, 1
    %op30 = add i32 %op29, 1
    %op31 = add i32 %op30, 1
    %op32 = add i32 %op31, 1
    %op33 = add i32 %op32, 1
    %op34 = add i32 %op33, 1
    %op35 = add i32 %op34, 1
    %op36 = add i32 %op35, 1
    %op37 = add i32 %op36, 1
    %op38 = add i32 %op37, 1
    %op39 = add i32 %op38, 1
    %op40 = add i32 %op39, 1
    %op44 = mul i32 %op40, %op40
    %op46 = mul i32 %op44, %op40
    %op48 = mul i32 %op46, %op40
    %op50 = mul i32 %op48, %op40
    %op52 = mul i32 %op50, %op40
    %op54 = mul i32 %op52, %op40
    %op56 = mul i32 %op54, %op40
    %op59 = mul i32 %op40, %op40
    %op61 = mul i32 %op59, %op40
    %op63 = mul i32 %op61, %op40
    %op65 = mul i32 %op63, %op40
    %op67 = mul i32 %op65, %op40
    %op69 = mul i32 %op67, %op40
    %op71 = mul i32 %op69, %op40
    %op72 = sdiv i32 %op56, %op71
    %op73 = add i32 %op78, %op72
    br label %label2
    label74:                                                ; preds = %label2
    %op77 = mul i32 %op79, %op79
    call void @output(i32 %op77)
    ret void
    }
    ```

    可以看到label7下基本都是常量加和乘

    优化后（`cminusfc testcase-1.cminus -mem2reg -const-propagation -emit-llvm`）
    ```llvm
    define void @main() {
    label_entry:
    br label %label2
    label2:                                                ; preds = %label_entry, %label7
    %op78 = phi i32 [ 0, %label_entry ], [ %op73, %label7 ]
    %op79 = phi i32 [ 0, %label_entry ], [ 34, %label7 ]
    %op4 = icmp slt i32 %op78, 100000000
    %op5 = zext i1 %op4 to i32
    %op6 = icmp ne i32 %op5, 0
    br i1 %op6, label %label7, label %label74
    label7:                                                ; preds = %label2
    %op73 = add i32 %op78, 1
    br label %label2
    label74:                                                ; preds = %label2
    %op77 = mul i32 %op79, %op79
    call void @output(i32 %op77)
    ret void
    }
    ```

    label7下的常量操作被替换成了一条简单的add，这样就提高了运行时效率。
    

* 循环不变式外提
    实现思路：
    
    对每一个循环，先历遍其中的每条指令，判断其是否修改了某个变量的值，对被修改的值进行标记。然后再历遍一次每条指令，对参数都在循环中未被修改过的Binary，fp2si，si2fp，cmp，fcmp，zext指令，将其提到base块的不在循环内的前驱中
    
    相应代码：
    
    ```c++
        bool isModified(Instruction *ins)//判断一个指令是否修改了某个变量
        {
            return ins->isBinary() || ins->is_phi() || ins->is_fp2si() || ins->is_si2fp() || ins->is_cmp() || ins->is_fcmp() || ins->is_call() || ins->is_zext();
        }
    
        bool move(Instruction *ins, BBset_t *loop, BasicBlock *father, LoopSearch ls)//将指令提到循环外
        {
            auto base = ls.get_loop_base(loop);//base块
            BasicBlock *pre;
            for (auto bb : base->get_pre_basic_blocks())
                if (loop->count(bb) == 0)//找到不在循环中的前驱
                {
                    pre = bb;
                    break;
                }
            auto &preIns = pre->get_instructions();//前驱的指令集（虽然返回值是引用，但这里仍需要声明为引用
            preIns.emplace(--preIns.end(), ins);//将指令插入到br/ret前
            father->get_instructions().remove(ins);//从原BB中删除指令
            return true;//for convenience
        }
    void LoopInvHoist::run()
    {
        // 先通过LoopSearch获取循环的相关信息
        LoopSearch loop_searcher(m_, LOOPSHOW);
        loop_searcher.run();
    
        // 接下来由你来补充啦！
        for (auto loop : loop_searcher)//对每一个循环
        {
            std::set<Value *> modified;//存贮被修改的变量
            for (auto bb : *loop)
                for (auto ins : bb->get_instructions())
                    if (isModified(ins))
                        modified.insert(ins);//记录每个被修改的变量
            bool nextTry = true;
            while (nextTry)
            {
                nextTry = false;
                for (auto bb : *loop)
                {
                    for (auto ins : bb->get_instructions())//历遍指令
                    {
                        if (ins->is_call() || ins->is_phi())//下方用是否修改了变量的值判断，call指令,phi指令虽然改变了变量，但不能外提
                            continue;
                        if (isModified(ins))//剩下Binary，fp2si，si2fp，cmp，fcmp，zext
                        {
                            if (ins->get_num_operand() == 2)//有2个参数
                            {
                                if (modified.count(ins->get_operand(0)) == 0 && modified.count(ins->get_operand(1)) == 0)//2个参数都在循环中被修改
                                {
                                    nextTry = move(ins, loop, bb, loop_searcher);//外提
                                    modified.erase(ins);//移除标记，让这个指令修改的变量涉及到的指令可以外提
                                }
                            }
                            else if (ins->get_num_operand() == 1)//有1个参数
                            {
                                if (modified.count(ins->get_operand(0)) == 0)//参数在循环中未被修改
                                {
                                    nextTry = move(ins, loop, bb, loop_searcher);//同上
                                    modified.erase(ins);
                                }
                            }
                            if (nextTry)//move中修改了当前bb的指令集(std::list),在历遍list的同时对其修改是危险的，必须break
                                break;
                        }
                    }
                    if (nextTry)//这里应该不break也无所谓，但这样外提出去的ll代码，顺序可能会和原来的有差别(但不影响执行)，还是留着吧,可能debug方便点
                        break;
                }
            }
            modified.clear();
        }
    }
    ```
    
    
    
    优化前后的IR对比（举一个例子）并辅以简单说明：
    
    对于
    
    ```c
    void main(void){
        int i;
        int j;
        int ret;
    
        i = 1;
        
        while(i<10000)
        {
            j = 0;
            while(j<10000)
            {
                ret = (i*i*i*i*i*i*i*i*i*i)/i/i/i/i/i/i/i/i/i/i;
                j=j+1;
            }
            i=i+1;
        }
    	output(ret);
        return ;
    }
    ```
    
    调用指令`cminusfc testcase-1.cminus -mem2reg -emit-llvm`查看未经过循环不变式外提的ll代码
    
    ```ll
    ; ModuleID = 'cminus'
    source_filename = "testcase-1.cminus"
    
    declare i32 @input()
    
    declare void @output(i32)
    
    declare void @outputFloat(float)
    
    declare void @neg_idx_except()
    
    define void @main() {
    label_entry:
      br label %label3
    label3:                                                ; preds = %label_entry, %label58
      %op61 = phi i32 [ %op64, %label58 ], [ undef, %label_entry ]
      %op62 = phi i32 [ 1, %label_entry ], [ %op60, %label58 ]
      %op63 = phi i32 [ %op65, %label58 ], [ undef, %label_entry ]
      %op5 = icmp slt i32 %op62, 10000
      %op6 = zext i1 %op5 to i32
      %op7 = icmp ne i32 %op6, 0
      br i1 %op7, label %label8, label %label9
    label8:                                                ; preds = %label3
      br label %label11
    label9:                                                ; preds = %label3
      call void @output(i32 %op61)
      ret void
    label11:                                                ; preds = %label8, %label16
      %op64 = phi i32 [ %op61, %label8 ], [ %op55, %label16 ]
      %op65 = phi i32 [ 0, %label8 ], [ %op57, %label16 ]
      %op13 = icmp slt i32 %op65, 10000
      %op14 = zext i1 %op13 to i32
      %op15 = icmp ne i32 %op14, 0
      br i1 %op15, label %label16, label %label58
    label16:                                                ; preds = %label11
      %op19 = mul i32 %op62, %op62
      %op21 = mul i32 %op19, %op62
      %op23 = mul i32 %op21, %op62
      %op25 = mul i32 %op23, %op62
      %op27 = mul i32 %op25, %op62
      %op29 = mul i32 %op27, %op62
      %op31 = mul i32 %op29, %op62
      %op33 = mul i32 %op31, %op62
      %op35 = mul i32 %op33, %op62
      %op37 = sdiv i32 %op35, %op62
      %op39 = sdiv i32 %op37, %op62
      %op41 = sdiv i32 %op39, %op62
      %op43 = sdiv i32 %op41, %op62
      %op45 = sdiv i32 %op43, %op62
      %op47 = sdiv i32 %op45, %op62
      %op49 = sdiv i32 %op47, %op62
      %op51 = sdiv i32 %op49, %op62
      %op53 = sdiv i32 %op51, %op62
      %op55 = sdiv i32 %op53, %op62
      %op57 = add i32 %op65, 1
      br label %label11
    label58:                                                ; preds = %label11
      %op60 = add i32 %op62, 1
      br label %label3
    }
    ```
    
    如同原来的c代码，全部计算都在2层循环内运行
    
    调用`cminusfc testcase-1.cminus -mem2reg -loop-inv-hoist -emit-llvm`获得优化后的代码
    
    ```ll
    ; ModuleID = 'cminus'
    source_filename = "testcase-1.cminus"
    
    declare i32 @input()
    
    declare void @output(i32)
    
    declare void @outputFloat(float)
    
    declare void @neg_idx_except()
    
    define void @main() {
    label_entry:
      br label %label3
    label3:                                                ; preds = %label_entry, %label58
      %op61 = phi i32 [ %op64, %label58 ], [ undef, %label_entry ]
      %op62 = phi i32 [ 1, %label_entry ], [ %op60, %label58 ]
      %op63 = phi i32 [ %op65, %label58 ], [ undef, %label_entry ]
      %op5 = icmp slt i32 %op62, 10000
      %op6 = zext i1 %op5 to i32
      %op7 = icmp ne i32 %op6, 0
      br i1 %op7, label %label8, label %label9
    label8:                                                ; preds = %label3
      %op19 = mul i32 %op62, %op62
      %op21 = mul i32 %op19, %op62
      %op23 = mul i32 %op21, %op62
      %op25 = mul i32 %op23, %op62
      %op27 = mul i32 %op25, %op62
      %op29 = mul i32 %op27, %op62
      %op31 = mul i32 %op29, %op62
      %op33 = mul i32 %op31, %op62
      %op35 = mul i32 %op33, %op62
      %op37 = sdiv i32 %op35, %op62
      %op39 = sdiv i32 %op37, %op62
      %op41 = sdiv i32 %op39, %op62
      %op43 = sdiv i32 %op41, %op62
      %op45 = sdiv i32 %op43, %op62
      %op47 = sdiv i32 %op45, %op62
      %op49 = sdiv i32 %op47, %op62
      %op51 = sdiv i32 %op49, %op62
      %op53 = sdiv i32 %op51, %op62
      %op55 = sdiv i32 %op53, %op62
      br label %label11
    label9:                                                ; preds = %label3
      call void @output(i32 %op61)
      ret void
    label11:                                                ; preds = %label8, %label16
      %op64 = phi i32 [ %op61, %label8 ], [ %op55, %label16 ]
      %op65 = phi i32 [ 0, %label8 ], [ %op57, %label16 ]
      %op13 = icmp slt i32 %op65, 10000
      %op14 = zext i1 %op13 to i32
      %op15 = icmp ne i32 %op14, 0
      br i1 %op15, label %label16, label %label58
    label16:                                                ; preds = %label11
      %op57 = add i32 %op65, 1
      br label %label11
    label58:                                                ; preds = %label11
      %op60 = add i32 %op62, 1
      br label %label3
    }
    ```
    
    计算的代码现在只在一层循环里运行了
    
    极大提高了运行效率
    
* 活跃变量分析
    实现思路：  
    1. 先将各种指令根据操作数分成几类，分析哪些操作数应该放到def集合，哪些操作数应该放到use集合中。然后我定义了几个宏，负责将对应的变量放入对应集合中（相同代码复用，增加可读性）。  
    
    ```
    OUT(BB) = 所有IN(SUCC_BB)汇聚一起（并集）
    IN(BB) = use + (OUT(BB) - def)
    ```
    2. 然后根据材料里给的伪代码写出迭代代码，其中需要先单独处理一下phi指令，把非本bb的其他路径的变量从集合中移出。
    然后就是实现伪代码的过程，计算出OUT(BB)后计算IN(BB)，迭代直到最后任何一个IN/OUT都不再更改为止。  

    3. 最后将所得的集合通过所给输出方法输出到json文件中。
    相应的代码：  
    第一部分

    ```cpp
    for(auto bb : func_->get_basic_blocks())
            {
                // for(auto inst: bb->get_instructions())
                //     output_active_vars << "--" << inst->get_num_operand() << "--!";

                for(auto inst : bb->get_instructions())
                {
                    // 分析每一种指令, 包括alloca, store, br, fptosi, call, phi等.
                    // 先填充use
                    // phi指令 先把两个操作数全都算进去，后面统一处理。由于操作数两个一组，所以每两个取1个
                    // output_active_vars << "bb name is: " << bb->get_name() << std::endl;
                    // output_active_vars << "(------" << inst->get_name()<< "   opname: " << inst->get_instr_op_name() << "------)" << std::endl;
                    if(inst->is_phi())
                    {
                        forop(0, 2)
                        {
                            initop(op_num);
                            check
                            if(defnotfound && usenotfound)
                            {
                                insuse;
                            }
                        }
                    }
                    else if(inst->is_call())
                    {
                        Value *op;
                        forop(1, 1)
                        {
                            op = inst->get_operand(op_num);
                            check
                            if(defnotfound && usenotfound)
                            {
                                insuse;
                            }
                        }
                    }
                    else if(inst->is_store())
                    {
                        initop(0);
                        check
                        if(defnotfound && usenotfound)
                        {
                            insuse;
                        }
                    }
                    else if(inst->is_br())
                    {
                        auto br_inst = dynamic_cast<BranchInst *>(inst);
                        if(br_inst->is_cond_br())
                        {
                            initop(0);
                            check
                            if(defnotfound && usenotfound)
                            {
                                insuse;
                            }
                        }
                    }
                    else if(inst->is_alloca())
                    {
                        ;//什么也不需要干
                    }
                    else if(inst->is_fp2si() || inst->is_si2fp() || inst->is_zext())
                    {
                        initop(0);
                        check
                        if(defnotfound && usenotfound)
                        {
                            insuse;
                        }
                    }
                    else
                    {
                        for(auto op: inst->get_operands())//该函数返回operands的一个vector
                        {
                            check
                            if(defnotfound && usenotfound)
                            {
                                insuse;
                            }
                        }
                    }
                    // 再填充def
                    if(inst->is_store())
                    {
                        initop(1);
                        if(defnotfound)
                        {
                            insdef;
                        }
                    }
                    else if(inst->is_call())
                    {
                        auto callinst = dynamic_cast<CallInst *>(inst);
                        if(!callinst->is_void())
                        {
                            if(defnotfound_(inst))
                            {
                                insdef_(inst);
                            } 
                        }
                    }
                    else if(inst->is_void() || inst->is_ret() || inst->is_br())
                    {
                        //不需要改动
                        ;
                    }
                    else
                    {
                        if(defnotfound_(inst))
                        {
                            insdef_(inst);
                        }
                    }
    ```
    第二部分

    ```cpp
    int in_has_changed = 1;
            while(in_has_changed == 1)//此处变量表示出现变化
            {
                in_has_changed = 0;
                for(auto bb: func_->get_basic_blocks())
                {
                    // 生成临时in out变量便于计算。
                    std::set<Value *> bb_in, bb_out, bb_succ_in;
                    // 对所有后继的IN(SUCC_BB)，根据数据流方程算出OUT(BB)
                    // 然后根据公式算出IN(BB) = use + (OUT(BB) - def)
                    for(auto succ_bb: bb->get_succ_basic_blocks())
                    {
                        // 先将上一次迭代的内容全部导入临时变量中
                        std::set<Value *> bb_succ_phi;
                        for(auto pre_in: live_in[succ_bb])
                        {
                            inssuc(pre_in);
                        }
                        for(auto inst: succ_bb->get_instructions())
                        {
                            if(inst->is_phi())
                            {
                                forop(0, 2)
                                {
                                    auto op_temp = inst->get_operand(op_num + 1);
                                    if(op_temp != bb)
                                    {
                                        initop(op_num);
                                        if(bb_succ_in.find(op) != bb_succ_in.end())
                                        {
                                            erasuc(op);
                                            if(bb_succ_phi.find(op) == bb_succ_phi.end())
                                            {
                                                insphi(op);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        for(auto inst: succ_bb->get_instructions())
                        {
                            if(inst->is_phi())
                            {
                                forop(0, 2)
                                {
                                    initop(op_num);
                                    if(phiisfound && inst->get_operand(op_num + 1) == bb && sucnotfound_(op))
                                    {
                                        inssuc(op);
                                    }
                                }
                            }
                            else if(inst->is_store())
                            {
                                initop(0);
                                if(phiisfound && sucnotfound_(op))
                                {
                                    inssuc(op);
                                }
                            }
                            else if(inst->is_alloca())
                            {
                                ;
                            }
                            else if(inst->is_br())
                            {  
                                auto br_inst = dynamic_cast<BranchInst *>(inst);
                                if(br_inst->is_cond_br())
                                {
                                    initop(0);
                                    if(phiisfound && sucnotfound_(op))
                                    {
                                        inssuc(op);
                                    }
                                }
                            }
                            else if(inst->is_fp2si() || inst->is_si2fp() || inst->is_zext())
                            {
                                initop(0);
                                if(phiisfound && sucnotfound_(op))
                                {
                                    inssuc(op);
                                }
                            }
                            else if(inst->is_call())
                            {
                                forop(1, 1)
                                {
                                    initop(op_num);
                                    if(phiisfound && sucnotfound_(op))
                                    {
                                        inssuc(op);
                                    }
                                }
                            }
                            else
                            {
                                for(auto op: inst->get_operands())
                                {
                                    if(phiisfound && sucnotfound_(op))
                                    {
                                        inssuc(op);
                                    }
                                }
                            }
                        }
                        for(auto succ_in: bb_succ_in)
                        {
                            if(bb_out.find(succ_in) == bb_out.end())
                                bb_out.insert(succ_in);
                            if(bb_in.find(succ_in) == bb_in.end())
                                bb_in.insert(succ_in);
                        }
                        bb_succ_in.clear();
                    }
                    // output_active_vars << std::endl;
                    // for(auto item:bb_out)
                    // {
                    //     output_active_vars<<"?";
                    //     output_active_vars <<item->get_name();
                    //     output_active_vars<<"?";
                    // }
                    for(auto def_in: def)
                    {
                        if(bb_in.find(def_in) != bb_in.end())
                        {
                            bb_in.erase(def_in);
                        }
                    }
                    for(auto use_in: use)
                    {
                        if(bb_in.find(use_in) == bb_in.end())
                        {
                            bb_in.insert(use_in);
                        }
                    }

                    if(live_in[bb].size() != bb_in.size())
                    {
                        in_has_changed = 1;
                    }
                    else
                    {
                        for(auto in_temp: bb_in)
                        {
                            if(live_in[bb].find(in_temp) == live_in[bb].end())
                            {
                                in_has_changed = 1;
                                break;
                            }
                        }
                    }
                    live_out[bb].clear();
                    live_in[bb].clear();
                    //output_active_vars << "------" << bb->get_name() << "------\n";
                    for(auto in: bb_in)
                    {
                        live_in[bb].insert(in);
                    }
                    for(auto out_temp: bb_out)
                    {
                        //output_active_vars << "-->" << out->get_name() << "<--\n";
                        live_out[bb].insert(out_temp);
                    }
                }
            }
    ```

### 实验总结

熟悉了C++

深入理解了各指令的结构及常量优化的思路

了解了循环不变式外提的思路以及实现

理解了计算活跃变量的思路以及实现，phi指令的作用，并且在过程中重新浏览了一边全部的指令，加深了对各指令的理解。

### 实验反馈 （可选 不会评分）

无

### 组间交流 （可选）

无
