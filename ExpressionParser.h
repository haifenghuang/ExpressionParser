#ifndef __EXPRESSIONPARSER_H__
#define __EXPRESSIONPARSER_H__

#define MAX_FORMULA_LEN  1024  /* max formula length */
#define MAX_CONSTS       256   /* max constants allowed */
#define MAX_VARIABLE     256   /* max variable allowed */
#define MAX_NAME         256   /* max name length */
#define MAX_FUNCS        256   /* max functions */

/* 自定义用户回调函数 */
typedef double (*UserFunction)(double value);

//增加一个自定义变量
int AddUserVar(const char *name, double value);

//设置某个变量的值
void SetValue(const char *name, double value);

//得到某个变量的值
double GetValue(const char *name);

//将某个变量的值加上某个固定的值
void IncValue(const char *name, double value);

//将某个变量的值减去某个固定的值
void DecValue(const char *name, double value);

//增加用户自定义常量
int AddUserConstant(const char *name, double value);

//注册自定义函数
void RegisterUserFunction(char *funcName, UserFunction proc);

//计算表达式
double Exec(const char *formula);

//表达式处理成功与否
int HasError();

#endif
