#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>
#include "ExPressionParser.h"
/*
{ BNF(巴克诺范式):
  Expression::=<Term> [<AddOp> <Term>]*
  <Term>    ::=<Factor> [<MulOp> <Factor> ]*
  <Factor>  ::=(<Expression>) | <Numeric> | <Constant> | UserFunc | PreDefinedFunc
}
{*------------------------------------------------------------------------------
  一个简单的数学表达式控件(由原来的Delphi语言更改成C语言)
  @author  huanghaifeng
  @version 2006/03/03 1.0 Initial revision.
  @todo
  @comment 这个类是从RxLib单元的Parsing中更改而来的，感谢他们所做出的贡献，
           同时我要感谢Let's Build a Compiler的作者Jack W. Crenshaw
-------------------------------------------------------------------------------}
 */
#define PI         3.14159
#define E          2.71828

#define EPSILON    1.19e-7f

#define IsAddOp(c)    ((c) == '+' || (c) == '-')
#define IsMulOp(c)    ((c) == '*' || (c) == '/' || (c) == '^')
#define IsNumeric(c)  (isdigit((c)) || (c) == '.')
#define IsLetter(c)   (isalpha((c)) || (c) == '_')

#define MAX_FORMULA_LEN  1024  /* max formula length */
#define MAX_CONSTS       256   /* max constants allowed */
#define MAX_VARIABLE     256   /* max variable allowed */
#define MAX_NAME         256   /* max name length */
#define MAX_FUNCS        256   /* max functions */

typedef struct _VarTable ConstTable;
typedef struct _VarTable VarTable;

/* 变量及其常量定义结构 */
struct _VarTable
{
  char name[MAX_NAME];
  double value;
};

/* 自带的函数 */
typedef enum TParserFunc
{
  pfArcTan, pfCos, pfSin, pfTan, pfAbs, pfExp, pfLn, pfLog,
  pfSqrt, pfSqr, pfInt, pfRound, pfFloor, pfCeiling, 
  pfArcSin, pfArcCos, pfSign, pfFact
}TParserFunc;

typedef struct FuncTable
{
  TParserFunc func;
  const char *name;
}FuncTable;

/* 自定义用户回调函数 */
typedef struct UserFuncList
{
  char name[MAX_NAME];
  UserFunction func;
}UserFuncList;

FuncTable funcTables[] =
{
  {pfArcTan,  "ARCTAN"},
  {pfCos,     "COS"},
  {pfSin,     "SIN"},
  {pfTan,     "TAN"},
  {pfAbs,     "ABS"},
  {pfExp,     "EXP"},
  {pfLn,      "LN"},
  {pfLog,     "LOG"},
  {pfSqrt,    "SQRT"},
  {pfSqr,     "SQR"},
  {pfInt,     "INT"},
  {pfRound,   "ROUND"},
  {pfFloor,   "FLOOR"},
  {pfCeiling, "CEILING"},
  {pfArcSin,  "ARCSIN"},
  {pfArcCos,  "ARCCOS"},
  {pfSign,    "SIGN"},
};

#define FuncTableSize sizeof(funcTables) /sizeof(funcTables[0])

/* TODO: 下面的三个数组可以用动态创建的方式，实现应该比较简单 */
static ConstTable constTable[MAX_CONSTS];
static int nConsts;

static UserFuncList userFuncList[MAX_FUNCS];
static int nUserFuncs;

static VarTable varTable[MAX_VARIABLE];
static int nVars;

static bool hasError; /* 解析时是否有错误 */
static int iCurPos; /* 当前处理的指针位置 */
static char FParseText[MAX_FORMULA_LEN];

static double Expression();

static char* SubString (const char* input, int offset, int len, char* dest)
{
  int input_len = (int)strlen (input);

  if (offset + len > input_len)
  {
     return NULL;
  }

  strncpy (dest, input + offset, len);
  return dest;
}

static void StrTrim(char *str, char *out)
{
  int i, j = 0;
  int end = (int)strlen(str);

  for (i = 0; i < end; i++)
    {
      if (str[i] != ' ' && str[i] != '\t')
          out[j++] = str[i];
    }

  out[j] = '\0'; // Null terminate string.
}

/* stolen from
   https://code.google.com/p/1927code/source/browse/strrep.c
*/
/* ---------------------------------------------------------------------------
  Name       : replace - Search & replace a substring by another one. 
  Creation   : Thierry Husson, Sept 2010
  Parameters :
      str    : Big string where we search
      oldstr : Substring we are looking for
      newstr : Substring we want to replace with
      count  : Optional pointer to int (input / output value). NULL to ignore.  
               Input:  Maximum replacements to be done. NULL or < 1 to do all.
               Output: Number of replacements done or -1 if not enough memory.
  Returns    : Pointer to the new string or NULL if error.
  Notes      : 
     - Case sensitive - Otherwise, replace functions "strstr" by "strcasestr"
     - Always allocates memory for the result.
--------------------------------------------------------------------------- */
static char* replace(const char *str, const char *oldstr, const char *newstr, int *count)
{
   const char *tmp = str;
   char *result;
   int   found = 0;
   int   length, reslen;
   int   oldlen = (int)strlen(oldstr);
   int   newlen = (int)strlen(newstr);
   int   limit = (count != NULL && *count > 0) ? *count : -1; 

   tmp = str;
   while ((tmp = strstr(tmp, oldstr)) != NULL && found != limit)
      found++, tmp += oldlen;
   
   length = (int)strlen(str) + found * (newlen - oldlen);
   if ( (result = (char *)malloc(length+1)) == NULL) {
      fprintf(stderr, "Not enough memory\n");
      found = -1;
   } else {
      tmp = str;
      limit = found; /* Countdown */
      reslen = 0; /* length of current result */ 
      /* Replace each old string found with new string  */
      while ((limit-- > 0) && (tmp = strstr(tmp, oldstr)) != NULL) {
         length = (int)(tmp - str); /* Number of chars to keep intouched */
         strncpy(result + reslen, str, length); /* Original part keeped */ 
         strcpy(result + (reslen += length), newstr); /* Insert new string */
         reslen += newlen;
         tmp += oldlen;
         str = tmp;
      }
      strcpy(result + reslen, str); /* Copies last part and ending nul char */
   }
   if (count != NULL) *count = found;
   return result;
}

static double Factorial(int value)
{
  double result;
  result = (value <= 1) ? value : (value * Factorial(value - 1));
  return result;
}

//查看变量是否存在
static bool VariableExists(const char *name, int *idx)
{
  int i;
  bool iFound = false;

  *idx = -1;

  for (i = 0; i < nVars; i++)
    {
      if (strcasecmp(name, varTable[i].name) == 0)
        {
          iFound = true;
          *idx = i;
          break;
        }
    } /* end for */
  return iFound;
}

//增加一个自定义变量
int AddUserVar(const char *name, double value)
{
  int iExists = false;
  int idx = 0;

  iExists = VariableExists(name, &idx);
  if (iExists)
    {
      varTable[idx].value = value;//更改这个变量
      return 0;
    }

  if (nVars == MAX_VARIABLE - 1)
    {
      hasError = 1;
      fprintf(stderr, "变量索引越界!\n");
      return -1;
    }

  strncpy(varTable[nVars].name, name, MAX_NAME- 1);
  varTable[nVars].name[MAX_NAME- 1] = '\0';

  varTable[nVars].value = value;

  nVars++;

  return 0;
}

//设置某个变量的值
void SetValue(const char *name, double value)
{
  int idx = 0;
  if (!VariableExists(name, &idx))
    {
      fprintf(stderr, "变量\"%s\" 不存在!\n", name);
      return;
    }

  varTable[idx].value = value;
}

//得到某个变量的值
double GetValue(const char *name)
{
  int idx = 0;
  if (!VariableExists(name, &idx))
    {
      hasError = 1;
      fprintf(stderr, "变量\"%s\" 不存在!\n", name);
      return -1; // do not use this return code
    }

  return varTable[idx].value;
}

//将某个变量的值加上某个固定的值
void IncValue(const char *name, double value)
{
  int idx = 0;
  if (!VariableExists(name, &idx))
    {
      hasError = 1;
      fprintf(stderr, "变量\"%s\" 不存在!\n", name);
      return;
    }

  varTable[idx].value += value;
}

//将某个变量的值减去某个固定的值
void DecValue(const char *name, double value)
{
  int idx = 0;
  if (!VariableExists(name, &idx))
    {
      //hasError = 1;
      fprintf(stderr, "变量\"%s\" 不存在!\n", name);
      return;
    }

  varTable[idx].value -= value;
}

//查看常量是否存在
static bool ConstantsExists(const char *name)
{
  int i;
  bool iFound = false;

  for (i = 0; i < nConsts; i++)
    {
      if (strcasecmp(name, constTable[i].name) == 0)
        {
          iFound = true;
          break;
        }
    } /* end for */

  return iFound;
}

//增加用户自定义常量
int AddUserConstant(const char *name, double value)
{
  int iExists = false;

  iExists = ConstantsExists(name);
  if (iExists)
    {
      fprintf(stderr, "常量已经存在!\n");
      return 0;
    }

  if (nConsts == MAX_CONSTS - 1)
    {
      hasError = 1;
      fprintf(stderr, "常量索引越界!\n");
      return -1; 
    }

  strncpy(constTable[nConsts].name, name, MAX_NAME- 1);
  constTable[nConsts].name[MAX_NAME- 1] = '\0';

  constTable[nConsts].value = value;

  nConsts++;

  return 0;
}

/* 检查用户自定义函数是否已经存在 */
static bool UserFunctionExists(char *fname, int *idx)
{
  int i;
  bool iFound = false;

  *idx = 0;
  for (i= 0; i < nUserFuncs; i++)
    {
      if (strcasecmp(fname, userFuncList[i].name) == 0)
        {
          *idx = i;
          iFound = true;
          break;
        }
    }
  return iFound;
}

static bool GetUserFunction(char *fname)
{
  char *funcName;
  int i;
  char tmpStr[MAX_NAME];
  bool result = false;

  if (IsLetter(FParseText[iCurPos]))
    {
      for (i = 0; i < nUserFuncs; i++)
        {
          funcName = userFuncList[i].name;
          int iLen = (int)strlen(funcName);

          memset(tmpStr, 0, sizeof(tmpStr));
          SubString (FParseText, iCurPos, iLen, tmpStr);

          if (strcasecmp(tmpStr, funcName) == 0)
            {
              if (FParseText[iCurPos + iLen] == '(')
                {
                  iCurPos += iLen;
                  strcpy(fname, funcName);
                  return true;
                }
            }
        }//end for
    }
  return result;
}

static bool GetFunction(TParserFunc *AValue)
{
  const char *_funcName = NULL;
  int i;
  char tmpStr[MAX_NAME];
  bool result = false;

  *AValue = pfArcTan;
  if (IsLetter(FParseText[iCurPos]))
    {
      for (i = 0; i < FuncTableSize; i++)
        {
          TParserFunc func = funcTables[i].func;
          _funcName = funcTables[i].name;
          int iLen = (int)strlen(_funcName);

          memset(tmpStr, 0, sizeof(tmpStr));
          SubString (FParseText, iCurPos, iLen, tmpStr);
          if (strcasecmp(tmpStr, _funcName) == 0)
            {
              *AValue = func;
              if (FParseText[iCurPos + iLen] == '(')
                {
                  result = true;
                  iCurPos += iLen;
                  break;
                }
            }
        }//end for
    }//end if
  return result;
}

static bool GetConst(double *AValue)
{
  int i;
  char tmpStr[MAX_NAME];
  bool result;

  result = false;
  *AValue = 0;
  if (IsLetter(FParseText[iCurPos]))
    {
      for (i = 0; i < nConsts; i++)
        {
          char *constName = constTable[i].name;
          int iLen = (int)strlen(constName);

          memset(tmpStr, 0, sizeof(tmpStr));
          SubString (FParseText, iCurPos, iLen, tmpStr);
          if (strcasecmp(tmpStr,constName) == 0)
            {
              *AValue = constTable[i].value;
              iCurPos += iLen;
              result = true;
              break;
            }
        }//end for
    }// end if
  return result;
}

static bool GetVariable(double *AValue)
{
  int i;
  char tmpStr[MAX_NAME];
  bool result;

  result = false;
  *AValue = 0;
  if (IsLetter(FParseText[iCurPos]))
    {
      for (i = 0; i < nVars; i++)
        {
          char *varName = varTable[i].name;
          int iLen = (int)strlen(varName);

          memset(tmpStr, 0, sizeof(tmpStr));
          SubString (FParseText, iCurPos, iLen, tmpStr);
          if (strcasecmp(tmpStr,varName) == 0)
            {
              *AValue = varTable[i].value;
              iCurPos += iLen;
              result = true;
              break;
            }
        }//end for
    }// end if
  return result;

}

/* 注册用户自定义函数 */
void RegisterUserFunction(char *funcName, UserFunction proc)
{
  int idx;
  if (strlen(funcName) > 0 && IsLetter(funcName[0]))
    {
      bool iExists = UserFunctionExists(funcName, &idx);
      if (!iExists)
        {
          strcpy(userFuncList[nUserFuncs].name, funcName);
          userFuncList[nUserFuncs].func = proc;
          nUserFuncs++;
        }
      else
        {
          hasError = 1;
          fprintf(stderr, "语法解析错误!\n");
        }
    }
}

static double Factor()
{
  double Value;
  char tmpStr[MAX_NAME];

  TParserFunc NoFunc;
  char UserFuncName[MAX_NAME];

  memset(tmpStr, 0, sizeof(tmpStr));
  memset(UserFuncName, 0, sizeof(UserFuncName));

  Value = 0;
  if (FParseText[iCurPos] == '(')
    {
      iCurPos++;
      Value = Expression();
      if (FParseText[iCurPos] != ')')
        {
          hasError = 1;
          fprintf(stderr, "括号不匹配!\n");
          return -1; //do not use this return code.
        }
      iCurPos++;
    }
  else
    {
      if (IsNumeric(FParseText[iCurPos]))
        {
          while (IsNumeric(FParseText[iCurPos]))
            {
              sprintf(tmpStr + strlen(tmpStr), "%c", FParseText[iCurPos]);
              iCurPos++;
            }// end while
          Value = atof(tmpStr);
        }
      else
        {
          if (!GetConst(&Value)) //如果不是常量
              if (!GetVariable(&Value)) //如果不是变量
                {
                  if (GetUserFunction(UserFuncName)) //如果是用户定义的方法
                    {
                      iCurPos++;

                      int idx;
                      bool iExists = UserFunctionExists(UserFuncName, &idx);
                      if (iExists)
                          Value = userFuncList[idx].func(Expression());
                      if (FParseText[iCurPos] != ')')
                        {
                          hasError = 1;
                          fprintf(stderr, "扩号不匹配!\n");
                          return -1; // do not use this return code.
                        }
                      iCurPos++;
                    }
                  else 
                      if (GetFunction(&NoFunc)) //如果是系统预定义的方法
                        {
                          iCurPos++;
                          Value = Expression();
                          switch (NoFunc) {
                            case pfArcTan: Value = atan(Value); break;
                            case pfCos: Value = cos(Value); break;
                            case pfSin: Value = sin(Value); break;
                            case pfTan:
                                        if (cos(Value) <=  EPSILON) 
                                          {
                                            hasError = 1;
                                            fprintf(stderr, "除数为零!\n");
                                            return -1; //do not use this return code.
                                          }
                                        else
                                            Value = tan(Value); 
                                        break;

                            case pfAbs: Value = fabs(Value); break;
                            case pfExp: Value = exp(Value); break;

                            case pfLn: 
                                        if (Value <= EPSILON)
                                          {
                                            hasError = 1;
                                            fprintf(stderr, "解析Ln函数错误!\n");
                                            return -1; // do not use this return code.
                                          }
                                        else
                                            Value = log(Value) / log(10);
                                        break;

                            case pfLog:
                                        if (Value <= 0)
                                          {
                                            hasError = 1;
                                            fprintf(stderr, "解析Log函数错误!\n");
                                            return -1; //do not use this return code
                                          }
                                        else
                                            Value = log(Value);
                                        break;

                            case pfSqrt:
                                        if (Value < 0)
                                          {
                                            hasError = 1;
                                            fprintf(stderr, "解析Sqrt函数错误!\n");
                                            return -1; //do not use this return code
                                          }
                                        else
                                            Value = sqrt(Value); 
                                        break;

                            case pfFact:// 阶乘
                                        if (Value < 0)
                                          {
                                            hasError = 1;
                                            fprintf(stderr, "解析Factorial函数错误!\n");
                                            return -1; //do not use this return code
                                          }
                                        else
                                            Value = Factorial((int)Value);
                                        break;

                            case pfSqr:     Value = pow(Value, 2); break;
                            case pfInt:     Value = (int)Value; break;
                            case pfRound:   Value = round(Value); break;
                            case pfFloor:   Value = floor(Value); break;
                            case pfCeiling: Value = ceil(Value); break;
                            case pfArcSin:  Value = (Value == 1) ? PI/2 : asin(Value); break;
                            case pfArcCos:  Value = (Value == 1) ? 0 : acos(Value); break;
                            case pfSign: Value = (Value > 0) ? 1 : -1; break;
                          } // end switch

                          if (FParseText[iCurPos] != ')')
                            {
                              hasError = 1;
                              fprintf(stderr, "扩号不匹配!\n");
                              return -1; //do not use this return code
                            }

                          iCurPos++;
                        }
                      else
                        {
                          hasError = 1;
                          fprintf(stderr, "表达式语法错误!\n");
                          return -1; //do not use this return code
                        }
                }
        }// end else
    }// end else

  return Value;
}

static double Term()
{
  double Value;
  double tmpValue;

  Value = Factor();
  while (IsMulOp(FParseText[iCurPos]))
    {
      iCurPos++;
      switch (FParseText[iCurPos - 1])
        {
        case '*': Value = Value * Factor(); break;
        case '^': Value = pow(Value, Factor()); break;
        case '/':
            tmpValue = Factor();
            if (tmpValue <= EPSILON)
              {
                hasError = 1;
                fprintf(stderr, "被零除!\n");
                return -1; // do not use return value, use hasError instead
              }
            Value = Value / Factor();
            break;
        }// end switch
    }// end while
  return Value;
}

static double Expression()
{
  double Value;

  Value = Term();
  while (IsAddOp(FParseText[iCurPos]))
    {
      iCurPos++;
      switch (FParseText[iCurPos - 1])
        {
        case '+': Value += Term(); break;
        case '-': Value -= Term(); break;
        }// end switch
    }// end while

  if (!((FParseText[iCurPos] == '\0') ||
        FParseText[iCurPos] == ')' ||
        FParseText[iCurPos] == '='))
    {
      hasError = 1;
      return -1; //do not use return value, use hasError to check error.
    }

  return Value;
}

static void addSysConsts()
{
  static bool isAdded = false;

  if (isAdded) return;
  AddUserConstant("PI", PI);
  AddUserConstant("E",  E);
  isAdded = true;
}

//计算表达式
double Exec(const char *formula)
{
  double result = 0;
  char *ptr = NULL;

  char sFormula[MAX_FORMULA_LEN];
  char sTmp[MAX_FORMULA_LEN];
  char sParseText[MAX_FORMULA_LEN];

  hasError = 0;

  if (!formula || '\0' == *formula)
    {
      hasError = 1;
      fprintf(stderr, "表达式为空!\n");
      return 0;
    }

  /*  这里的判断实际上是不太准确的 */
  if (strlen(formula) >= MAX_FORMULA_LEN)
    {
      hasError = 1;
      fprintf(stderr, "表达式长度越界!\n");
      return 0;
    }

  memset(sFormula, 0, sizeof(sFormula));
  memset(sTmp, 0, sizeof(sTmp));
  memset(sParseText, 0, sizeof(sParseText));

  /* 去除所有的空格 */
  strcpy(sTmp, formula);
  StrTrim(sTmp, sFormula);
  if (0 == strlen(sFormula))
    {
      hasError = 1;
      fprintf(stderr, "表达式为空!\n");
      return 0;
    }

  int i = 0;
  int j = 0;
  int k = 0;
  memset(sTmp, 0, sizeof(sTmp));
  // 首先查找括号是否匹配
  for (i = 0; i < strlen(sFormula); i++)
    {
      switch (sFormula[i])
        {
        case '(': j++; break;
        case ')': j--; break;
        }// end case
      if (sFormula[i] > ' ')
          sTmp[k++] = sFormula[i];
    }// end for

  if (j != 0) // 括号不匹配
    {
      hasError = 1;
      fprintf(stderr, "括号不匹配!\n");
      return 0;
    }

  sTmp[k] = '\0';// 最后加入一个'\0'结束符
  // 如果第一个字符为+,-，则字符串变为 "0+x"或者"0-x"
  if (sTmp[0] == '+' || sTmp[0] == '-') 
    {
      sprintf(sParseText, "0%s", sTmp);
    }
  else
    {
      sprintf(sParseText, "%s", sTmp);
    }

  /* 将所有的"(-"替换为"(0-". 
   * e.g. abs(-10.2) ==> abs(0-10.2)
   * */
  memset(sTmp, 0, sizeof(sTmp));
  ptr = replace(sParseText, "(-", "(0-", NULL);
  if (ptr != NULL)
    {
      strcpy(sTmp, ptr);
      free(ptr);
    }

  /* 将所有的"(+"替换为"(0+".
   * e.g. abs(+10.2) ==> abs(0+10.2)
   * */
  memset(sParseText, 0, sizeof(sTmp));
  ptr = replace(sTmp, "(+", "(0+", NULL);
  if (ptr != NULL)
    {
      strcpy(sParseText, ptr);
      free(ptr);
    }

  memset(FParseText, 0, sizeof(FParseText));
  strcpy(FParseText, sParseText);

  addSysConsts();

  iCurPos = 0;
  result = Expression();
  return result;
}

int HasError()
{
  return hasError;
}
