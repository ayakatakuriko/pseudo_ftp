/**
   io.c

   @auther AYAKA OHWADA
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"
#include "io.h"

/**
 * @brief 文字列をトークンで分割する
 * @param  str     分割する文字列
 * @param  list    分割した文字列を格納する
 * @param  len     listの長さ
 * @return         listに格納されたトークン数
 */
int split(char *str, char **list, int len)
{
        char *token;
        int count = 0;
        token = strtok(str, DELM);
        while (token != NULL) {
                if (count >= len) {
                        len *= 2;
                        mem_realloc(list, char *, len, 1);
                }
                list[count++] = token;
                token = strtok(NULL, DELM);
        }
        return count;
}
