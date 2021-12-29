# Lab3 实验报告

队长姓名:应宇峰
队长学号:PB19000260
队员1姓名:孙若培
队员1学号:PB19000244
队员2姓名:梁奕涵
队员2学号:PB19000066


## 实验难点

对于API不熟悉不知道所需要的功能如何实现

不够细心，对于情况的讨论不完全，产生了许多bug

## 实验设计

请写明为了顺利完成本次实验，加入了哪些亮点设计，并对这些设计进行解释。
可能的阐述方向有:

1. 如何设计全局变量

2. 遇到的难点以及解决方案

3. 如何降低生成 IR 中的冗余

4. ...

设计了全局变量`Value *var_ptr,*expr;`其中expr用于存储临时产生的表达式（如访问num，val，additiveExpression等产生的临时结果），每产生一个可能用到的临时值就将其存入expr,便于调用，var_ptr存储产生的左值对应的地址，用于对该变量的赋值。

遇到的难点：不知道程序在那个地方崩溃了。 解决方案：通过使用`LOG`在每个visit开始处添加标记，来判断，如果不足以判断则在函数体中加入更多标记。默认的给分程序没有提供报错显示，需要手动魔改输出stderr或者单独对.cminus文件生成ll并执行来解决。

如何降低生成 IR 中的冗余：利用Value的get_use_list方法，获取所有使用该值的指令的链表。判断多次计算同一表达式的情况，进行处理。另外，判断循环体中的不变量，将其移出循环体先进行计算，以避免多次执行同一指令的情况。

对于BB的名字，我们原本使用了`trueBB` `falseBB`这样的名字，但这样生成的ll中，很多lable名字相同，然后我们发现将名字留空，会自动以序号生成label

对于函数的参数，我们在`ASTFunDeclaration`中就完成了所有的工作：包括将函数放入scope，以及参数的`store` `load`,所以在`ASTParam`中留空即可

`ASTProgram`直接依次访问其子节点

`ASTVarDeclaration`根据`scope.in_global()`判断是否为全局变量，然后对于两种情况再分别讨论是否为数组，加入scope即可

`ASTFunDeclaration`先构造函数类型，然后加入scope，（这里我另外把本该在`ASTParam`中做的将参数加入scopeload,store这些工作也做了，所以`ASTParam`什么都不需要做），然后访问其子节点`compound_stmt`即可，在访问完成后需要判断当前块是否ret，即当前函数是否写了`return`语句，如果没写，根据返回值类型自动生成`return`（顺带一提，这是我们de出的最后一个bug）

`ASTCompoundStmt`较为简单，进入一个新的`scope`然后分别访问其子节点即可

`ASTExpressionStmt`也是直接访问其子节点即可

`ASTSelectionStmt`需要先访问其`expression`子节点，获得表达式的值，然后将值与0进行比较，然后对if-else语句的2种情况分别进行实现即可，另外这里在跳出BB时，还需要判断当前BB是否已经有了ret或者br语句，没有才进行判断

`ASTAdditiveExpression`判断其`additive_expression`部分是否为空，为空则直接访问其`term`即可（表达式的值可以通过expr返回），不为空则依次访问其`additive_expression`以及`term`部分（顺序表示了结合性）然后再根据类型讨论一下是否需要类型转化即可

`ASTReturnStmt`需要判断返回值类型和函数是否匹配，和是否具有返回值(return;的情况)，并根据函数的返回值类型做相应的类型转换或返回void。函数最后没有返回语句的情况放在`ASTFunDeclaration`解决。

`ASTIterationStmt`先获取表达式的值，然后和0比较，以判断是否跳出循环。跳出循环后的BB若遇到无任何跳转语句和返回语句的情况时，交由`ASTFunDeclaration`判断无返回值处理。

### 实验总结

熟悉了debug方法

熟悉了编译原理的文法，语法结构树等知识

熟悉了C++语法

### 实验反馈 （可选 不会评分）

无。

### 组间交流 （可选）

无组间交流。
