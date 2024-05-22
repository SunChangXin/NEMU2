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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <memory/paddr.h>
#include "sdb.h"
#include "/home/scx/ysyx-workbench/nemu/src/monitor/sdb/watchpoint.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char *rl_gets()
{
  static char *line_read = NULL;

  if (line_read)
  {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read)
  {
    add_history(line_read);
  }

  return line_read;
}

/******************** 增删查监视点 **********************/

// 显示监视点
void sdb_watchpoint_display()
{
  bool flag = true;
  for (int i = 0; i < NR_WP; i++)
  {
    if (wp_pool[i].flag)
    {
      printf("Watchpoint.No: %d, expr = \"%s\", old_value = %d, new_value = %d\n",
             wp_pool[i].NO, wp_pool[i].expr, wp_pool[i].old_value, wp_pool[i].new_value);
      flag = false;
    }
  }
  if (flag)
    printf("No watchpoint now.\n");
}

// 删除监视点
void delete_watchpoint(int NO)
{
  for (int i = 0; i < NR_WP; i++)
    if (wp_pool[i].NO == NO)
    {
      free_wp(&wp_pool[i]);
      return;
    }
}

// 增加监视点
void create_watchpoint(char *args)
{
  WP *p = new_wp();
  strcpy(p->expr, args);
  bool success = false;
  int tmp = expr(p->expr, &success);
  if (success)
  {
    p->old_value = tmp;
    printf("Create watchpoint No.%d success.\n", p->NO);
  }
  else
  {
    printf("创建watchpoint的时候expr求值出现问题\n");
    assert(0);
  }
}

/***************** 指令函数声明 *********************/

static int cmd_help(char *args);

static int cmd_c(char *args);

static int cmd_q(char *args);

static int cmd_si(char *args);

static int cmd_p(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static int cmd_gentest();

/******************** 指令集 **********************/

static struct
{
  const char *name;
  const char *description;
  int (*handler)(char *);
} cmd_table[] = {
    {"h", "打印命令的帮助信息", cmd_help},
    {"c", "继续运行被暂停的程序", cmd_c},
    {"q", "退出NEMU", cmd_q},
    {"si", "让程序单步执行N条指令后暂停执行,当N没有给出时, 缺省为1", cmd_si},                               // 单步执行
    {"p", "求出表达式EXPR的值, EXPR支持的运算请见调试中的表达式求值小节", cmd_p},                           // 表达式求值
    {"info", "info r 打印寄存器状态\n\t- info w 打印监视点信息", cmd_info},                                 // 打印寄存器 打印监视点
    {"x", "扫描内存: 求出表达式EXPR的值, 将结果作为起始内存地址, 以十六进制形式输出连续的N个4字节", cmd_x}, // 扫描内存
    {"w", "当表达式EXPR的值发生变化时, 暂停程序执行", cmd_w},
    {"d", "删除序号为N的监视点", cmd_d},
    {"gentest", "表达式求值测试", cmd_gentest},

};

/******************** 指令函数实现 **********************/

#define NR_CMD ARRLEN(cmd_table)

// cmd_help
static int cmd_help(char *args)
{
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL)
  {
    /* no argument given */
    for (i = 0; i < NR_CMD; i++)
    {
      printf("%s\t- %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else
  {
    for (i = 0; i < NR_CMD; i++)
    {
      if (strcmp(arg, cmd_table[i].name) == 0)
      {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

// cmd_c
static int cmd_c(char *args)
{
  /* src/cpu/cpu-exec.c */
  cpu_exec(-1);
  return 0;
}

// cmd_q
static int cmd_q(char *args)
{
  nemu_state.state = NEMU_QUIT;
  return -1;
}

// cmd_si
static int cmd_si(char *args)
{
  int step = 1;
  // printf("arg: %c\n", *args);
  if (args == NULL)
    cpu_exec(step);
  else
  {
    sscanf(args, "%d", &step);
    // printf("step: %d\n", step);
    cpu_exec(step);
  }
  return 0;
}

// cmd_p
static int cmd_p(char *args)
{
  if (args == NULL)
  {
    printf("No args\n");
    return 0;
  }
  Log("args = %s\n", args);
  bool flag = false;
  expr(args, &flag);
  return 0;
}

// cmd_info
static int cmd_info(char *args)
{
  if (args == NULL)
    printf("缺少参数\n");
  else if (strcmp(args, "r") == 0) // *args=='r'： args第一个字符是否为'r'
    isa_reg_display();
  else if (strcmp(args, "w") == 0)
    sdb_watchpoint_display();
  else
    printf("未定义的指令: info %s\n", args);
  return 0;
}

// cmd_x                      			Todo
static int cmd_x(char *args)
{
  if (args == NULL)
  {
    printf("Error: args is NULL\n");
    return 0;
  }

  char *n = strtok(args, " ");
  char *addr_tmp = strtok(NULL, " ");
  int len = 0;
  paddr_t addr = 0;

  if (n == NULL || addr_tmp == NULL)
  {
    printf("Error: Invalid arguments\n");
    return 0;
  }

  sscanf(n, "%d", &len);
  sscanf(addr_tmp, "%x", &addr); // 从输入参数中获取 长度和地址

  /*  include/memory/host.h

    static inline word_t host_read(void *addr, int len) {
      switch (len) {
      case 1: return *(uint8_t  *)addr;
      case 2: return *(uint16_t *)addr;

      //这行代码的作用是将 addr 所指向的内存地址中的内容视为 uint32_t 类型的数据，然后返回这个数据的值
      case 4: return *(uint32_t *)addr;

      IFDEF(CONFIG_ISA64, case 8: return *(uint64_t *)addr);
      default: MUXDEF(CONFIG_RT_CHECK, assert(0), return 0);
      }
    }

  */
  for (int i = 0; i < len; i++)
  {
    printf("%x\n", paddr_read(addr, 4));
    addr = addr + 4;
  }
  return 0;
}

// cmd_w
static int cmd_w(char *args)
{
  create_watchpoint(args);
  return 0;
}

// cmd_d
static int cmd_d(char *args)
{
  if (args == NULL)
    printf("No args.\n");
  else
  {
    delete_watchpoint(atoi(args));
  }
  return 0;
}

// cmd_gentest
static int cmd_gentest()
{
  FILE *fp;
  char line[1024]; // 根据最长行的长度调整大小
  int result, calculated_result;
  char expression[1024]; // 用于存储表达式字符串

  fp = fopen("tools/gen-expr/input", "r");
  if (fp == NULL)
  {
    perror("Error opening file");
    return -1;
  }

  // 逐行读取文件内容
  while (fgets(line, sizeof(line), fp))
  {
    // 去除可能的换行符
    line[strcspn(line, "\n")] = 0;

    // 找到空格分割结果和表达式
    char *endPtr;
    result = (int)strtol(line, &endPtr, 10); // 将结果转换为数字
    if (*endPtr == ' ')
    {
      // 跳过空格
      endPtr++;
      // 复制表达式到单独的字符串
      strncpy(expression, endPtr, sizeof(expression) - 1);
      expression[sizeof(expression) - 1] = '\0';

      // 去除表达式末尾的换行符
      expression[strcspn(expression, "\n")] = 0;

      bool success = false;

      // 调用expr函数计算表达式的值
      calculated_result = expr(expression, &success);
      if (success)
      {
        // 比较结果
        if (calculated_result == result)
        {
          // printf("Test passed for expression: '%s'\n", expression);
        }
        else
        {
          printf("Test failed for expression: '%s' - Expected: %d, Calculated: %d\n", expression, result, calculated_result);
        }
      }
      else
      {
        Log("求值时发生错误.\n");
        assert(0);
      }
    }
    else
    {
      printf("Error parsing line: '%s'\n", line);
    }
  }

  // 关闭文件
  fclose(fp);
  return 0;
}

/******************** 工作循环 **********************/

void sdb_set_batch_mode()
{
  /* 设置批处理模式标志为 true */
  is_batch_mode = true;
}

void sdb_mainloop()
{
  /* 如果处于批处理模式，则执行 cmd_c(NULL) 后返回 */
  if (is_batch_mode)
  {
    cmd_c(NULL);
    return;
  }

  /* 从用户获取命令 */
  // rl_gets 是 GNU Readline 库中的一个函数，用于从用户输入中获取一行文本，并提供编辑和历史记录功能
  for (char *str; (str = rl_gets()) != NULL;)
  {
    char *str_end = str + strlen(str);

    // printf("str_end: %s\n",str_end-1);   str_end : '\0'

    /* 提取第一个字母作为指令 */
    char *cmd = strtok(str, " "); // strtok 是 C 标准库中的一个函数，用于将字符串分割成一系列子字符串

    // 打印cmd
    // Log("cmd: %s\n", cmd);

    if (cmd == NULL)
    {
      continue;
    }

    /* 剩下的作为等待进一步操作的参数 */
    char *args = cmd + strlen(cmd) + 1; // 1 表示空格
    // Log("args: %s\n",args);

    if (args >= str_end)
    {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    /* 如果定义了 CONFIG_DEVICE 宏，则清空事件队列 */
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    /* 遍历命令表 */
    for (i = 0; i < NR_CMD; i++)
    {
      /* 找到对应的命令处理函数并执行 */
      if (strcmp(cmd, cmd_table[i].name) == 0)
      {
        if (cmd_table[i].handler(args) < 0)
        {
          return;
        }
        break;
      }
    }

    /* 如果找不到命令则打印提示信息 */
    if (i == NR_CMD)
    {
      printf("Unknown command '%s'\n", cmd);
    }
  }
}

void init_sdb()
{

  /* expr.c */
  /* 编译正则表达式 */
  init_regex();

  /* watchpoint.c */
  /* 初始化监视点池 */
  init_wp_pool();
}
