# ccgen
Universal compiler frontend

Sometimes we would want to create lots of output files, no matter from which stage of a compiler they're from, be it preprocessor, code generation, assembly or linker stage.

For example, you have a C source file, _source.c_, and you need to produce object files with and without debug symbols, x86 and x86_64 versions, position independent and position dependent. How would you do it? The first thing that comes to mind is to run many times compilation with different options like that:
    
    cc -c -g -no-pie -m32 -o source_debug_nopie_32.o hello.c
    cc -c -g -no-pie -m64 -o source_debug_nopie_64.o hello.c
    cc -c -g -m32 -o source_debug_pie_32.o hello.c
    cc -c -g -m64 -o source_debug_pie_64.o hello.c
    cc -c -no-pie -m32 -o source_nodebug_nopie_32.o hello.c
    cc -c -no-pie -m64 -o source_nodebug_nopie_64.o hello.c
    cc -c -m32 -o source_nodebug_pie_32.o hello.c
    cc -c -m64 -o source_nodebug_pie_64.o hello.c

A lot of typing, isn't it?
That is how you can do it with _ccgen_:

    ccgen -e o \
          -b source \
          -o -c -o -g,debug,,nodebug \
          -o -no-pie,nopie,,pie \
          -o -m32,32,-m64,64 \
          source.c

Adding a new option with _ccgen_ is much simpler and shorter, than doing the same manually.
That is, we achieve scalability.

# Synopsis
  ccgen [-l logfile]
        [-x backend]
	[-b outfile_base]
	[-e extension]
	[-o option_spec]...
	args...

  ccgen -h

  ccgen -v

# Description

  It is particularly useful to use a compiler as a backend,
  effectively generating a lot of files, each of which will be compiled with different options.

  You pass options to _ccgen_, and after a little bit of transformation it will
  run backend with the specified options.

# Command-line options
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

# Return value
  0 on success. Some negative value otherwise.
