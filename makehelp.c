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



int main(int argc, char *argv[])
{
   FILE *in, *out;
   char buf[256];
   int len;
   int c;
   int line = 1;

   if (argc != 3) {
      printf("\nUsage: makehelp <infile> <outfile>\n");
      return -1;
   }

   in = fopen(argv[1], "rb");
   if (!in) {
      printf("\nError opening %s\n", argv[1]);
      return -1;
   }

   out = fopen(argv[2], "w");
   if (!in) {
      fclose(in);
      printf("\nError opening %s\n", argv[2]);
      return -1;
   }

   fprintf(out, "/* output from the makehelp utility program */\n\n");
   fprintf(out, "#include \"fed.h\"\n\n");
   fprintf(out, "char *help_text[] = \n{\n");

   len = 0;

   while (!feof(in)) {
      c = getc(in);

      if ((c=='\r') || (c=='\n')) {
	 if (c=='\r') {
	    c = getc(in);
	    if (c != '\n')
	       ungetc(c, in);
	 }
	 buf[len] = 0;
	 len = 0;
	 if ((buf[0] == '#') || ((buf[0] == ' ') && (buf[1] == '#')))
	    fprintf(out, "%s\n", buf);
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
	    printf("%s%d: Warning: NULL will be interpreted as end of line\n", argv[1], line);
	 buf[len++] = c;
      }
   }

   fprintf(out, "   NULL,\n};\n\n");

   fclose(in);
   fclose(out);
   return 0;
}
