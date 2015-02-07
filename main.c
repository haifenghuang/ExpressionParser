#include <stdio.h>
#include "ExpressionParser.h"
/*=======================================================
 *                      TEST
 *======================================================*/
double Multiply_Four(double value)
{
    return value * 4;
}

#ifdef TERMINAL
  #define COLOR_RED          "\033[0;31m"
  #define COLOR_LIGHT_YELLOW "\033[1;33m"
  #define COLOR_NORMAL       "\033[0m"

  #define Print_Expr(expr) \
      printf("    %sEXPR=[%s]%s\n", COLOR_RED, (expr), COLOR_NORMAL);

  #define Print_Result(result) \
      printf("%sresult=[%g]%s\n", COLOR_LIGHT_YELLOW, (result), COLOR_NORMAL);

#else
  #define Print_Expr(expr) \
      printf("    EXPR=[%s]\n", (expr));

  #define Print_Result(result) \
      printf("result=[%g]\n", (result));

#endif
int main(int argc, char *argv[])
{
    double result;
    /*
     if (argc != 2)
     {
     printf("please input math expression\n");
     return 0;
     }
     
     result = Exec(argv[1]);
     */
    
    char *expr = "1+3-((2*10)-5)";
    printf("/*=======================================================\n");
    printf("    Normal expression\n");
    Print_Expr(expr);
    printf("=======================================================*/\n");
    result = Exec(expr);
    if (!HasError())
        Print_Result(result);
    
    AddUserConstant("five", 5);
    AddUserVar("var", 10);
    expr = "1+3-((2*10)-5) +(five*var)";
    printf("\n/*=======================================================\n");
    printf("    Add Constant: five = 5\n");
    printf("    Add Varable: var = 10\n");
    Print_Expr(expr);
    printf("=======================================================*/\n");
    result = Exec(expr);
    if (!HasError())
        Print_Result(result);
    
    
    SetValue("var", 15);
    expr = "1+3-((2*10)-5) +(five*var)";
    printf("\n/*=======================================================\n");
    printf("    Set var = 15\n");
    Print_Expr(expr);
    printf("=======================================================*/\n");
    result = Exec(expr);
    if (!HasError())
        Print_Result(result);
    
    
    expr = "+1+3-((2*10)-5) +(five*var)-sqr(((1+1)*sqr(2)))";
    printf("\n/*===============================================================\n");
    printf("    Using sysFuncs : sqr\n");
    Print_Expr(expr);
    printf("===============================================================*/\n");
    result = Exec(expr);
    if (!HasError())
        Print_Result(result);
    
    RegisterUserFunction("Multiply_Four", Multiply_Four);
    expr = "+1+3-((2*10)-5) +(five*var)-sqr(((1+1)*sqr(2)))+Multiply_Four(3)+PI*2.34+abs(-10.2)+int(-20)";
    printf("\n/*===============================================================================================================\n");
    printf("    Using sysConsts PI\n");
    printf("    Using sysFuncs abs, int\n");
    printf("    Registing Custom Function : Multiply_Four(x) \n");
    Print_Expr(expr);
    printf("===============================================================================================================*/\n");
    result = Exec(expr);
    if (!HasError())
        Print_Result(result);
    
    return 0;
}

