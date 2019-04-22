#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define NO_KEYWORDS 5
#define ID_LENGTH 12

enum tsymbol {tnull = -1,
        tident,	tnumber, tlparen,	trparen, tmul, tplus,
//        0       1         2       3       4      5
        tcomma,	tminus, tdiv, tsemicolon, tassign, teof,
//        6       7      8        9         10      11
        //	......	 word symbols ..................................... //
        tpakage, tis, tbegin, tend, twrite
//        12      13    14     15     16
};

struct tokenType {
    int number;		// token number
    char paren;
    char* string;

    union {
        char id[ID_LENGTH];
        int num;
    } value;		// token value
};

char *keyword[NO_KEYWORDS] = {
        "package", "is", "begin", "end", "write"
};

enum tsymbol tnum[NO_KEYWORDS] = {
        tpakage, tis, tbegin, tend, twrite
};

void lexicalError(int n)
{
  printf(" *** Lexical Error : ");

  switch (n)	{
    case 1 : printf("an identifier length must be less than 12.\n");
      break;
    case 2 : printf("next character must be &.\n");
      break;
    case 3 : printf("next character must be |.\n");
      break;
    case 4 : printf("invalid character!!!\n");
      break;
    default:
      break;
  }
}

int superLetter(char ch)
{
  if(isalpha(ch) || ch == '_') return 1;
  else return 0;
}
int superLetterOrDigit(char ch)
{
  if (isalnum(ch) || ch == '_') return 1;
  else return 0;
}
int hexValue(char ch)
{
  switch (ch) {
    case '0' : case '1' : case '2' : case '3' : case '4' :
    case '5' : case '6' : case '7' : case '8' : case '9' :
      return (ch - '0');
    case 'A' : case 'B' : case 'C' : case 'D' : case 'E' : case 'F' :
      return (ch - 'A' + 10);
    case 'a' : case 'b' : case 'c' : case 'd' : case 'e' : case 'f' :
      return (ch - 'a' + 10);
    default : return -1;
  }
}

int getIntNum(char firstCharacter, FILE* source_file)
{
  int num = 0;
  int value;
  char ch;
  if (firstCharacter != '0') {			// decimal
    ch = firstCharacter;
    do {
      num = 10*num + (int)(ch - '0');
      ch = fgetc(source_file);
    } while (isdigit(ch));
  } else {
    ch = fgetc(source_file);
    if ((ch >= '0') && (ch <= '7'))		// octal
      do {
        num = 8*num + (int)(ch - '0');
        ch = fgetc(source_file);
      } while ((ch >= '0') && (ch <= '7'));
    else if ((ch == 'X') || (ch == 'x')) {	// hexa decimal
      while ((value = hexValue(ch=fgetc(source_file))) != -1)
        num = 16 * num + value;
    }
    else num = 0;			// zero
  }
  ungetc(ch, source_file);	// retract
  return num;
}

struct tokenType scanner(FILE* source_file) {
  struct tokenType token;
  int i, index;
  char ch, id[ID_LENGTH];
  token.number = tnull;

  do {
    while (isspace(ch = fgetc(source_file))); // 공백은 뛰어 넘기자.

    if (superLetter(ch)) { // 식별자나 키워드가 있다면,
      i=0;

      do {
        if (i<ID_LENGTH) id[i++] = ch;
        ch = fgetc(source_file);
      } while (superLetterOrDigit(ch));

      if (i>=ID_LENGTH) lexicalError(1);

      id[i] = '\0';
      ungetc(ch, source_file); // retract

      // find the identifier in the keyword table
      for (index=0; index < NO_KEYWORDS; index++)
        if (!strcmp(id, keyword[index])) break;

      if (index < NO_KEYWORDS)	// fount, keyword exit
      {
        token.number = tnum[index];
        token.string = keyword[index];
      }
      else {			// not found, identifier exit
        token.number = tident;  // state 0
        strcpy(token.value.id, id);
      }
    } // end of identifier or keyword
    else if (isdigit(ch)) {			// integer constant
      token.number = tnumber;   // state 1
      token.value.num = getIntNum(ch, source_file);
    }
    else switch (ch) {				// special character
        case '/' :					// state 8
          ch = fgetc(source_file);
          token.number = tdiv;
          token.paren = '/';
          ungetc(ch, source_file); // retract
          break;
        case '*' :					// state 4
          ch = fgetc(source_file);
          token.number = tmul;
          token.paren = '*';
          ungetc(ch, source_file);	// retract
          break;
        case '+' :					// state 5
          ch = fgetc(source_file);
          token.number = tplus;
          token.paren = '+';
          ungetc(ch, source_file);	// retract
          break;
        case '-' :					// stats 7
          ch = fgetc(source_file);
          if (ch == '-')
            while (fgetc(source_file) != '\n');
          else {
            token.number = tminus;
            token.paren = '-';
            ungetc(ch, source_file);	// retract
          }
          break;
        case ':' :					// state 10
          ch = fgetc(source_file);
          if (ch == '=') {
            token.number = tassign;
            token.string = ":=";
          } else {
            ungetc(ch, source_file);	// retract
          }
          break;
        case '(' :
          token.number = tlparen;
          token.paren = '(';
          break;  // state 2
        case ')' :
          token.number = trparen;
          token.paren = ')';
          break;  // state 3
        case ',' :
          token.number = tcomma;
          token.paren = ',';
          break;  // state 6
        case ';' :
          token.number = tsemicolon;
          token.paren = ';';
          break;  // state 9
        case EOF :
          token.number = teof;
          break;  // state 11
        default : {
          printf("Current character : %c", ch);
          lexicalError(4);
          break;
        }
      } // switch end
  } while (token.number == tnull);

  return token;
} // end of scanner

int main(int argc, char *argv[])
{
  FILE *source_file;
  struct tokenType token;

  if (argc < 2) {
    fprintf(stderr, "Usage : scanner <source file name>\n");
    exit(1);
  }

  if((source_file = fopen(argv[1], "r")) == NULL) {
    fprintf(stderr, "%s file not found \n", argv[1]);
    exit(-1);
  }

  do {
    token = scanner(source_file);
    fprintf(stdout, "Token ---> ");

    switch (token.number) {
      case 0 :
        fprintf(stdout, ": (%d, '%s')\n", token.number, token.value.id);
        break;
      case 1 :
        fprintf(stdout, ": (%d, %d)\n", token.number, token.value.num);
        break;
      case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
        fprintf(stdout, ": (%d, '%c')\n", token.number, token.paren);
        break;
      case 10:
        fprintf(stdout, ": (%d, '%s')\n", token.number, token.string);
        break;
      case 11:
        fprintf(stdout, ": (%d, 'exit')\n", token.number);
        break;
      case 12: case 13: case 14: case 15: case 16:
        fprintf(stdout, ": (%d, '%s')\n", token.number, token.string);
        break;
    }
  } while (!feof(source_file));

  fclose(source_file);
}
