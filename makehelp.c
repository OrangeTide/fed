/*
 *      makehelp utility for FED 2.0
 *
 *      By Shawn Hargreaves, 1994
 *
 *      This tool converts a text file into a C source file containing
 *      an array of char pointers. Used to generate help.c from help.txt
 *
 *      Usage: makehelp <infile> <outfile>
 */


#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

char *strltrim(const char *s)
{
  const char *p = s;

  while (p[0] && p[1] && isspace(*p))
     p++;
  return (char*)p;
}


int main(int argc, char *argv[])
{
   FILE *in, *out;
   char buf[256];
   char *buf2;
   int len;
   int c;
   int line = 1;

   if (argc != 3) {
      printf("\nUsage: makehelp <infile> <outfile>\n");
      return 1;
   }

   in = fopen(argv[1], "rb");
   if (!in) {
      printf("\nError opening %s\n", argv[1]);
      return 1;
   }

   out = fopen(argv[2], "w");
   if (!in) {
      fclose(in);
      printf("\nError opening %s\n", argv[2]);
      return 1;
   }

   fprintf(out, "/* output from the makehelp utility program */\n\n");
   fprintf(out, "#include \"fed.h\"\n\n");
   fprintf(out, "char *help_text[] = \n{\n");

   len = 0;

   while (!feof(in)) {
      c = getc(in);

      if (ferror(in)) {
         printf("%s: file I/O error!\n", argv[1]);
         return 1;
      }

      if (len + 1 >= sizeof(buf)) {
         printf("%s:%d: line length too long\n", argv[1], line);
         return 1;
      }

      if ((c=='\r') || (c=='\n')) {
	 if (c=='\r') {
	    c = getc(in);
	    if (c != '\n')
	       ungetc(c, in);
	 }
	 buf[len] = 0;
	 len = 0;
	 buf2 = strltrim(buf);
	 if (*buf2 == '#')
	    fprintf(out, "%s\n", buf2);
	 else
	    fprintf(out, "   \"%s\",\n", buf);
	 line++;
      }
      else if (c=='\t') {
	 do {
	    buf[len++] = ' ';
	 } while (len != (len&0xfff8));
      }
      else if (c=='"') {
	 buf[len++] = '\\';
	 buf[len++] = '"';
      }
      else if (c=='\\') {
	 buf[len++] = c;
	 c = getc(in);
	 if (c=='@') {
	    buf[len-1] = '"';
	 }
	 else {
	    if ((c != '\\') && (c != 'r') && (c != 'n') && (c != 't')) {
	       printf("%s:%d: Warning: \\ will be interpreted as a control code\n", argv[1], line);
	       ungetc(c, in);
	    }
	    else
	       buf[len++] = c;
	 }
      }
      else {
	 if (c==0)
	    printf("%s:%d: Warning: NULL will be interpreted as end of line\n", argv[1], line);
	 buf[len++] = c;
      }
   }

   fprintf(out, "   NULL,\n};\n\n");

   fclose(in);
   fclose(out);
   return 0;
}
