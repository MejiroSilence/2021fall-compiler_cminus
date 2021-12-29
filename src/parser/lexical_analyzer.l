%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>

#include "syntax_tree.h"
#include "syntax_analyzer.h"

int lines;
int pos_start;
int pos_end;
int i;

void pass_node(char *text){
     yylval.node = new_syntax_tree_node(text);
}

/*****************声明和选项设置  end*****************/

%}


%%
 /* to do for students */
 /* two cases for you, pass_node will send flex's token to bison */


"float" {pos_start = pos_end; pos_end += 5; pass_node(yytext); return FLOAT;}
"int" {pos_start = pos_end; pos_end += 3; pass_node(yytext); return INT;}
"void" {pos_start = pos_end; pos_end += 4; pass_node(yytext); return VOID;}

"struct" {pos_start = pos_end; pos_end += 6; pass_node(yytext); return STRUCT;}

"if" {pos_start = pos_end; pos_end += 2; pass_node(yytext); return IF;}
"else" {pos_start = pos_end; pos_end += 4; pass_node(yytext); return ELSE;}
"while" {pos_start = pos_end; pos_end += 5; pass_node(yytext); return WHILE;}
"return" {pos_start = pos_end; pos_end += 6; pass_node(yytext); return RETURN;}

[a-zA-Z]+ {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return ID;}
[0-9]+ {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INTEGER;}
[0-9]+"."|[0-9]*"."[0-9]+ {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATPOINT;}

";" {pos_start = pos_end; pos_end++; pass_node(yytext); return SEMICOLON;}
"," {pos_start = pos_end; pos_end++; pass_node(yytext); return COMMA;}
"=" {pos_start = pos_end; pos_end++; pass_node(yytext); return ASSIGN;}

"(" {pos_start = pos_end; pos_end++; pass_node(yytext); return LBRACKETS;}
")" {pos_start = pos_end; pos_end++; pass_node(yytext); return RBRACKETS;}
"[" {pos_start = pos_end; pos_end++; pass_node(yytext); return LBRACKETM;}
"]" {pos_start = pos_end; pos_end++; pass_node(yytext); return RBRACKETM;}
"{" {pos_start = pos_end; pos_end++; pass_node(yytext); return LBRACKETL;}
"}" {pos_start = pos_end; pos_end++; pass_node(yytext); return RBRACKETL;}

\. {pos_start = pos_end; pos_end++; pass_node(yytext); return DOT;}
\+|\- {pos_start = pos_end; pos_end++; pass_node(yytext); return ADDOP;}
\*|\/ {pos_start = pos_end; pos_end++; pass_node(yytext); return MULOP;}
"<"|"<="|">"|">="|"=="|"!=" {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return RELOP;}
[ \t\f\r\n] {if (!strcmp(yytext, "\n")) {
    lines++;
    pos_start = pos_end = 0;
} else {
    pos_start = pos_end;
    pos_end += strlen(yytext);
}}

"/*"([*]*(([^*/])+([/])*)*)*"*/" {for (i = 0; i < strlen(yytext); i++) {
    if (yytext[i] == '\n') {
        lines++;
        pos_start = pos_end = 0;
    }
}
}

"/*" {for (i = 0; i < strlen(yytext); i++) {
    if (yytext[i] == '\n') {
        lines++;
        pos_start = pos_end = 0;
    }
}
return UNDEFINED;
}

. {;}

 /****请在此补全所有flex的模式与动作  end******/
%%
