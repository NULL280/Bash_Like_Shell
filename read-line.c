/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);

// Buffer where line is stored
int line_length;
char line_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
// 历史定位为当前行
int history_index = -1024;
char * history[32];
int history_length = 0;

// 光标位置
int cursor_position = 0;

void print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " Backspace    Deletes last character\n"
    " Up arrow     See last command in the history\n"
    " Down arrow   See next command in the history\n"
    " ctrl-D       Delete key at current position\n"
    " ctrl-H       Delete key at position before the cursor\n"
    " ctrl-A       Move to beginning of line\n"
    " ctrl-E       Move to end of line\n"
    ;

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;

  // Read one line until enter is typed
  while (1) {

    // 读取输入char
    char ch;
    read(0, &ch, 1);

    if (ch>=32 && ch != 127) {
      // 正常字符

      // 显示输入char
      write(1,&ch,1);

      // 行长超标, 不再读取
      if (line_length == MAX_BUFFER_LINE-2) break; 

      // 添加char到行缓存
      line_buffer[cursor_position] = ch;
      // 光标在末尾, 刷新行长
      if (cursor_position == line_length) {
        line_length++;
      }

      // 刷新光标位置
      cursor_position++;
    }
    else if (ch == 10) {
      // 输入char:回车
      
      // 换行
      write(1,&ch,1);

      // 退出 while loop 读取
      break;
    }
    else if (ch == 31) {
      // ctrl-? 说明文档
      print_usage();
      // 清空行缓存
      line_buffer[0]=0;
      break;
    }
    else if (ch == 1) {
      // ctrl-A 光标移到开始位置
    
      // 光标到达开头情况
      if (cursor_position == 0) continue;

      // 移动光标到显示行最左 (注意: 从光标位置开始移动)
      int i = 0;
      for (i = 0; i < cursor_position; i++) {
        ch = 8;
        write(1,&ch,1);
      }

      // 更新光标真实位置
      cursor_position = 0;
    }
    else if (ch == 5) {
      // ctrl-E 光标移到结束位置

      // 光标到达结尾情况
      if (cursor_position == line_length) continue;

      // 移动光标到显示行最右 (注意: 从光标位置开始移动) + 更新光标真实位置
      for (; cursor_position < line_length; cursor_position++) {
        ch = line_buffer[cursor_position];
	      write(1, &ch, 1);
      }
    }
    else if (ch == 4) {
      // ctrl-D 移除光标位置字母

      // 空行情况
      if (line_length == 0) continue;

      // 光标到达结尾情况
      if (cursor_position == line_length) continue;

      // 清空显示行
      // 移动光标到显示行最左 (注意: 从光标位置开始移动)
      int i = 0;
      for (i =0; i < cursor_position; i++) {
        ch = 8;
        write(1,&ch,1);
      }
      // 用空格覆写行
      for (i =0; i < line_length; i++) {
        ch = ' ';
        write(1,&ch,1);
      }
      // 移动光标到显示行最左
      for (i = 0; i < line_length; i++) {
        ch = 8;
        write(1,&ch,1);
      }

      // 更新行缓存
      // 光标位置及右边的字符统一左移一格
      for (int i = cursor_position; i < line_length - 1; i++) {
        line_buffer[i] = line_buffer[i + 1];
      }
      // 更新行缓存长度
      line_length--;

      // 光标真实位置不变

      // 刷新显示行
      for (int i = 0; i < line_length; i++) {
        ch = line_buffer[i];
        write(1,&ch,1);
      }

      // 刷新光标显示位置
      for (int i = 0; i < line_length - cursor_position; i++) {
        ch = 8;
	      write(1,&ch,1);
      }
    }
    else if (ch == 8 || ch == 127) {
      
      // <backspace> 删除前个字母, 一定要包含127!

      // 空行情况
      if (line_length == 0) continue;

      // 光标到达开头情况
      if (cursor_position == 0) continue;

      // 清空显示行
      // 移动光标到显示行最左 (注意: 从光标位置开始移动)
      int i = 0;
      for (i =0; i < cursor_position; i++) {
        ch = 8;
        write(1,&ch,1);
      }
      // 用空格覆写行
      for (i =0; i < line_length; i++) {
        ch = ' ';
        write(1,&ch,1);
      }
      // 移动光标到显示行最左
      for (i = 0; i < line_length; i++) {
        ch = 8;
        write(1,&ch,1);
      }
   
      // 更新行缓存
      // 光标位置及右边的字符统一左移一格
      for (int i = cursor_position; i < line_length; i++) {
        line_buffer[i - 1] = line_buffer[i];
      }
      // 更新行缓存长度
      line_length--;

      
      // 更新光标真实位置
      cursor_position--;

      // 刷新显示行
      for (int i = 0; i < line_length; i++) {
        ch = line_buffer[i];
        write(1,&ch,1);
      }
      
      // 刷新光标显示位置
      for (int i = 0; i < line_length - cursor_position; i++) {
        ch = 8;
	      write(1,&ch,1);
      }
    }
    else if (ch == 27) {
      // ESC 键. 读取 3 char

      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);

      if (ch1 == 91 && ch2 == 65) {
        // 向上键. 历史上一条

        // 空历史情况
        if (history_length == 0) continue;

        // 清空显示行
        // 移动光标到显示行最左
        int i = 0;
        for (i =0; i < line_length; i++) {
          ch = 8;
          write(1,&ch,1);
        }
        // 用空格覆写行
        for (i =0; i < line_length; i++) {
          ch = ' ';
          write(1,&ch,1);
        }
        // 移动光标到显示行最左
        for (i =0; i < line_length; i++) {
          ch = 8;
          write(1,&ch,1);
        }
        
        
        // 获得历史
        // 更新历史定位
        if (history_index == -1024) {
          // 从当前行进入历史记录
          history_index = 0;
        } else {
          history_index=(history_index + 1) % history_length;
        }
        // 复制历史到行缓存
        strcpy(line_buffer, history[history_index]);
        // 更新行缓存长度
        line_length = strlen(line_buffer);

        // 打印行缓存到显示行
        write(1, line_buffer, line_length);

        // 更新光标位置
	      cursor_position = line_length;
      } else if (ch1 == 91 && ch2 == 66) {
        // 向下键, 历史下一条

        // 空历史情况
        if (history_length == 0) continue;

        // 清空显示行
        // 移动光标到显示行最左
        int i = 0;
        for (i =0; i < line_length; i++) {
          ch = 8;
          write(1,&ch,1);
        }
        // 用空格覆写行
        for (i =0; i < line_length; i++) {
          ch = ' ';
          write(1,&ch,1);
        }
        // 移动光标到显示行最左
        for (i =0; i < line_length; i++) {
          ch = 8;
          write(1,&ch,1);
        }


        // 获得历史
        // 更新历史定位
        if (history_index == -1024) {
          // 从当前行进入历史记录
          history_index = history_length - 1;
        } else {
          history_index=(history_length + history_index - 1) % history_length;
        }
        // 复制历史到行缓存
        strcpy(line_buffer, history[history_index]);
        // 更新行缓存长度
        line_length = strlen(line_buffer);

        // 打印行缓存到显示行
        write(1, line_buffer, line_length);

        // 更新光标位置
	      cursor_position = line_length;
      } else if (ch1 == 91 && ch2 == 68) {
        // 向左键, 光标向左

        // 空行情况
        if (line_length == 0) continue;

        // 光标到达开头情况
        if (cursor_position == 0) continue;

        // 左移光标
        ch = 8;
	      write(1,&ch,1);
        // 修改光标位置
	      cursor_position--;

      } else if (ch1 == 91 && ch2 == 67) {
        // 向右键, 光标向右

        // 空行情况
        if (line_length == 0) continue;

        // 光标到达结尾(比行缓存多一格)情况
        if (cursor_position == line_length) continue;

        // 右移光标
        ch = line_buffer[cursor_position];
	      write(1, &ch, 1);
        // 修改光标位置
	      cursor_position++;
      } else if (ch1 == 91 && ch2 == 51) {
        char ch3;
        read(0, &ch3, 1);
        if (ch3 != 0 && ch3 == 126) {
          // Delete按键, 同 ctrl-D, 移除光标位置字母

          // 空行情况
          if (line_length == 0) continue;

          // 光标到达结尾情况
          if (cursor_position == line_length) continue;

          // 清空显示行
          // 移动光标到显示行最左 (注意: 从光标位置开始移动)
          int i = 0;
          for (i =0; i < cursor_position; i++) {
            ch = 8;
            write(1,&ch,1);
          }
          // 用空格覆写行
          for (i =0; i < line_length; i++) {
            ch = ' ';
            write(1,&ch,1);
          }
          // 移动光标到显示行最左
          for (i = 0; i < line_length; i++) {
            ch = 8;
            write(1,&ch,1);
          }

          // 更新行缓存
          // 光标位置及右边的字符统一左移一格
          for (int i = cursor_position; i < line_length - 1; i++) {
            line_buffer[i] = line_buffer[i + 1];
          }
          // 更新行缓存长度
          line_length--;

          // 光标真实位置不变

          // 刷新显示行
          for (int i = 0; i < line_length; i++) {
            ch = line_buffer[i];
            write(1,&ch,1);
          }

          // 刷新光标显示位置
          for (int i = 0; i < line_length - cursor_position; i++) {
            ch = 8;
            write(1,&ch,1);
          }
        }
      }
    }

  }

  // 更新行缓存到历史
  line_buffer[line_length] = '\0';
  history_length++;
  for (int i = history_length - 1; i > 0; i--) {
    history[i] = history[i - 1];
  }

  history[0] = (char *) malloc(line_length + 1);
        
  strcpy(history[0], line_buffer);

  // 补全行缓存
  line_buffer[line_length] = 10;
  line_length++;
  line_buffer[line_length] = 0;

  // 重置历史记录定位
  history_index = -1024;

  // 重置光标位置
  cursor_position = 0;

  return line_buffer;
}