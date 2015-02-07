# ExpressionParser 
使用巴克诺范式(BNF)实现的简单数学表达式解析器。

## 特性
1. 简单轻量
2. 支持基本的+,-,*,/,%，括号等
3. 支持自定义常量
4. 支持自定义变量
5. 支持自定义函数


## 使用
```c
//自定义函数
double Multiply_Four(double value)
{
  return value * 4;
}

//基本表达式
double result;
char *expr = "1+3-((2*10)-5)";
result = Exec(expr);
if (!HasError()) printf("result=[%g]\n", result);

//增加自定义常量和变量
AddUserConstant("five", 5);
AddUserVar("var", 10);
expr = "1+3-((2*10)-5) +(five*var)";
printf("\n/*=======================================================\n");
printf("    Add Constant: five = 5\n");
printf("    Add Varable: var = 10\n");
printf("=======================================================*/\n");
result = Exec(expr);
if (!HasError()) printf("result=[%g]\n", result);
    
//重新设置自定义变量的值
SetValue("var", 15);
expr = "1+3-((2*10)-5) +(five*var)";
printf("\n/*=======================================================\n");
printf("    Set var = 15\n");
printf("=======================================================*/\n");
result = Exec(expr);
if (!HasError()) printf("result=[%g]\n", result);

//使用自带函数sqr(开方)
expr = "+1+3-((2*10)-5) +(five*var)-sqr(((1+1)*sqr(2)))";
printf("\n/*===============================================================\n");
printf("    Using sysFuncs : sqr\n");
printf("===============================================================*/\n");
result = Exec(expr);
if (!HasError()) printf("result=[%g]\n", result);

//使用自定义函数Multiply_four
RegisterUserFunction("Multiply_Four", Multiply_Four);
expr = "+1+3-((2*10)-5) +(five*var)-sqr(((1+1)*sqr(2)))+Multiply_Four(3)+PI*2.34+abs(-10.2)+int(-20)";
printf("\n/*===============================================================================================================\n");
printf("    Using sysConsts PI\n");
printf("    Using sysFuncs abs, int\n");
printf("    Registing Custom Function : Multiply_Four(x) \n");
printf("===============================================================================================================*/\n");
result = Exec(expr);
if (!HasError()) printf("result=[%g]\n", result);
```

关于详细信息：  
测试文件请参照`main.c`

## Bug汇报
如果你发现程序中有任何错误，请发送邮件给我：`fenghai_hhf@163.com`。

## 许可证
MIT许可证,详细请参见LICENSE文件

