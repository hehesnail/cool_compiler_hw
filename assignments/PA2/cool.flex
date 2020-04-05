/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */

int str_len;
bool str_with_null;

%}

/*
 * Define names for regular expressions here.
 */

DARROW      =>
ASSIGN      <-
LE          <=  

digit       [0-9]
number      {digit}+
typeid      [A-Z][A-Za-z0-9_]*
objid       [a-z][A-Za-z0-9_]*
newline     \n
whitespace  [ \t\v\r\f]+

%x comment1 comment2 string

%%


{newline}     {curr_lineno++;}
{whitespace}  { /*Skip the white spaces*/ }

"--"    {BEGIN(comment1);}
"(\*"    {BEGIN(comment2);}
"\*)"    {
    strcpy(cool_yylval.error_msg, "Unmatched *)");
    return (ERROR);
}

<comment1>\n   {
    BEGIN(0); 
    curr_lineno++;
}

<comment2>\n   {curr_lineno++;}
<comment2><<EOF>> {
    strcpy(cool_yylval.error_msg, "EOF in comment");
    BEGIN(0);
    return (ERROR);
}
<comment2>"\*)"   {
    BEGIN(0);
}

<comment1>.   { /*skip the comments started with -- */ }
<comment2>.   { /*skip the comments in (*...*) */ }


"{"   {return '{';}
"}"   {return '}';}
"("   {return '(';}
")"   {return ')';}
"~"   {return '~';}
","   {return ',';}
";"   {return ';';}
":"   {return ':';}
"+"   {return '+';}
"-"   {return '-';}
"*"   {return '*';}
"/"   {return '/';}
"<"   {return '<';}
"="   {return '=';}
"."   {return '.';}
"@"   {return '@';}


 /*
  *  The multiple-character operators.
  */
{DARROW}    {return (DARROW);}
{ASSIGN}    {return (ASSIGN);}
{LE}        {return (LE);}


 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */
(?i:class)    {return CLASS;}
(?i:else)     {return ELSE;}
(?i:fi)       {return FI;}
(?i:if)       {return IF;}
(?i:in)       {return IN;}
(?i:inherits) {return INHERITS;}
(?i:isvoid)   {return ISVOID;}
(?i:let)      {return LET;}
(?i:loop)     {return LOOP;}
(?i:pool)     {return POOL;}
(?i:then)     {return THEN;}
(?i:while)    {return WHILE;}
(?i:case)     {return CASE;}
(?i:esac)     {return ESAC;}
(?i:new)      {return NEW;}
(?i:of)       {return OF;}
(?:not)       {return NOT;}

t[rR][uU][eE]   {
      cool_yylval.boolean = 1;
      return (BOOL_CONST);
}

f[aA][lL][sS][eE]   {
      cool_yylval.boolean = 0;
      return (BOOL_CONST);
}


 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */

\"  {
    memset(string_buf, 0, sizeof(string_buf));
    str_len = 0;
    str_with_null = false;
    BEGIN(string);
}

<string><<EOF>> {
    strcpy(cool_yylval.error_msg, "EOF in string constant");
    BEGIN(0);
    return (ERROR);
}

<string>\\. {
    if (str_len >= MAX_STR_CONST) {
        strcpy(cool_yylval.error_msg, "String constant too long");
        BEGIN(0);
        return (ERROR);
    }
    switch(yytext[1]) {
      case '\"': string_buf[str_len++] = '\"'; break;
      case '\\': string_buf[str_len++] = '\\'; break;
      case 'n': string_buf[str_len++] = '\n'; break;
      case 'b': string_buf[str_len++] = '\b'; break;
      case 'f': string_buf[str_len++] = '\f'; break;
      case 't': string_buf[str_len++] = '\t'; break;
      case '0': string_buf[str_len++] = 0; 
                str_with_null = true;
                break;
      default:  string_buf[str_len++] = yytext[1];
                break;
    }
}


<string>\\\n  {
    curr_lineno++;
}

<string>\n    {
    curr_lineno++;
    strcpy(cool_yylval.error_msg, "Unterminated string constant");
    BEGIN(0);
    return (ERROR);
}

<string>\"  {
    if (str_len > 1 && str_with_null) {
        strcpy(cool_yylval.error_msg, "String contains null character");
        BEGIN(0);
        return (ERROR);
    }
    cool_yylval.symbol = stringtable.add_string(string_buf);
    BEGIN(0);
    return (STR_CONST);
}

<string>. {
    if (str_len >= MAX_STR_CONST) {
        strcpy(cool_yylval.error_msg, "String constant too long");
        BEGIN(0);
        return (ERROR);
    }
    string_buf[str_len++] = yytext[0];
}



{number}    {
    cool_yylval.symbol = inttable.add_string(yytext);
    return (INT_CONST);
}

{typeid}    {
    cool_yylval.symbol = idtable.add_string(yytext);
    return (TYPEID);
}

{objid}   {
    cool_yylval.symbol = idtable.add_string(yytext);
    return (OBJECTID);
}

.     {
    strcpy(cool_yylval.error_msg, yytext);
    return (ERROR);
}

%%
