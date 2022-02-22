/*
  :::Name:::
  _ccgen_ is a universal frontend utility.

  :::Synopsis:::
  ccgen [-l logfile]
        [-x backend]
        [-b outfile_base]
	[-e extension]
	[-o option_spec]... [args]...

  ccgen -h

  ccgen -v

  :::Description:::
  _ccgen_ is some type of a frontend to underlying customizable backend.
  It is particularly useful to use a compiler as a backend,
  effectively generating a lot of files, each of which will be compiled with different options.

  You pass options to _ccgen_, and after a little bit transformation it will
  run backend with the specified options.

  :::Command-line options:::
  -b outfile_base
      Choose output file base name. By default it's empty(backend defaults will be used).

  -x backend
      Choose backend, which _ccgen_ will run and pass options and arguments to.
      Defaults to _cc_.

  -o option_spec
      Option specification. 
      _option_spec_ is a comma seperated list, which is logically divided in groups of two, each of which
      specify one possible value for an option. The first element(fname, or formal name) of a group is mandatory,
      and it's copied as it is to a backend. The second element(iname, or informal name)  of a group is optional,
      and it specifies meaning of an option for humans, which is used in output file name.

      If the second field of a group is missing, it is set to empty string.

  -l log_file
      File, to which all of the logs will be sent.

  -h
      Invoke help and exit.

  -v
      Print version and exit.

  -- 
      Stop processing options. That is, all after that will be though of as of arguments, even
      if it is started with -(dash).

  args... 
      Remaining arguments. All are passed to backend without change.

  :::Example:::
  ccgen -e .o		      \
        -b source	      \
	-o -c                 \
	-o -g,debug,,nodebug  \
	-o -m32,32,-m64,64    \
	source.c

  This will effectively generate following object files:
  source_debug_32.o, source_debug_64.o, source_nodebug_32.o, source_nodebug_64.o
	
  Name of files correspond to their contents, that is _source_debug_32.o_ contains
  object code for (home) 32 bit architecture with debug symbols, _source_nodebug_64. - 
  64 bit architecture without debug symbols, and so on.

  :::Return value:::
  0 on success. Some negative value otherwise. */
  

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>




#define MAX_OPTIONS        (100)
#define MAX_ARGS           (100)
#define MAX_COMMAND_LEN    (1000)
#define MAX_FILENAME_LEN   (50)
#define MAX_OPTION_VALUES  (10)

/* helping functions */

void error_exit(const char*,...);

void errno_exit(const char*,...);

/* @function str_write

   :::Summary:::
   Format append to string at index

   :::Description:::
   Appends formatted string to _str_ at index stores in *ind,
   writing at most _n_ characters, including null terminator('\0').
   
   If the call is successful(no overflow occured), the *ind is
   updated to point to the final null-terminator.

   Otherwise, ccgen is exited with some negative return code. */
void str_write(char *str, int *ind, size_t n, const char *fmt, ...);

/*
  @struct option_value
  :::Summary:::
  _option_value_ just represents a value that an option
  might have.

  :::Description:::
  _fname_ (char*) is the name of an option in terms
  of backend program. Mandatory.

  _iname_ (char*) is the name of an option, which
  will be used to generate output file name. Optional.
*/

struct option_value
{
  char *fname, *iname;
};

/*
  @struct option
  :::Summary:::
  Option that has many values.

  :::Description:::
  One of the values of a concrete option
  is passed to a backend at any moment.
*/
struct option
{
  struct option_value opt_val[MAX_OPTION_VALUES];
  int val_cnt;
};


/* 
   @function parse_input

   :::Summary:::
   Parses input.

  :::Notes:::
  Note that the function is implemented through _getopt_ function,
  instead of _argp_, because the latter is non-standard, despite being
  more easy-to-use.

*/
void parse_input(int, char **);

/*
  @function call_backend

  :::Summary:::
  Calls backand passing it 
  all needed options and arguments.

*/
int call_backend(const char *);



/*
  @function doTheJob

  :::Summary:::
  Iterates through all possible
  combinations of options and their values.
*/
void doTheJob(int);


/*
  @function print_help
  
  :::Summary:::
  Prints help message.

  The argument should be the name
  of the program, when it was invoked.
*/
void print_help(const char*);


/* global variables definitions */
static struct option passed_options[MAX_OPTIONS]; /* array of options that eventually will be passed to a backend */
static int cur_set[MAX_OPTIONS], option_count = 0, arg_count = 0;
static char *backend = "cc";
static char *outfile_base = NULL; /* if this field is NULL(not changed with command-line arguments,
				     then we don't explicitly specify output file, 
				     so we use backend's defaults */

static char *arguments[MAX_ARGS], /* passed_arguments */
  cmd_buf[MAX_COMMAND_LEN],       /* command formation buffer */
  file_buf[MAX_FILENAME_LEN];     /* filename formation buffer */
static char *logfile = NULL;      /* If it's non-NULL, redirect all output to that file */
static char *extension = NULL;    /* If it's NULL, no extension is appended to output filename. */
static const char * const ccgen_version = "1.0"; /* Current _ccgen_ version */



/* -----------MAIN BEGIN--------- */
int main(int argc, char *argv[])
{
  if (argc < 2)
    {
      print_help(argv[0]);
      exit(EXIT_FAILURE);
    }
  parse_input(argc, argv);

  if (logfile)
    {
      freopen(logfile, "w", stdout);
      freopen(logfile, "w", stderr);
    }
 
  doTheJob(0);

  exit(EXIT_SUCCESS);
}
/* ----------MAIN END----------- */



void parse_input(int argc, char *argv[])
{
  char *subopts, *value,  *just_null = NULL;
  int c, i;
  struct option tmp_option, *cur;

  opterr = 0;
  while ((c = getopt(argc, argv, ":vhb:x:l:e:o:")) != -1)
    {
      switch(c)
	{
	case 'v': /* version information is needed */
	  printf("ccgen %s\n", ccgen_version);
	  exit(EXIT_SUCCESS);
	  break;
	case 'h': /* help information is needed */
	  print_help(argv[0]);
	  exit(EXIT_SUCCESS);
	  break;
	case 'b': /* base name of an output file */
	  outfile_base = optarg;
	  break;
	case 'x': /* backend name */
	  backend = optarg;
	  break;
	case 'l': /* logging to some file */
	  logfile = optarg;
	  break;
	case 'e': /* output filename's extension */
	  extension = optarg;
	  break;
	case 'o': /* some option which we ultimately
		     pass to an underlying program */
	  memset(&tmp_option, 0, sizeof(struct option));

	  subopts = optarg;
	  cur = &passed_options[option_count++];
	  for (i = 0; *subopts != '\0'; ++i)
	    {
	      getsubopt(&subopts, &just_null, &value);

	      if (i % 2)
		cur -> opt_val[cur -> val_cnt-1].iname = value;
	      else
		cur -> opt_val[cur -> val_cnt++].fname = value;
	    }
	  
	  break;
	case '?':
	  error_exit("Unrecognized `-%c' option\n", optopt);
	  break;
	case ':':
	  error_exit("Missing operand for `-%c' option\n", optopt);
	  break;
	default:
	  abort();
	}
    }
  
  arg_count = argc - optind;
  /* all of the remaining (if any) arguments
     are copied without change */
  memcpy(arguments, argv + optind, arg_count * sizeof(char*));
}

int call_backend(const char *command)
{
  return system(command); /*currently we implement this
			    through the abstraction, which
			    was here before us in libc, but for 
			    practice purposes it should
			    be reimplemented in a harder
			    way */
}

void doTheJob(int opt)
{
  int i, j, cmd_ind = 0, file_ind = 0;
  struct option_value *cur_val;
  
  if (opt == option_count)
    {
      str_write(cmd_buf,
		&cmd_ind,
		MAX_COMMAND_LEN - cmd_ind,
		backend);

      if (outfile_base)
	str_write(file_buf,
		  &file_ind,
		  MAX_FILENAME_LEN - file_ind,
		  outfile_base);
      
      for (i = 0; i < option_count; ++i)
	{
	  cur_val = &passed_options[i].opt_val[cur_set[i]];
	  if (cur_val -> fname && strlen(cur_val -> fname))
	    str_write(cmd_buf,
		      &cmd_ind,
		      MAX_COMMAND_LEN - cmd_ind,
		      " %s", cur_val -> fname);
	  if (outfile_base && cur_val -> iname && strlen(cur_val -> iname))
	    str_write(file_buf,
		      &file_ind,
		      MAX_FILENAME_LEN - file_ind,
		      "_%s", cur_val -> iname);
	}

      if (outfile_base)
	{
	  if (extension)
	    str_write(file_buf,
		      &file_ind,
		      MAX_FILENAME_LEN - file_ind,
		      ".%s", extension);
	  str_write(cmd_buf,
		  &cmd_ind,
		  MAX_COMMAND_LEN - cmd_ind,
		  " -o %s", file_buf);
	}
      
      for (i = 0; i < arg_count; ++i)
	str_write(cmd_buf,
		  &cmd_ind,
		  MAX_COMMAND_LEN - cmd_ind,
		  " %s", arguments[i]);
      printf("Executing... %s\n", cmd_buf);

      call_backend(cmd_buf);
      return;
    }

  for (j = 0; j < passed_options[opt].val_cnt; ++j)
    {
      cur_set[opt] = j;
      doTheJob(opt + 1);
    }	
}

void print_help(const char *prog)
{
  printf("Usage: %s [options]... file...\n", prog);
  printf("Options:\n"
	 "-l <log_file>\t\t\tSend all output to <log_file>.\n"
	 "-x <backend>\t\t\tBackend name.\n"
	 "-o <option_spec>\t\tOption specification.\n"
	 "-b <base_file>\t\t\tOutput file base name.\n"
	 "-h\t\t\t\tDisplay this help.\n"
	 "-v\t\t\t\tDisplay version information\n");
}

void error_exit(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  exit(EXIT_FAILURE);
}

void errno_exit(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  
  perror("");
  vfprintf(stderr, fmt, ap);

  va_end(ap);

  exit(EXIT_FAILURE);
}

void str_write(char *str, int *ind, size_t n, const char *fmt, ...)
{
  int ch_cnt;

  va_list ap;
  va_start(ap, fmt);

  ch_cnt = vsnprintf(str + *ind, n, fmt, ap);

  if (ch_cnt >= n)
    error_exit("Attempt to buffer overflow encountered");

  *ind = *ind + ch_cnt;
  
  va_end(ap);
}
