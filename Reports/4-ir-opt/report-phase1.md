# Lab4 实验报告-阶段一

小组成员 姓名 学号  

队长姓名:应宇峰  

队长学号:PB19000260  

队员1姓名:孙若培  

队员1学号:PB19000244  

队员2姓名:梁奕涵  

队员2学号:PB19000066  

## 实验要求

请按照自己的理解，写明本次实验需要干什么

答：阅读并理解`LoopSearch`和`Mem2Reg`两个优化pass代码，理解它们具体的功能和作用，以及实现方式，以便在后续的实验中可以灵活掌握并开发优化Pass。最后完成思考题。

## 思考题

### LoopSearch

1. `LoopSearch`中直接用于描述一个循环的数据结构是什么？需要给出其具体类型。

    是`BBset_t`即` std::unordered_set<BasicBlock *>`BB的无序集合容器

    计算时使用`CFGNodePtrSet`即` std::unordered_set<CFGNode*>`存储循环，除了BB外还包含了cfg的信息,以及*Tarjan*s算法需要的信息

2. 循环入口是重要的信息，请指出`LoopSearch`中如何获取一个循环的入口？需要指出具体代码，并解释思路。

    通过函数`find_loop_base`

    先历遍循环里的BB，如果这个BB有一个不在循环内的前驱，即这个前驱可通过进入当前BB进入循环，那么当前BB就是循环的入口

    由于我们寻找内层循环的方式是通过删除外层循环的base以破坏强连通分量，那么在寻找内层循环的base时，base的前驱可能已经被删除，这时候需要历遍被删除的BB，如果被删除的某个BB有一个后继在循环中，即这个BB可以通过进入这个后继进入循环，显然这个后继就是循环的入口

    ```c++
    CFGNodePtr LoopSearch::find_loop_base(
        CFGNodePtrSet *set,//循环
        CFGNodePtrSet &reserved)//被删除的外层循环base结点
    {
    
        CFGNodePtr base = nullptr;//初始化
        for (auto n : *set)//遍历循环
        {
            for (auto prev : n->prevs)//遍历当前BB的前驱
            {
                if (set->find(prev) == set->end())//如果前驱不在这个循环中，即外部可通过当前BB进入循环，说明这是循环的入口
                {
                    base = n;
                }
            }
        }
        if (base != nullptr)//找到循环入口直接返回
            return base;
        for (auto res : reserved)//没找到循环入口，可能循环路口的前驱是已被删除的外层循环的入口
        {
            for (auto succ : res->succs)//历遍已被删除结点的后继
            {
                if (set->find(succ) != set->end())//后继在当前循环中,说明外部通过进入这个后继以进入循环，即后继是循环入口
                {
                    base = succ;
                }
            }
        }
    
        return base;
    }
    ```

    

3. 仅仅找出强连通分量并不能表达嵌套循环的结构。为了处理嵌套循环，`LoopSearch`在Tarjan algorithm的基础之上做了什么特殊处理？

    每次运行*tarjan*可得到一个外层循环，处理完外层循环后，我们删除外层的入口，破坏强连通分量，这时候外层循环不再存在，内层循环被暴露到最外层，再次运行*tarjan*即可达到内层循环。

    反复进行这个过程，直到*tarjan*找不到循环为止

    同样为了防止寻找内层循环的入口时其前驱已经被删除，还做了思考题2的处理

4. 某个基本块可以属于多层循环中，`LoopSearch`找出其所属的最内层循环的思路是什么？这里需要用到什么数据？这些数据在何时被维护？需要指出数据的引用与维护的代码，并简要分析。

    引用的代码

    ```c++
        BBset_t *get_inner_loop(BasicBlock* bb){
            if(bb2base.find(bb) == bb2base.end())//不存在bb到base的记录，即bb不在循环中
                return nullptr;//返回空
            return base2loop[bb2base[bb]];
        }
    ```

    需要bb所在的最内层循环的base信息，base所在的循环信息，每找到一个循环，维护`bb2base`和`base2loop`，`bb2base`记录bb所在循环的base,`base2loop`记录base对应的循环，因为先找到外层循环再找到内层循环，内层循环的记录会覆盖外层循环的记录，即记录的是最内层的信息，那么通过`base2loop[bb2base[bb]]`就能找到bb所在的最内层循环。

    维护的代码（都在循环`while (strongly_connected_components(nodes, sccs))`中）

    ```c++
                            loop_set.insert(bb_set);
                            func2loop[func].insert(bb_set);
                            base2loop.insert({base->bb, bb_set});//这里维护base2loop，记录base对应的循环
                            loop2base.insert({bb_set, base->bb});
    ```

    ```c++
                            // step 5: map each node to loop base
                            for (auto bb : *bb_set)//历遍循环内bb
                            {
                                if (bb2base.find(bb) == bb2base.end())//这里其实冗余了，没必要
                                    bb2base.insert({bb, base->bb});
                                else
                                    bb2base[bb] = base->bb;//维护bb2base,记录bb对应的base(内层循环覆盖外层循环)
                            }
    ```

    

### Mem2reg

1. 请简述概念：支配性、严格支配性、直接支配性、支配边界。  

答：
  - 支配性：若结点d支配结点n，则从流图起点开始每条到达结点n的路径都必须经过结点d，也即d是n的必经结点。据此可知，支配性类似于必要条件，若经过了n则一定经过了d。
  - 严格支配性：结点n的支配集中除了自身以外的结点都严格支配n
  - 直接支配性：从入口结点到n的所有路径上，结点n的最后一个非自身的支配结点，也即所有严格支配n的结点中，最靠近n的结点。
  - 支配边界：附件中对支配边界的定义如下：
    >(1)n支配m的一个前驱(q∈preds(m)且n∈Dom(q))  
    >(2)n并不严格支配m  
    >我们将相对于n具有这种性质的结点m的集合称为n的支配边界，记作DF(n)

    这个定义的意思大概是，支配边界就是结点所能支配到的结点的外侧一层，或者说是支配关系刚刚消失的位置(类似于数学中二维开区间的边界`partial`的概念)

2. phi节点是SSA的关键特征，请简述phi节点的概念，以及引入phi节点的理由。

答：phi结点简单来说就是一个选择函数，在有多个前驱的程序块起始处，通过程序实际走过的路径来选择变量的“版本”。

它为当前过程中使用或定义的名字插入一个phi函数，其中每一个前驱块都有一个参数与之对应。在运行的时候，phi函数会根据程序从哪个前驱结点执行而来来得到变量的值。


引进理由：当汇合点的多个前驱最后所用的变量版本不一致时，汇合点后的该变量就无法判定该使用哪个版本。此时需要在汇合点处插入一条phi函数用来选择该使用哪个版本的变量。如下例：
```
其中B4的前驱是B2和B3

B2:
y3 := y2 - 1
y4 := y3 - 2

B3: 
y3 := y2 +5

B4:
此时如果要引用y变量，到底该使用y4还是y3呢？由于不确定具体走了哪条分支，所以无法判断到底该引用y4还是y3。
```

3. 下面给出的cminus代码显然不是SSA的，后面是使用lab3的功能将其生成的LLVM IR（未加任何Pass），说明对一个变量的多次赋值变成了什么形式？

答：对一个变量的多次赋值，会先计算好等式右侧的值后，反复将其store入该变量的那个唯一地址中。尤其是等式右侧出现该变量时，则会先将该变量的值load入一个新变量中，然后再进行计算，最后再将计算的值store入原变量地址中。

4. 对下面给出的cminus程序，使用lab3的功能，分别关闭/开启Mem2Reg生成LLVM IR。对比生成的两段LLVM IR，开启Mem2Reg后，每条load, store指令发生了变化吗？变化或者没变化的原因是什么？请分类解释。

答：
  - 对全局变量和GEP指令产生的的赋值，存取值操作产生的load/store/alloca指令没有变化。而其他的load/store/alloca则全部删除，不再申请空间存值，改为SSA的格式。
  - 上述没变化的原因是代码中`if(!IS_GLOBAL_VARIABLE(l_val) && !IS_GEP_INSTR(l_val))`的判断忽略了全局变量和GEP。
  - 变化的原因是代码将所有申请空间存值的指令改成了静态单赋值也即SSA的形式。例如`func`函数中的`%op9 = phi i32 [ %arg0, %label_entry ], [ 0, %label6 ]`，该语句判断程序从哪个基本块执行而来，如果是从`%label_entry`执行而来，则使用`%arg0`变量的值，如果`从%label6`执行而来，则使用值`0`。而没有优化之前，`label_entry`块中把`%arg0`的值存储在`%op1`中，`label6`块中把`0`存储在`%op1`中，并在`label7`中读取`%op1`的值并返回。更改后的代码符合SSA的格式，二者最终的产生的效果一样，但SSA格式的代码省去了load/store指令，节约了时间。
5. 指出放置phi节点的代码，并解释是如何使用支配树的信息的。需要给出代码中的成员变量或成员函数名称。

答：放置phi节点的操作位于`Mem2Reg::generate_phi`函数的step2部分，内容如下
```C++
std::map<std::pair<BasicBlock *,Value *>, bool> bb_has_var_phi; // bb has phi for var
    for (auto var : global_live_var_name )
    {
        std::vector<BasicBlock *> work_list;
        work_list.assign(live_var_2blocks[var].begin(), live_var_2blocks[var].end());
        for (int i =0 ; i < work_list.size() ; i++ )
        {   
            auto bb = work_list[i];
            for ( auto bb_dominance_frontier_bb : dominators_->get_dominance_frontier(bb))
            {
                if ( bb_has_var_phi.find({bb_dominance_frontier_bb, var}) == bb_has_var_phi.end() )
                { 
                    // generate phi for bb_dominance_frontier_bb & add bb_dominance_frontier_bb to work list
                    auto phi = PhiInst::create_phi(var->get_type()->get_pointer_element_type(), bb_dominance_frontier_bb);
                    phi->set_lval(var);
                    bb_dominance_frontier_bb->add_instr_begin( phi );
                    work_list.push_back( bb_dominance_frontier_bb );
                    bb_has_var_phi[{bb_dominance_frontier_bb, var}] = true;
                }
            }
        }
    }
```
整体思路
- 遍历step1生成的`global_live_var_name`，即局部变量对应的内存单元，对于每个变量遍历`live_var_2blocks`列表，即所有对这个变量做写操作的基本块
- 对于上述每个基本块，通过支配树查询其支配边界(对应`dominators_->get_dominance_frontier`方法)，对于支配边界中的每个基本块，在它的头部放置一个关于当前变量`var`的phi节点(如果已经放过不再重复放置，已经放置了phi节点的基本块集合通过`bb_has_var_phi`记录)
- 加入`phi`语句后，当前处理的支配边界基本块也变成对`var`做了写操作，因此需要对当前块执行前两步，实现中采用广度优先策略递归查找支配边界，直到所有相关的基本块首部都添加了phi节点

具体流程
- 遍历`global_live_var_name`中的`var`，初始化`work_list`为`live_var_2blocks`
- 对于`work_list`中的每个bb,使用`dominators_->get_dominance_frontier`查找它的支配边界
- 遍历支配边界中的每个基本块`bb_dominance_frontier_bb`，判断其首部是否有对当前变量赋值的phi节点，若没有，插入新的phi节点
- 将`bb_dominance_frontier_bb`加入`work_list`(后续会检查它的支配边界)

### 代码阅读总结

通过对`LoopSearch`的阅读，理解了查找代码中的循环的方式，以及*Tarjan*算法，查找循环入口的算法

通过对`Mem2Reg`代码的阅读，理解并掌握了`Mem2Reg`这个优化Pass中关于生成插入phi函数，优化store/load指令等功能的具体的原理和实现方式。

### 实验反馈 （可选 不会评分）

希望助教能够在两个pass的代码实现中再多加一些注释，帮助理解。

### 组间交流 （可选）

未交流信息。
