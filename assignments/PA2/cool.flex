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
#define append_str(c) \
        *(string_buf_ptr++) = c; \ 
        if (string_buf + MAX_STR_CONST == string_buf_ptr) { BEGIN(string_long); cool_yylval.error_msg = "String constant too long"; return ERROR;}
int comment_level;

%}

/*
 * Define names for regular expressions here.
 */

DARROW      =>
ASSIGN      <-
LE          <=  

number      [0-9]+
typeid      [A-Z][A-Za-z0-9_]*
objid       [a-z][A-Za-z0-9_]*
newline     \n
whitespace  [ \t\v\r\f]+

%x comment1 comment2 
%x string string_null string_long string_escape

%%


{newline}     {curr_lineno++;}
{whitespace}  { /*Skip the white spaces*/ }


"--"    {BEGIN(comment1);}
<comment1>\n   {
    BEGIN(0); 
    curr_lineno++;
}
<comment1>.   { /*skip the comments started with -- */ }

"(*"              {BEGIN(comment2); comment_level=1;}
<comment2>"(*"    {comment_level++;}
<comment2>\n      {curr_lineno++;}
<comment2><<EOF>> {
    BEGIN(0);
    cool_yylval.error_msg = "EOF in comment";
    return (ERROR);
}
<comment2>.   { /*skip the comments in (*...*) */ }
<comment2>"*)"   {
    comment_level--;
    if (comment_level == 0) BEGIN(0);
}
"*)"    {
    cool_yylval.error_msg = "Unmatched *)";
    return (ERROR);
}

 /*
  *  The multiple-character operators.
  */
{DARROW}    {return (DARROW);}
{ASSIGN}    {return (ASSIGN);}
{LE}        {return (LE);}


"{"			{ return '{'; }
"}"			{ return '}'; }
"("			{ return '('; }
")"			{ return ')'; }
"~"			{ return '~'; }
","			{ return ','; }
";"			{ return ';'; }
":"			{ return ':'; }
"+"			{ return '+'; }
"-"			{ return '-'; }
"*"			{ return '*'; }
"/"			{ return '/'; }
"."			{ return '.'; }
"<"			{ return '<'; }
"="			{ return '='; }
"@"			{ return '@'; }

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

t(?i:rue)   {
      cool_yylval.boolean = 1;
      return (BOOL_CONST);
}

f(?i:alse)   {
      cool_yylval.boolean = 0;
      return (BOOL_CONST);
}


 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */

\"              { BEGIN(string); string_buf_ptr = string_buf;}
<string>\"      { BEGIN(0); 
                  cool_yylval.symbol = stringtable.add_string(string_buf, string_buf_ptr - string_buf); 
                  return STR_CONST;}
<string><<EOF>> { BEGIN(0); cool_yylval.error_msg = "EOF in string constant";
                  return ERROR;}
<string>\n      { BEGIN(0);
                  cool_yylval.error_msg = "Unterminated string constant";
                  curr_lineno++;
                  return ERROR; }
<string>\0      { BEGIN(string_null);
                  cool_yylval.error_msg = "String contains null character";
                  return ERROR; }
<string>\\0     { append_str('0');}
<string>\\n     { append_str('\n');}
<string>\\t     { append_str('\t');}
<string>\\b     { append_str('\b');}
<string>\\f     { append_str('\f');}
<string>\\      { BEGIN(string_escape);}
<string>.       { append_str(yytext[0]);}

<string_escape><<EOF>>  {BEGIN(0); cool_yylval.error_msg = "EOF in string constant"; return ERROR;}
<string_escape>\0   {BEGIN(string_null); cool_yylval.error_msg = "String contains null character"; return ERROR;}
<string_escape>\n   {BEGIN(string); append_str('\n'); curr_lineno++; }
<string_escape>.    {BEGIN(string); append_str(yytext[0]);}

<string_null>\"     {BEGIN(0);}
<string_null>\n     {BEGIN(0); curr_lineno++;}
<string_null>.

<string_long>\"     {BEGIN(0);}
<string_long>\n     {BEGIN(0); curr_lineno++;}
<string_long>.

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

.   {cool_yylval.error_msg = yytext; return ERROR; }

%%
