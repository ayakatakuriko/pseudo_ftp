/*
 * io.h
 *
 *  Created on: 2018/10/17
 *      Author: user
 */

#ifndef IO_H_
#define IO_H_


#define DELM " \t\n"

/**
 * @brief 文字列をトークンで分割する
 * @param  str     分割する文字列
 * @param  list    分割した文字列を格納する
 * @param  len     listの長さ
 * @return         listに格納されたトークン数
 */
int split(char *str, char **list, int len);


#endif /* IO_H_ */
