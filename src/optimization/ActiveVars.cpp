#include "ActiveVars.hpp"

void ActiveVars::run()
{
    std::ofstream output_active_vars;
    output_active_vars.open("active_vars.json", std::ios::out);
    output_active_vars << "[";
    for (auto &func : this->m_->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        }
        else
        {
            func_ = func;  

            func_->set_instr_name();
            live_in.clear();
            live_out.clear();
            
            // 在此分析 func_ 的每个bb块的活跃变量，并存储在 live_in live_out 结构内
            std::map<BasicBlock *,std::set<Value *>> bb_use, bb_def;
            bb_use.clear();
            bb_def.clear();
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
                        // for(int op_num = 1; op_num < inst->get_num_operand(); op_num += 1)
                        // {
                        //     initop(op_num);
                        //     check
                        //     if(defnotfound && usenotfound)
                        //     {
                        //         insuse;
                        //     }
                        // }
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
                        // initop(0);
                        // check
                        // if(defnotfound)
                        // {
                        //     insdef;
                        // }
                    }
                }
                // output_active_vars << "\nbb_name is: " << bb->get_name() << std::endl;
                // output_active_vars<<"use vector: \n";
                // for(auto use_temp:use)
                // {
                //     output_active_vars<<"-->";
                //     output_active_vars << use_temp->get_name();
                //     output_active_vars<<"<--|";
                // }
                // output_active_vars<<"\ndef vector: \n";
                // for(auto def_temp:def)
                // {
                //     output_active_vars<<"-->";
                //     output_active_vars << def_temp->get_name();
                //     output_active_vars<<"<--|";
                // }
            }

            // 此时，对所有基本块bb的每条指令inst，都将引用和定值的变量写入了对应的变量中。
            // 接下来，将根据数据流方程和ppt中的算法，算出各基本块的in和out
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


            // 在此之上分析 func_ 的每个bb块的活跃变量，并存储在 live_in live_out 结构内

            output_active_vars << print();
            output_active_vars << ",";
        }
    }
    output_active_vars << "]";
    output_active_vars.close();
    return ;
}

std::string ActiveVars::print()
{
    std::string active_vars;
    active_vars +=  "{\n";
    active_vars +=  "\"function\": \"";
    active_vars +=  func_->get_name();
    active_vars +=  "\",\n";

    active_vars +=  "\"live_in\": {\n";
    for (auto &p : live_in) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]" ;
            active_vars += ",\n";   
        }
    }
    active_vars += "\n";
    active_vars +=  "    },\n";
    
    active_vars +=  "\"live_out\": {\n";
    for (auto &p : live_out) {
        if (p.second.size() == 0) {
            continue;
        } else {
            active_vars +=  "  \"";
            active_vars +=  p.first->get_name();
            active_vars +=  "\": [" ;
            for (auto &v : p.second) {
                active_vars +=  "\"%";
                active_vars +=  v->get_name();
                active_vars +=  "\",";
            }
            active_vars += "]";
            active_vars += ",\n";
        }
    }
    active_vars += "\n";
    active_vars += "    }\n";

    active_vars += "}\n";
    active_vars += "\n";
    return active_vars;
}