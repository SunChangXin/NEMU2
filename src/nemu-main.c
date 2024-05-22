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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common.h>
#include "/home/scx/ysyx-workbench/nemu/src/monitor/sdb/sdb.h"

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

int main(int argc, char *argv[])
{
  /* Initialize the monitor.  函数体在 src/monitor/monitor.c */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

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
          printf("Test passed for expression: '%s'\n", expression);
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

  /* Start engine.   函数体在 src/engine/interpreter/init.c */
  engine_start();

  return is_exit_status_bad(); // src/utils/state.c
}
