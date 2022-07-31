#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//scanner
typedef enum
{
	START, INASSIGN, INCOMMENT, INNUM, INID, DONE
}
StateType;

typedef enum
{
    ELSE, IF, INT, RETURN, VOID, WHILE, ID, NUM, PLUS, MINUS, TIMES, OVER, LT, LTEQ, RT, RTEQ, EQ, NOT, ASSIGN, SEMI, COM, LPAREN, RPAREN, L1, R1, L2, R2, ENDFILE,ERROR
    //L1 =[ , L2={
} TokenType;

char tokenString[50];
char lineBuf[256];
int linepos = 0;
int bufsize = 0;
int EOF_flag = 0;
int lineno = 0;
FILE* file;
FILE* file2;
int assign_check = 0;
int Error = 0;
int cnt = 0;

//예약어 테이블
static struct
{
    char* str;
    TokenType tok;
} reservedWords[6]
= { {"else",ELSE},{"if",IF},{"int",INT},{"return",RETURN},{"void",VOID},{"while",WHILE} };

//parser
static TokenType token;
typedef enum { StmtK, ExpK} NodeKind;
typedef enum { CompoundK, IfK, WhileK,  ReturnK, CallK} StmtKind;
typedef enum { OpK, NumK, IdK, ArrayK, FunctionK} ExpKind;
typedef enum { Void, Integer, Boolean } ExpType;
typedef struct treeNode {
    struct treeNode* child[3];
    struct treeNode* sibling;
    int lineno; /*error*/
    NodeKind nodekind;
    union {
        StmtKind stmt;
        ExpKind exp;
    } kind;
    TokenType op;
    int val;
    char* name;
    ExpType type;
} TreeNode;

int getNextChar();
void ungetNextChar();
TokenType reservedLookup(char* s);
TokenType getToken();
void printToken(TokenType token, char* tokenString);
void syntaxError(char* message);
void match(TokenType expected);
TreeNode* declaration_list(void);
TreeNode* declaration(void);
TreeNode* var_declaration(void);
TreeNode* fun_declaration(void);
ExpType type_specifier(void);
TreeNode* params(void);
TreeNode* compound_stmt(void);
TreeNode* param_list(void);
TreeNode* param(void);
TreeNode* local_declarations(void);
TreeNode* statement_list(void);
TreeNode* statement(void);
TreeNode* expression_stmt(void);
TreeNode* selection_stmt(void);
TreeNode* iteration_stmt(void);
TreeNode* return_stmt(void);
TreeNode* expression(void);
TreeNode* var();
TreeNode* simple_expression(TreeNode* A);
TreeNode* additive_expression(TreeNode* A);
TreeNode* term(TreeNode* A);
TreeNode* factor(TreeNode* A);
TreeNode* call(void);
TreeNode* args(void);
TreeNode* args_list(void);
TreeNode* parse(void);
void printTree(TreeNode* tree);
//scanner 함수구현

int getNextChar()
{
	if (!(linepos < bufsize))
	{
		lineno++;
		if (fgets(lineBuf, sizeof(lineBuf), file))
		{
			bufsize = strlen(lineBuf);
			linepos = 0;
			return lineBuf[linepos++];
		}
		else
		{
			EOF_flag = 1;
			return EOF;
		}
	}
	else return lineBuf[linepos++];
}
void ungetNextChar()
{
	if (!EOF_flag) linepos--;
}
TokenType reservedLookup(char* s)
{
    int i;

    for (i = 0; i < 6; i++)
        if (!strcmp(s, reservedWords[i].str))
            return reservedWords[i].tok;
    return ID;
}
TokenType getToken()
{
    /* index for storing into tokenString */
    int tokenStringIndex = 0;
    TokenType currentToken;
    StateType state = START;
    int save;
    while (state != DONE)
    {
        int c = getNextChar();
        save = 1;
        switch (state)
        {
        case START:
            if (isdigit(c))
                state = INNUM;
            else if (isalpha(c))
                state = INID;
            else if ((c == ' ') || (c == '\t') || (c == '\n'))//공백
                save = 0;
            else if (c == '/')
            {
                save = 0;
                c = getNextChar();
                if (c == '*')
                {
                    while (1)
                    {//INCOMMENT
                        c = getNextChar();
                        if (c == '*')
                        {
                            c = getNextChar();
                            if (c == '/')
                            {
                                //코멘트 끝, 새 토큰 받으러 감.
                                break;
                            }
                        }
                        else if (c == EOF)
                        {
                            state = DONE;
                            currentToken = ENDFILE;
                        }
                    }
                }
                else
                {
                    ungetNextChar();
                    save = 1;
                    state = DONE;
                    currentToken = OVER;
                }
                break;
            }
            else if (c == '!')
            {
                save = 0;
                state = DONE;
                c = getNextChar();
                if (c == '=')
                {
                    tokenString[tokenStringIndex++] = '!';
                    tokenString[tokenStringIndex++] = '=';
                    tokenStringIndex++;
                    currentToken = NOT;
                }
                else
                {
                    ungetNextChar();
                    save = 0;
                    currentToken = ERROR;
                }
            }
            else if (c == '<')
            {
                save = 0;
                state = DONE;
                c = getNextChar();
                if (c == '=')
                {
                    tokenString[tokenStringIndex++] = '<';
                    tokenString[tokenStringIndex++] = '=';
                    currentToken = LTEQ;
                }
                else
                {
                    tokenString[0] = '<';
                    ungetNextChar();
                    currentToken = LT;
                }
                break;
            }
            else if (c == '>')
            {
                save = 0;
                state = DONE;
                c = getNextChar();
                if (c == '=')
                {
                    tokenString[tokenStringIndex++] = '>';
                    tokenString[tokenStringIndex++] = '=';
                    currentToken = RTEQ;
                }
                else
                {
                    tokenString[tokenStringIndex++] = '>';
                    ungetNextChar();
                    currentToken = RT;
                }
                break;
            }
            else if (c == '=')
            {
                save = 0;
                state = DONE;
                c = getNextChar();
                if (c == '=')
                {
                    tokenString[tokenStringIndex++] = '=';
                    tokenString[tokenStringIndex++] = '=';
                    currentToken = EQ;
                }
                else
                {
                    tokenString[tokenStringIndex++] = '=';
                    ungetNextChar();
                    currentToken = ASSIGN;
                }
                break;
            }
            else
            {
                state = DONE;
                switch (c)
                {
                case '+':
                    currentToken = PLUS;
                    break;
                case '-':
                    currentToken = MINUS;
                    break;
                case '*':
                    currentToken = TIMES;
                    break;
                case ';':
                    currentToken = SEMI;
                    break;
                case ',':
                    currentToken = COM;
                    break;
                case '(':
                    currentToken = LPAREN;
                    break;
                case ')':
                    currentToken = RPAREN;
                    break;
                case '[':
                    currentToken = L1;
                    break;
                case ']':
                    currentToken = R1;
                    break;
                case '{':
                    currentToken = L2;
                    break;
                case '}':
                    currentToken = R2;
                    break;
                case EOF:
                    save = 0;
                    currentToken = ENDFILE;
                    break;
                default:
                    currentToken = ERROR;
                    break;
                }
            }
            break;
        case INNUM:
            if (!isdigit(c))
            { /* backup in the input */
                ungetNextChar();
                save = 0;
                state = DONE;
                currentToken = NUM;
            }
            break;
        case INID:
            if (!isalpha(c))
            {   ungetNextChar();
                save = 0;
                state = DONE;
                currentToken = ID;
            }
            break;
        case DONE:
        default: /* should never happen */
            state = DONE;
            currentToken = ERROR;
            break;
        }
        if ((save) && (tokenStringIndex <= sizeof(tokenString)))
            tokenString[tokenStringIndex++] = (char)c;
        if (state == DONE)
        {
            tokenString[tokenStringIndex] = '\0';
            if (currentToken == ID)
                currentToken = reservedLookup(tokenString);
        }
   }
   return currentToken;
}
//parser 함수구현
void printToken(TokenType token, char* tokenString)
{
    switch (token)
    {
    case ELSE:
        fprintf(file2, "else\n");
        break;
    case IF:
        fprintf(file2, "if\n");
        break;
    case INT:
        fprintf(file2, "int\n");
        break;
    case RETURN:
        fprintf(file2, "return\n");
        break;
    case VOID:
        fprintf(file2, "void\n");
        break;
    case WHILE:
        fprintf(file2, "while\n");
        break;
    case ID:
        fprintf(file2, "ID: %s\n", tokenString);
        break;
    case NUM:
        fprintf(file2, "NUM: %s\n", tokenString);
        break;
    case PLUS:
        fprintf(file2, "+\n");
        break;
    case MINUS:
        fprintf(file2, "-\n");
        break;
    case TIMES:
        fprintf(file2, "*\n");
        break;
    case OVER:
        fprintf(file2, "/\n");
        break;
    case LT:
        fprintf(file2, "<\n");
        break;
    case LTEQ:
        fprintf(file2, "<=\n");
        break;
    case RT:
        fprintf(file2, ">\n");
        break;
    case RTEQ:
        fprintf(file2, ">=\n");
        break;
    case EQ:
        fprintf(file2, "==\n");
        break;
    case NOT:
        fprintf(file2, "!=\n");
        break;
    case ASSIGN:
        fprintf(file2, "=\n");
        break;
    case SEMI:
        fprintf(file2, ";\n");
        break;
    case COM:
        fprintf(file2, ",\n");
        break;
    case LPAREN:
        fprintf(file2, "(\n");
        break;
    case RPAREN:
        fprintf(file2, ")\n");
        break;
    case L1:
        fprintf(file2, "[\n");
        break;
    case R1:
        fprintf(file2, "]\n");
        break;
    case L2:
        fprintf(file2, "{\n");
        break;
    case R2:
        fprintf(file2, "}\n");
        break;
    case ENDFILE:
        fprintf(file2, "EOF\n");
        break;
    case ERROR:
        fprintf(file2, "ERROR: %s\n", tokenString);
        break;
    }
}
TreeNode* newStmtNode(StmtKind kind)
{
    TreeNode* new;
    new = (TreeNode*)malloc(sizeof(TreeNode));
    for (int i = 0; i < 3; ++i)
        new->child[i] = NULL;
    new->sibling = NULL;
    new->lineno = lineno;
    new->nodekind = StmtK;
    new->kind.stmt = kind;
    /*TokenType op;
    int val;
    char* name;
    ExpType type;*/
}
TreeNode* newExpNode(ExpKind kind)
{
    TreeNode* new;
    new = (TreeNode*)malloc(sizeof(TreeNode));
    for (int i = 0; i < 3; ++i)
        new->child[i] = NULL;
    new->sibling = NULL;
    new->lineno = lineno;
    new->nodekind = ExpK;
    new->kind.exp = kind;
    /*TokenType op;
    int val;
    char* name;
    sExpType type;*/
}
void syntaxError(char* message)
{
    fprintf(file2, "\n>>> ");
    fprintf(file2, "Syntax error at line %d: %s", lineno, message);
    Error = 1;
}
void match(TokenType expected)
{
    if (token == expected)
        token = getToken();
    else {
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
    }
}
TreeNode* declaration_list(void)
{
    //declaration-list —> declaration{declaration}
    TreeNode* t = declaration();
    TreeNode* p = t;
    TreeNode* q = NULL;

    while (token != ENDFILE)
    {
        q = declaration();
        if (p != NULL)
        {
            p->sibling = q;
            p = q;
        }
    }
    return t;
}
TreeNode* declaration(void)
{
    //var or fun -> var은 decl함수내 자체구현
    TreeNode* t = NULL;
    ExpType type_name = type_specifier();
    //char *id_name = tokenString;
    token = getToken();
    char* id_name;
    int n = strlen(tokenString) + 1;
    id_name = (char*)malloc(n * sizeof(char));
    strcpy(id_name, tokenString);
    match(ID);

    if (token == SEMI)
    {
        t = newExpNode(IdK);
        t->type = type_name;
        t->name = id_name;
        match(SEMI);
    }
    else if (token == L1)//배열
    {
        t = newExpNode(ArrayK);
        t->type = type_name;
        t->name = id_name;
        match(L1);
        t->val = atoi(tokenString);
        match(NUM);
        match(R1);
        match(SEMI);
    }
    else if (token == LPAREN)//함수
    {
        t = fun_declaration();
        if (t != NULL)
        {
            t->type = type_name;
            t->name = id_name;
        }
    }
    else
    {
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        token = getToken();
    }
    return t;
}
TreeNode* var_declaration(void)
{
    TreeNode* t = NULL;
    ExpType type_name = type_specifier();
    token = getToken();
    char* id_name;
    int n = strlen(tokenString) + 1;
    id_name = (char*)malloc(n * sizeof(char));
    strcpy(id_name, tokenString);

    match(ID);

    if (token == SEMI)
    {
        t = newExpNode(IdK);
        t->type = type_name;
        t->name = id_name;
        match(SEMI);
    }
    else if (token == L1)//배열
    {
        t = newExpNode(ArrayK);
        t->type = type_name;
        t->name = id_name;
        match(L1);
        t->val = atoi(tokenString);
        match(NUM);
        match(R1);
        match(SEMI);
    }
    else
    {
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        token = getToken();
    }
    return t;
}
TreeNode* fun_declaration()
{
    TreeNode* t = NULL;

    if (token == LPAREN)
    {
        t = newExpNode(FunctionK);
        match(LPAREN);
        t->child[0] = params();
        match(RPAREN);
        t->child[1] = compound_stmt();
    }
    else
    {
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        token = getToken();
    }
    return t;

}
ExpType type_specifier(void)
{
    ExpType type;
    if (token == INT)
        type = Integer;
    else if (token == VOID)
        type = Void;
    else
    {
        syntaxError("unexpected token->");
        printToken(token, tokenString);
        token = getToken();
        while ((token != INT) && (token != VOID))
        {
            syntaxError("unexpected token->");
            printToken(token, tokenString);
            token = getToken();
            if (token == ENDFILE)
            {
                syntaxError("unexpected token->");
                printToken(token, tokenString);
                break;
            }
        }
        if (token == INT)
            type = Integer;
        else if (token == VOID)
            type = Void;

    }
    return type;
}
TreeNode* params(void)
{
    TreeNode* t = NULL;

    if (token == VOID)
    {
        match(VOID);
        t = newExpNode(IdK);
        t->name = " ";
        t->type = Void;
    }
    else
        t = param_list();
    return t;
}
TreeNode* compound_stmt(void)
{
    TreeNode* t = NULL;

    match(L2);
    t = newStmtNode(CompoundK);
    if ((token == INT) || (token == VOID))
        t->child[0] = local_declarations();
    if (token != R2)
        t->child[1] = statement_list();
    match(R2);
    return t;
}
TreeNode* param_list(void)
{
    //param-list —>param{,param}
    TreeNode* t = param();
    TreeNode* p = t;
    TreeNode* q = NULL;
    while ((p != NULL) && (token == COM))
    {
        match(COM);
        q = param();
        p->sibling = q;
        p = q;
    }
    return t;
}
TreeNode* param(void)
{
    TreeNode* t = NULL;
    ExpType type_name = type_specifier();
    char* id_name;
    token = getToken();
    int n = strlen(tokenString) + 1;
    id_name = (char*)malloc(n * sizeof(char));

    strcpy(id_name, tokenString);

    match(ID);

    if (token == L1)//배열
    {
        match(L1);
        match(R1);
        t = newExpNode(ArrayK);
        t->name = id_name;
        t->type = type_name;
        t->val = 0;
    }
    else
    {
        t = newExpNode(IdK);
        t->name = id_name;
        t->type = type_name;
    }
    return t;
}
TreeNode* local_declarations(void)
{
    //local-declarations—>empty[{var-declaration}]
    TreeNode* t = NULL;
    TreeNode* p = NULL;
    TreeNode* q = NULL;

    if ((token == INT) || (token == VOID))
        t = var_declaration();
    if (t != NULL)
    {
        p = t;

        while ((token == INT) || (token == VOID))
        {
            q = var_declaration();
            if (p != NULL)
            {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}
TreeNode* statement_list(void)
{
    //statement-list —>empty[{statement}]
    TreeNode* t = statement();
    TreeNode* p = t;
    TreeNode* q = NULL;

    if (token != ENDFILE)
    {
        while (token != R2)
        {
            q = statement();
            if (p != NULL)
            {
                p->sibling = q;
                p = q;
            }
        }
    }
    return t;
}
TreeNode* statement(void)
{
    TreeNode* t = NULL;

    switch (token)
    {
    case LPAREN:
        t = expression_stmt();
        break;
    case ID:
        t = expression_stmt();
        break;
    case NUM:
        t = expression_stmt();
        break;
    case SEMI:
        t = expression_stmt();
        break;
    case L2:
        t = compound_stmt();
        break;
    case IF:
        t = selection_stmt();
        break;
    case WHILE:
        t = iteration_stmt();
        break;
    case RETURN:
        t = return_stmt();
        break;
    default:
        syntaxError("unexpected token->");
        printToken(token, tokenString);
        token = getToken();
        break;
    }
    return t;
}
TreeNode* expression_stmt(void)
{
    assign_check = 0;
    TreeNode* t = NULL;

    if(token!=SEMI)
        t = expression();
    match(SEMI);
    return t;
}
TreeNode* selection_stmt(void)
{
    //selection-stmt —> if ( expression ) statement[else statement]
    TreeNode* t = newStmtNode(IfK);

    match(IF);
    match(LPAREN);
    t->child[0] = expression();
    match(RPAREN);
    t->child[1] = statement();

    if (token == ELSE)
    {
        match(ELSE);
        t->child[2] = statement();
    }
    return t;
}
TreeNode* iteration_stmt(void)
{
    TreeNode* t = newStmtNode(WhileK);
    match(WHILE);
    match(LPAREN);
    t->child[0] = expression();
    match(RPAREN);
    t->child[1] = statement();

    return t;
}
TreeNode* return_stmt(void)
{
    TreeNode* t = newStmtNode(ReturnK);

    match(RETURN);
    if (token != SEMI)
        t->child[0] = expression();
    match(SEMI);
    return t;
}
TreeNode* expression(void)
{
    //var=expression | simple-expression
    //= 나 [ 나오면 첫번째. var으로
    //아니면 simple-expression
    TreeNode* t = NULL;
    TreeNode* p = NULL;
    TreeNode* q = NULL;
    if (token == ID)
        p = var();
    if (token == ASSIGN)
    {
        assign_check = 1;
        t = newExpNode(OpK);
        t->op = token;
        match(ASSIGN);
        q = expression();
        t->child[0] = p;
        t->child[1] = q;
    }
    else
        t = simple_expression(p);
        /*if (assign_check)
        {
            t = p;
        }*/
    return t;
}
TreeNode* var()
{
    TreeNode* t = NULL;
    TreeNode* p = NULL;

    char* id_name;
    int n = strlen(tokenString) + 1;
    id_name = (char*)malloc(n * sizeof(char));
    strcpy(id_name, tokenString);

    match(ID);

    if (token == L1)//배열
    {
        match(L1);
        p = expression();
        match(R1);
        t= newExpNode(ArrayK);
    }
    else if (token == LPAREN) //call
    {
        match(LPAREN);
        p = args();
        match(RPAREN);

        t = newStmtNode(CallK);
    }
    else
        t = newExpNode(IdK);

    t->child[0] = p;
    t->name = id_name;
    return t;
}
TreeNode* simple_expression(TreeNode* A)
{
    //simple_ex->additive_ex{relop additive_ex}
    TreeNode* t = additive_expression(A);
    TreeNode* temp = NULL;
    if ((token == LTEQ) || (token == LT) || (token == RTEQ) || (token == RT) || (token == EQ) || (token == NOT))
    {
        TreeNode* p = newExpNode(OpK);
        p->op = token;
        p->child[0] = t;
        t = p;
        match(token);
        t->child[1] = additive_expression(temp);
    }
    return t;
}
TreeNode* additive_expression(TreeNode* A)
{
    //additive_ex->term{addop term}
    TreeNode* t = term(A);
    TreeNode* temp = NULL;
    while ((token == PLUS) || (token == MINUS))
    {
        TreeNode* p = newExpNode(OpK);
        p->child[0] = t;
        p->op = token;
        t = p;
        match(token);
        t->child[1] = term(temp);

    }
    return t;
}
TreeNode* term(TreeNode* A)
{
    TreeNode* t = factor(A);
    TreeNode* temp = NULL;
    while ((token == TIMES) || (token == OVER))
    {
        TreeNode* p = newExpNode(OpK);
        p->child[0] = t;
        p->op = token;
        t = p;
        match(token);
        p->child[1] = factor(temp);
    }
    return t;
}
TreeNode* factor(TreeNode* A)
{
    //말단까지 내려주고 return
    //factor —> ( expression ) | var | call | NOM
    TreeNode* t = NULL;
    if (A != NULL)
        return A;

    if (token == LPAREN)
    {
        match(LPAREN);
        t = expression();
        match(RPAREN);
    }
    else if (token == ID)
    {
        char* id_name;
        int n = strlen(tokenString) + 1;
        id_name = (char*)malloc(n * sizeof(char));
        strcpy(id_name, tokenString);

        TokenType tokenB = getToken();
        ungetNextChar();//되돌려놓기.

        //tokenString이 변경된 상태이므로..
        strcpy(tokenString, id_name);
        if (tokenB == LPAREN)
            t = call();
        else
            t = var();
    }
    else if (token == NUM)
    {
        t = newExpNode(NumK);
        if (token == NUM)
            t->val = atoi(tokenString);
        match(NUM);
    }
    else
    {
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        token = getToken();
    }
    return t;
}
TreeNode* call(void)
{
    TreeNode* t = NULL;
    TreeNode* p = NULL;
    char* id_name = "\0";

    if (token == ID)
        strcpy(id_name, tokenString);
    match(ID);

    if (token == LPAREN)
    {
        match(LPAREN);
        match(RPAREN);
        p = args();

        t = newStmtNode(CallK);
        t->child[0] = p;
        t->name = id_name;
    }
    else {
        syntaxError("unexpected token -> ");
        printToken(token, tokenString);
        token = getToken();
    }
    return t;
}
TreeNode* args(void)
{
    TreeNode* t = NULL;
    if (token != RPAREN)//not empty
        t = args_list();
    return t;
}
TreeNode* args_list(void)
{
    TreeNode* t = expression();
    TreeNode* p = t;
    TreeNode* q;

    while (token == COM)
    {
        match(COM);
        q = expression();
        if (p != NULL)
        {
            p->sibling = q;
            p = q;
        }
    }
    return t;
}
TreeNode* parse(void)
{
    TreeNode* t;
    token = getToken();
    t = declaration_list();
    if (token != ENDFILE)
        syntaxError("Code ends before file\n");
    return t;
}
void printTree(TreeNode* tree)
{
    cnt++;
    while (tree != NULL)
    {
        for (int i = 0; i < cnt; i++)
        {
            fprintf(file2, "      ");
        }
        char* temp = "T"; //test용
        if (tree->nodekind == ExpK)
        {
            switch (tree->kind.exp)
            {
            case OpK:
                fprintf(file2, "Op: ");
                switch (tree->op)
                {
                case PLUS:
                    fprintf(file2, "+\n");
                    break;
                case MINUS:
                    fprintf(file2, "-\n");
                    break;
                case TIMES:
                    fprintf(file2, "*\n");
                    break;
                case OVER:
                    fprintf(file2, "/\n");
                    break;
                case LT:
                    fprintf(file2, "<\n");
                    break;
                case LTEQ:
                    fprintf(file2, "<=\n");
                    break;
                case RT:
                    fprintf(file2, ">\n");
                    break;
                case RTEQ:
                    fprintf(file2, ">=\n");
                    break;
                case EQ:
                    fprintf(file2, "==\n");
                    break;
                case NOT:
                    fprintf(file2, "!=\n");
                    break;
                case ASSIGN:
                    fprintf(file2, "=\n");
                    break;
                case SEMI:
                    fprintf(file2, ";\n");
                    break;
                case COM:
                    fprintf(file2, ",\n");
                    break;
                case LPAREN:
                    fprintf(file2, "(\n");
                    break;
                case RPAREN:
                    fprintf(file2, ")\n");
                    break;
                case L1:
                    fprintf(file2, "[\n");
                    break;
                case R1:
                    fprintf(file2, "]\n");
                    break;
                case L2:
                    fprintf(file2, "{\n");
                    break;
                case R2:
                    fprintf(file2, "}\n");
                    break;
                }
                break;
            case IdK:
                if (tree->type == Integer)
                    temp = "int";
                else if (tree->type == Void)
                    temp = "Void";
                if (!(temp == "Void"))
                {
                    if (!(temp == "T")) {
                        fprintf(file2, "Id: %s(type: %s)\n", tree->name, temp);
                    }
                    else
                        fprintf(file2, "Id: %s\n", tree->name);
                }
                else
                    fprintf(file2, "\n"); //(void) 같은 경우..\n
                break;
            case NumK:
                fprintf(file2, "num: %d\n", tree->val);
                break;
            case ArrayK:
                if (tree->type == Integer)
                    fprintf(file2, "array: %s(size: %d,type: int)\n", tree->name, tree->val);
                else if (tree->type == Void)
                    fprintf(file2, "array: %s(size: %d,type: Void)\n", tree->name, tree->val);
                else//선언이 아닌 배열일 때. type 없음
                {
                    fprintf(file2, "array: %s\n", tree->name);
                    //그후, child에 있는 exp를 출력하면 됨.
                }
            
                break;
            case FunctionK:
                if (tree->type == Integer)
                    temp = "int";
                else if (tree->type == Void)
                    temp = "Void";
                fprintf(file2, "function: %s(type: %s)\n", tree->name, temp);
                break;
            default:
                fprintf(file2, "error\n");
                break;
            }
        }
        else if (tree->nodekind == StmtK)
        {
            switch (tree->kind.stmt)
            {
            case CompoundK:
                fprintf(file2, "Compound: \n");
                break;
            case IfK:
                fprintf(file2, "If: \n");
                break;
            case WhileK:
                fprintf(file2, "While: \n");
                break;
            case ReturnK:
                fprintf(file2, "Return: \n");
                break;
            case CallK:
                fprintf(file2, "Call: %s\n", tree->name);
                break;
            default:
                fprintf(file2, "error\n");
                break;
            }
        }
        for (int i = 0; i < 3; ++i)
            printTree(tree->child[i]);
        tree = tree->sibling;
    }
    cnt--;
}

int main()
{
    printf("parse ");
    char filename[300] = "\0";
    char filename2[300] = "\0";
    scanf("%s %s", filename, filename2);
    file = fopen(filename, "r");
    file2 = fopen(filename2, "w");

    TreeNode* tree = NULL;
    tree = parse();
    printTree(tree);
    fclose(file);
    fclose(file2);
}