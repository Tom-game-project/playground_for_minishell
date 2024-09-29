#include <string.h>
#include <stdlib.h>
#include "minishell.h"

t_token *new_token(char *word, t_token_kind kind)
{
    t_token *tok;

    tok = calloc(1, sizeof(*tok));
    if (tok == NULL)
        fatal_error("calloc");
    tok->word = word;
    tok->kind = kind;
    return (tok);
}

bool is_blank(char c)
{
    return (c == ' ' || c == '\t' || c == '\n');
}

bool consume_blank(char **rest, char *line)
{
    if (is_blank(*line))
    {
        while (*line && is_blank(*line))
            line++;
        *rest = line;
        return (true);
    }
    *rest = line;
    return (false);
}

bool startswith(const char *s, const char *keyword)
{
    return (memcmp(s, keyword, strlen(keyword)) == 0);
}

/*
metacharacter
    A character that, when unquated, separates words. One of the following:
    | & ; ( ) < > space tab newline
control operator
    A token that performs a control function. It is a metacharacter or a reserved word.
    || & && ; ;; ( ) | <newline>
*/

bool is_operator (const char *s)
{
    static char *const operators[] = {"||", "&", "&&", ";", ";;", "(", ")", "|", "\n"};
    size_t i = 0;

    while (i < sizeof(operators) / sizeof(operators[0]))
    {
        if (startswith(s, operators[i]))
            return (true);
        i++;
    }
    return (false);
}

/*
DEFINITIONS
    The following definitions are used in the specification:
    blank 
        A space or tab character.
    word 
        A sequence of characters considered as a single unit by the shell. Also known as a token.
    name 
        A word consisting only of alphanumeric characters and underscores, 
        and beginning with an alphabetic character or an underscore. 
        Also referred to as an identifier.
    metacharacter 
        A character that, when unquoted, separates words. 
        One of the following: | & ; ( ) < > space tab
    control operator 
        A token that performs a control function. 
         One of the following symbols:  || & && ; ;; ( ) | <newline>
*/

bool is_metacharacter(char c)
{
    return (c && strchr("|&;()<> \t\n", c));
}

bool is_word(const char *s)
{
    return (*s && !is_metacharacter(*s));
}

/* 文字列からoperatorを抜き出して新しいトークンとして返す */
t_token *operator(char **reset, char *line)
{
    static char *const operators[] = {"||", "&", "&&", ";", ";;", "(", ")", "|", "\n"};
    size_t i = 0;
    char *op; 

    while (i < sizeof(operators) / sizeof(*operators))
    {
        if (startswith(line, operators[i]))
        {
            op = strndup(line, strlen(operators[i]));
            if (op == NULL)
                fatal_error("strndup");
            *reset = line + strlen(operators[i]);
            return (new_token(op, TK_OP));
        }
        i++;
    }
    assert_error("Unexpected operator");
}

t_token *word(char **rest, char *line)
{
    char *start = line;
    char *word;

    while (*line && !is_metacharacter(*line))
    {
        if (*line == SINGLE_QUOTE_CHAR)
        {
            //skip quote
            line++;
            while (*line != SINGLE_QUOTE_CHAR)
            {
                if (*line == '\0')
                    assert_error("Unclosed single quote");
                line++;
            }
            //skip quote
            line++;
        } 
        else if (*line == DOUBLE_QUOTE_CHAR)
        {
            //skip quote
            line++;
            while (*line != DOUBLE_QUOTE_CHAR)
            {
                if (*line == '\0')
                    assert_error("Unclosed double quote");
                line++;
            }
            //skip quote
            line++;
        }
        else 
            line++;
    }
    word = strndup(start, line - start);
    if (word == NULL)
        fatal_error("strndup");
    *rest = line;
    return (new_token(word, TK_WORD));
}

t_token *tokenize(char *line)
{
    t_token head;
    t_token *tok;

    head.next = NULL; //dummy 実際のtokenはこの先に追加される
    tok = &head; //tokは現在操作中のtokenを指すポインタ
    while (*line)
    {
        if (consume_blank(&line, line))
            continue;
        else if (is_operator(line))
            // 新しいトークンをtok->nextに追加し、tokポインタも更新して次のトークンを指すようにする
            tok = tok->next = operator(&line, line);
        else if (is_word(line))
            tok = tok->next = word(&line, line);
        else
            assert_error("Unexpected character");
    }
    tok->next = new_token(NULL, TK_EOF);
    return (head.next);
}

/* tail recursion 再帰呼出しの後に追加の処理を行わない
各再帰ステップでスタックに新しい情報を保持する必要がないので
最適化（スタックフレームの再利用）されやすくなる */
char **tail_recursive(t_token *tok, int nargs, char **argv)
{
    if (tok == NULL || tok->kind == TK_EOF)
        return (argv);
    char **tmp = realloc(argv, (nargs + 2) * sizeof(char *));
    if (tmp == NULL)
    {
        free(argv);
        fatal_error("realloc");
        return NULL;
    }
    argv = tmp;
    argv[nargs] = strdup(tok->word);
    if (argv[nargs] == NULL)
        fatal_error("strdup");
    argv[nargs + 1] = NULL;
    return (tail_recursive(tok->next, nargs + 1, argv));
}

char **token_list_to_argv(t_token *tok)
{
    char **argv;

    argv = calloc(1, sizeof(char *));
    if (argv == NULL)
        fatal_error("calloc");
    return (tail_recursive(tok, 0, NULL));
}

