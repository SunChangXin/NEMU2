/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/
#include <stdint.h>
#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdlib.h> // 包含 atoi 函数的头文件
#include <string.h> // 包含 strlen 函数的头文件

// 函数声明：
void init_regex();
static bool make_token(char *e);
bool check_parentheses(int m, int n);
word_t eval(int p, int q);
int char2int(char s[]);
word_t expr(char *e, bool *success);

enum
{
  TK_NOTYPE = 256,
  TK_EQ = 1,
  TK_NEQ = 2,
  TK_NUM = 3,
  TK_LK = 4,
  TK_RK = 5,
  TK_LQ = 6,
  TK_RQ = 7,
  TK_AND = 8,
  TK_OR = 9,
  TK_NOT = 10,
  HEX = 11,
  RESGISTER = 12,
  /* TODO: Add more token types */

};

static struct rule
{
  const char *regex;
  int token_type;
} rules[] = {

    /* TODO: Add more rules.
     * Pay attention to the precedence level of different rules.
     */

    {" +", TK_NOTYPE}, // 空格
    {"\\+", '+'},      // 加减乘除
    {"\\-", '-'},
    {"\\*", '*'},
    {"\\/", '/'},

    {"\\(", TK_LK}, // 括号
    {"\\)", TK_RK},

    {"[0-9]", TK_NUM},

    {"\\&\\&", TK_AND}, // 逻辑运算符
    {"\\|\\|", TK_OR},
    {"\\!", TK_NOT},

    {"<\\=", TK_LQ},
    {">\\=", TK_RQ},
    {"\\=\\=", TK_EQ},  // 相等
    {"\\!\\=", TK_NEQ}, // 不相等

    {"0[xX][0-9a-fA-F]+", HEX},        // 十六进制地址
    {"\\$[a-zA-Z]*[0-9]*", RESGISTER}, // 寄存器
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};
bool division_zero = false;

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex()
{
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i++)
  {
    // 初始化时将 rules[] 存入 re[] 中：
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED); // REG_EXTENDED：是一个编译选项，表示使用扩展的正则表达式语法进行编译

    if (ret != 0)
    {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token
{
  int type;
  char str[32];
} Token;

static Token tokens[1024] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e)
{
  int position = 0;
  int i;

  /*
    typedef struct {
    regoff_t rm_so;  // 匹配子字符串在原始字符串中的起始位置
    regoff_t rm_eo;  // 匹配子字符串在原始字符串中的结束位置的下一个位置
    } regmatch_t;
  */

  regmatch_t pmatch;

  nr_token = 0;

  /*
  for(int temp=0;temp<NR_REGEX;temp++)
    printf("%s ", rules[temp].regex);  		 // 打印正则表达式的源码
  printf("\n");
  */

  while (e[position] != '\0')
  {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i++)
    {

      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0)
      {
        // char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // printf("%d\n",pmatch.rm_eo - pmatch.rm_so);
        // printf("%s\n",substr_start);

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type)
        {
        case '+':
          tokens[nr_token++].type = '+';
          break;
        case '-':
          tokens[nr_token++].type = '-';
          break;
        case '*':
          tokens[nr_token++].type = '*';
          break;
        case '/':
          tokens[nr_token++].type = '/';
          break;

        case TK_LK:
          tokens[nr_token++].type = '(';
          break;
        case TK_RK:
          tokens[nr_token++].type = ')';
          break;

        case TK_AND:
          tokens[nr_token].type = 8;
          strncpy(tokens[nr_token++].str, &e[position - substr_len], substr_len);
          break;
        case TK_OR:
          tokens[nr_token].type = 9;
          strncpy(tokens[nr_token++].str, &e[position - substr_len], substr_len);
          break;
        case TK_NOT:
          tokens[nr_token++].type = '!';
          break;

        case TK_LQ:
          tokens[nr_token].type = 6;
          strncpy(tokens[nr_token++].str, &e[position - substr_len], substr_len);
          break;
        case TK_RQ:
          tokens[nr_token].type = 7;
          strncpy(tokens[nr_token++].str, &e[position - substr_len], substr_len);
          break;

        case TK_EQ:
          tokens[nr_token].type = 1;
          strncpy(tokens[nr_token++].str, &e[position - substr_len], substr_len);
          break;
        case TK_NEQ:
          tokens[nr_token].type = 2;
          strncpy(tokens[nr_token++].str, &e[position - substr_len], substr_len);
          break;

        case TK_NUM:
          tokens[nr_token].type = 3;
          strncpy(tokens[nr_token++].str, &e[position - substr_len], substr_len);
          break;
        case HEX:
          tokens[nr_token].type = 11;
          strncpy(tokens[nr_token++].str, &e[position - substr_len], substr_len);
          break;
        case RESGISTER:
          tokens[nr_token].type = 12;
          strncpy(tokens[nr_token++].str, &e[position - substr_len], substr_len);
          break;

        case TK_NOTYPE:
          break;

        default:
          break;
        }

        break;
      }
    }

    if (i == NR_REGEX)
    {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int m, int n)
{
  // printf("m: %d n: %d\n",m ,n);

  if (tokens[m++].type != '(' || tokens[n--].type != ')')
    return false;
  else
  {
    /*
    while (m < n) {
          if (tokens[m].type == '(') {
              if(tokens[n].type == ')'){
                m++;n--;
              } else {
                n--;
              }

          } else if (tokens[m].type == ')') {
              return false;

          } else m++;

    }
    return true;
    */

    int cnt = 0;
    for (int i = m; i <= n; i++)
    {
      if (cnt >= 0)
      {
        if (tokens[i].type == '(')
        {
          cnt++;
        }
        else if (tokens[i].type == ')')
        {
          cnt--;
        }
      }
      else
      {
        return false;
      }
      // printf("cnt: %d\n",cnt);
    }
    return (cnt == 0);
  }
}

// 返回类型？
word_t eval(int p, int q)
{
  // printf("p: %d\n", p);
  // printf("q: %d\n", q);

  if (p > q)
  {
    assert(0);
    return -1;
  }
  else if (p == q)
  {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    // printf("atoi return: %d\n", atoi(tokens[p].str));
    return atoi(tokens[p].str);
  }
  else if (check_parentheses(p, q) == true)
  {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    // printf("check_parentheses true\n");
    return eval(p + 1, q - 1);
  }
  else
  {
    /* We should do more things here. */

    int op = -1; // the position of 主运算符 in the token expression;

    // 设置 op_type
    int op_type = 0;

    bool op_change = 0;

    // 从左往右 依次执行
    for (int i = p; i < q; i++)
    {

      if (tokens[i].type == 8 || tokens[i].type == 9) //  && ||    优先级最低
      {
        op = i;
        op_type = tokens[i].type;
        op_change = 1;
      }
      else if (tokens[i].type == 1 || tokens[i].type == 2) //  == !=
      {
        if (!(op_type == 8 || op_type == 9))
        {
          op = i;
          op_type = tokens[i].type;
          op_change = 1;
        }
      }
      else if (tokens[i].type == 6 || tokens[i].type == 7) //  <= >=
      {
        if (!(op_type == 8 || op_type == 9 || op_type == 1 || op_type == 2))
        {
          op = i;
          op_type = tokens[i].type;
          op_change = 1;
        }
      }
      else if (tokens[i].type == '+' || tokens[i].type == '-')
      {
        if (!(op_type == 1 || op_type == 2 || op_type == 6 || op_type == 7 || op_type == 8 || op_type == 9))
        {
          op = i;
          op_type = tokens[i].type;
          op_change = 1;
        }
      }
      else if (tokens[i].type == '*' || tokens[i].type == '/')
      {
        if (op_type == '*' || op_type == '/' || op_type == 0)
        {
          op = i;
          op_type = tokens[i].type;
          op_change = 1;
        }
      }
      else if (tokens[i].type == '(')
      {
        // 方案迭代：
        // 01 while(tokens[i].type != ')') { i++; }

        /* 02
        int t = q;
        while(tokens[t].type != ')'){ t--; };
        i = t;
        */

        // 03 最终版
        i++;
        int cnt = 1;
        while (cnt)
        {
          if (tokens[i].type == '(')
          {
            cnt++;
          }
          else if (tokens[i].type == ')')
          {
            cnt--;
          }
          i++;
        }
        i--;
      }

      // printf("循环过程中 i : %d  op : %d\n", i, op);
      // printf("type[%d]: %d op_type: %d\n", i, tokens[i].type, op_type);
    }

    // 如果op没变 说明当前子表达式为 数字
    // 对其进行拆分
    op = (op_change ? op : q);

    // printf("拆分表达式后 op: %d\n", op);
    // assert(op != -1);

    word_t val1 = eval(p, op - 1);

    // printf("op: %d q: %d\n", op, q);

    word_t val2 = eval(op + 1, q);

    switch (op_type)
    {
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      if (val2 == 0)
      {
        division_zero = true;
        return 0;
      }

      // printf("val1: %d val2: %d\n", val1, val2);
      int val1_signed = (int)val1; // 将 val1 转换为有符号整数
      int val2_signed = (int)val2; // 将 val2 转换为有符号整数
      // printf("val1_signed: %d val2_signed: %d\n", val1_signed, val2_signed);
      // printf("val1_signed / val2_signed: %d", val1_signed / val2_signed);

      return val1_signed / val2_signed;

      return val1 / val2;

    case 1:
      return val1 == val2;
    case 2:
      return val1 != val2;
    case 6:
      return val1 <= val2;
    case 7:
      return val1 >= val2;
    case 8:
      return val1 && val2;
    case 9:
      return val1 || val2;
    default:
      return val1 + val2; // 默认返回加法 是为了迁就子表达式为数字的特殊情况 不知道未来会不会出现bug
    }
  }
}
// 该函数将输入的数字字符串转化为相应的整数
int char2int(char s[])
{
  int s_size = strlen(s);
  int res = 0;
  for (int i = 0; i < s_size; i++)
  {
    res += s[i] - '0';
    res *= 10;
  }
  res /= 10;
  return res;
}

word_t expr(char *e, bool *success)
{
  if (!make_token(e))
  {
    *success = false;
    return 0;
  } // 成功 则将规则编译进 tokens 中

  // 在调用eval函数前 我们需要对 tokens 做一些初始化：

  /*
   * 获取 tokens 长度
   *
   */
  int tokens_len = 0;
  while (tokens[tokens_len].type != 0)
  {
    tokens_len++;
  }
  // printf("before tokens_len: %d\n", tokens_len);
  // printf("%d", tokens[tokens_len-1].type);

  /*
   * 初始化 负数
   *
   */
  for (int i = 0; i < tokens_len; i++)
  {
    if ((i == 0 && tokens[i].type == '-') || (i > 0 && tokens[i - 1].type != TK_NUM && tokens[i - 1].type != ')' && tokens[i].type == '-' && tokens[i + 1].type == TK_NUM))
    {

      // printf("0： %d i: %d\n", tokens[i+1].str[0], i);

      // 将字符数组按位后移
      // warning: 如果数字位数很大 可能导致bug
      for (int j = 30; j >= 0; j--)
      {
        tokens[i + 1].str[j + 1] = tokens[i + 1].str[j];
        // printf("%c\n", tokens[i+1].str[j+1]);
      }
      // str[0]作为符号位
      tokens[i + 1].str[0] = '-';
      // printf("1： %d i: %d\n", tokens[i+1].str[1], i);

      // 将所有负号消除
      tokens[i].type = TK_NOTYPE;
    }

    // 去除 TK_NOTYPE
    for (int j = 0; j < tokens_len; j++)
    {
      if (tokens[j].type == TK_NOTYPE)
      {
        for (int k = j; k < tokens_len - 1; k++)
        {
          tokens[k] = tokens[k + 1];
        }
        tokens_len--;
      }
    }
  }

  /*
   * 初始化 解引用       TODO
   *
   */
  for (int i = 0; i < tokens_len; i++)
  {
    // 条件？
    if (tokens[i].type == '*' &&
        (i == 0 || (i != 0 && tokens[i - 1].type != ')' && tokens[i - 1].type != TK_NUM)))
    {

      tokens[i].type = TK_NOTYPE;
      int tmp = char2int(tokens[i + 1].str); // 获取下一位的值
      printf("解引用 tmp: %d\n", tmp);

      // uintptr_t 是 C 和 C++ 标准库 <stdint.h>（或 <cstdint>）中定义的一个整数类型
      // 它是一种无符号整数类型，用于存储指针值和地址
      // uintptr_t a = (uintptr_t)tmp;
      // int value = *((int *)a);
      // printf("value: %d\n", value);

      sprintf(tokens[i + 1].str, "%d", tmp); // 将下一位的值换为对应数值的字符串

      // 去除 TK_NOTYPE
      for (int j = 0; j < tokens_len; j++)
      {
        if (tokens[j].type == TK_NOTYPE)
        {
          for (int k = j + 1; k < tokens_len; k++)
          {
            tokens[k - 1] = tokens[k];
          }
          tokens_len--;
        }
      }
    }
  }

  /*
   * 初始化 寄存器
   *
   */
  for (int i = 0; i < tokens_len; i++)
  {
    if (tokens[i].type == 12)
    {
      bool flag = true;
      char str_tmp[10];
      strcpy(str_tmp, tokens[i].str + 1);
      // printf("str_tmp: %s\n", str_tmp);
      int tmp = isa_reg_str2val(str_tmp, &flag); // 获取 对应寄存器中的值

      if (flag)
      {
        sprintf(tokens[i].str, "%d", tmp);
        // printf("str: %s\n", tokens[i].str);
      }
      else
      {
        printf("Transfrom error. \n");
        assert(0);
      }

      /* for (int j = 0; j < tokens_len; j++)
      {
        if (tokens[j].type == TK_NOTYPE)
        {
          for (int k = j + 1; k < tokens_len; k++)
          {
            tokens[k - 1] = tokens[k];
          }
          tokens_len--;
        }
      } */
    }
  }

  /*
   * 初始化十六进制地址   TODO
   *
   */
  for (int i = 0; i < tokens_len; i++)
  {
    if (tokens[i].type == 11) // HEX
    {
      int value = strtol(tokens[i].str, NULL, 16); // 将十六进制字符串转换为整数
      sprintf(tokens[i].str, "%d", value);         // 将整数存入到 str 中
    }
  }

  /*
   * 初始化 '!'
   *
   */
  for (int i = 0; i < tokens_len; i++)
  {
    if (tokens[i].type == '!')
    {
      tokens[i].type = TK_NOTYPE;
      int tmp = char2int(tokens[i + 1].str);
      if (tmp == 0) // 如果转换结果为 0，则将下一个令牌的字符串清零，并将其第一个字符设置为 '1' [!0 = 1]
      {
        memset(tokens[i + 1].str, 0, sizeof(tokens[i + 1].str));
        tokens[i + 1].str[0] = '1';
      }
      else // 否则，仅清零下一个令牌的字符串 [!1 = 0]
      {
        memset(tokens[i + 1].str, 0, sizeof(tokens[i + 1].str));
      }

      // 去除 TK_NOTYPE
      for (int j = 0; j < tokens_len; j++)
      {
        if (tokens[j].type == TK_NOTYPE)
        {
          for (int k = j + 1; k < tokens_len; k++)
          {
            tokens[k - 1] = tokens[k];
          }
          tokens_len--;
        }
      }
    }
  }

  // printf("after tokens_len: %d\n", tokens_len);

  // 求值：
  uint32_t res = 0;
  res = eval(0, tokens_len - 1);

  // 除零警告：
  if (!division_zero)
  {
    // printf("res = %d\n", res);
    *success = true;
  }
  else
  {
    printf("Your input have an error: can't division zero\n");
    *success = false;
  }

  // 重置系统 以便下一次求值：
  division_zero = false;
  memset(tokens, 0, sizeof(tokens));

  return success ? res : 0;
}
