// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=param %s -verify
// XPASS: x86_64
// XFAIL: i686
// expected-no-diagnostics
// END.

# 1 "tree-NI.cpp"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 167 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "tree-NI.cpp" 2
# 1 "/usr/include/stdio.h" 1 3 4
# 28 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/features.h" 1 3 4
# 324 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/predefs.h" 1 3 4
# 325 "/usr/include/features.h" 2 3 4
# 357 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 1 3 4
# 378 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 379 "/usr/include/x86_64-linux-gnu/sys/cdefs.h" 2 3 4
# 358 "/usr/include/features.h" 2 3 4
# 389 "/usr/include/features.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 1 3 4



# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 5 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 2 3 4




# 1 "/usr/include/x86_64-linux-gnu/gnu/stubs-64.h" 1 3 4
# 10 "/usr/include/x86_64-linux-gnu/gnu/stubs.h" 2 3 4
# 390 "/usr/include/features.h" 2 3 4
# 29 "/usr/include/stdio.h" 2 3 4

extern "C" {




# 1 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stddef.h" 1 3 4
# 34 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stddef.h" 3 4
typedef long int ptrdiff_t;







typedef long unsigned int size_t;
# 35 "/usr/include/stdio.h" 2 3 4

# 1 "/usr/include/x86_64-linux-gnu/bits/types.h" 1 3 4
# 28 "/usr/include/x86_64-linux-gnu/bits/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 29 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4


typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;


typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;







typedef long int __quad_t;
typedef unsigned long int __u_quad_t;
# 131 "/usr/include/x86_64-linux-gnu/bits/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/typesizes.h" 1 3 4
# 132 "/usr/include/x86_64-linux-gnu/bits/types.h" 2 3 4


typedef unsigned long int __dev_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef unsigned long int __ino_t;
typedef unsigned long int __ino64_t;
typedef unsigned int __mode_t;
typedef unsigned long int __nlink_t;
typedef long int __off_t;
typedef long int __off64_t;
typedef int __pid_t;
typedef struct { int __val[2]; } __fsid_t;
typedef long int __clock_t;
typedef unsigned long int __rlim_t;
typedef unsigned long int __rlim64_t;
typedef unsigned int __id_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;

typedef int __daddr_t;
typedef long int __swblk_t;
typedef int __key_t;


typedef int __clockid_t;


typedef void * __timer_t;


typedef long int __blksize_t;




typedef long int __blkcnt_t;
typedef long int __blkcnt64_t;


typedef unsigned long int __fsblkcnt_t;
typedef unsigned long int __fsblkcnt64_t;


typedef unsigned long int __fsfilcnt_t;
typedef unsigned long int __fsfilcnt64_t;

typedef long int __ssize_t;



typedef __off64_t __loff_t;
typedef __quad_t *__qaddr_t;
typedef char *__caddr_t;


typedef long int __intptr_t;


typedef unsigned int __socklen_t;
# 37 "/usr/include/stdio.h" 2 3 4








struct _IO_FILE;



typedef struct _IO_FILE FILE;
# 65 "/usr/include/stdio.h" 3 4
typedef struct _IO_FILE __FILE;
# 75 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/libio.h" 1 3 4
# 32 "/usr/include/libio.h" 3 4
# 1 "/usr/include/_G_config.h" 1 3 4
# 15 "/usr/include/_G_config.h" 3 4
# 1 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stddef.h" 1 3 4
# 16 "/usr/include/_G_config.h" 2 3 4




# 1 "/usr/include/wchar.h" 1 3 4
# 83 "/usr/include/wchar.h" 3 4
typedef struct
{
  int __count;
  union
  {

    unsigned int __wch;



    char __wchb[4];
  } __value;
} __mbstate_t;
# 21 "/usr/include/_G_config.h" 2 3 4

typedef struct
{
  __off_t __pos;
  __mbstate_t __state;
} _G_fpos_t;
typedef struct
{
  __off64_t __pos;
  __mbstate_t __state;
} _G_fpos64_t;
# 53 "/usr/include/_G_config.h" 3 4
typedef int _G_int16_t __attribute__ ((__mode__ (__HI__)));
typedef int _G_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int _G_uint16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int _G_uint32_t __attribute__ ((__mode__ (__SI__)));
# 33 "/usr/include/libio.h" 2 3 4
# 53 "/usr/include/libio.h" 3 4
# 1 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stdarg.h" 1 3 4
# 30 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stdarg.h" 3 4
typedef __builtin_va_list va_list;
# 48 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 54 "/usr/include/libio.h" 2 3 4
# 172 "/usr/include/libio.h" 3 4
struct _IO_jump_t; struct _IO_FILE;
# 182 "/usr/include/libio.h" 3 4
typedef void _IO_lock_t;





struct _IO_marker {
  struct _IO_marker *_next;
  struct _IO_FILE *_sbuf;



  int _pos;
# 205 "/usr/include/libio.h" 3 4
};


enum __codecvt_result
{
  __codecvt_ok,
  __codecvt_partial,
  __codecvt_error,
  __codecvt_noconv
};
# 273 "/usr/include/libio.h" 3 4
struct _IO_FILE {
  int _flags;




  char* _IO_read_ptr;
  char* _IO_read_end;
  char* _IO_read_base;
  char* _IO_write_base;
  char* _IO_write_ptr;
  char* _IO_write_end;
  char* _IO_buf_base;
  char* _IO_buf_end;

  char *_IO_save_base;
  char *_IO_backup_base;
  char *_IO_save_end;

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;



  int _flags2;

  __off_t _old_offset;



  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];



  _IO_lock_t *_lock;
# 321 "/usr/include/libio.h" 3 4
  __off64_t _offset;
# 330 "/usr/include/libio.h" 3 4
  void *__pad1;
  void *__pad2;
  void *__pad3;
  void *__pad4;
  size_t __pad5;

  int _mode;

  char _unused2[15 * sizeof (int) - 4 * sizeof (void *) - sizeof (size_t)];

};





struct _IO_FILE_plus;

extern struct _IO_FILE_plus _IO_2_1_stdin_;
extern struct _IO_FILE_plus _IO_2_1_stdout_;
extern struct _IO_FILE_plus _IO_2_1_stderr_;
# 366 "/usr/include/libio.h" 3 4
typedef __ssize_t __io_read_fn (void *__cookie, char *__buf, size_t __nbytes);







typedef __ssize_t __io_write_fn (void *__cookie, __const char *__buf,
     size_t __n);







typedef int __io_seek_fn (void *__cookie, __off64_t *__pos, int __w);


typedef int __io_close_fn (void *__cookie);




typedef __io_read_fn cookie_read_function_t;
typedef __io_write_fn cookie_write_function_t;
typedef __io_seek_fn cookie_seek_function_t;
typedef __io_close_fn cookie_close_function_t;


typedef struct
{
  __io_read_fn *read;
  __io_write_fn *write;
  __io_seek_fn *seek;
  __io_close_fn *close;
} _IO_cookie_io_functions_t;
typedef _IO_cookie_io_functions_t cookie_io_functions_t;

struct _IO_cookie_file;


extern void _IO_cookie_init (struct _IO_cookie_file *__cfile, int __read_write,
        void *__cookie, _IO_cookie_io_functions_t __fns);




extern "C" {


extern int __underflow (_IO_FILE *);
extern int __uflow (_IO_FILE *);
extern int __overflow (_IO_FILE *, int);
# 462 "/usr/include/libio.h" 3 4
extern int _IO_getc (_IO_FILE *__fp);
extern int _IO_putc (int __c, _IO_FILE *__fp);
extern int _IO_feof (_IO_FILE *__fp) throw ();
extern int _IO_ferror (_IO_FILE *__fp) throw ();

extern int _IO_peekc_locked (_IO_FILE *__fp);





extern void _IO_flockfile (_IO_FILE *) throw ();
extern void _IO_funlockfile (_IO_FILE *) throw ();
extern int _IO_ftrylockfile (_IO_FILE *) throw ();
# 492 "/usr/include/libio.h" 3 4
extern int _IO_vfscanf (_IO_FILE * __restrict, const char * __restrict,
   __gnuc_va_list, int *__restrict);
extern int _IO_vfprintf (_IO_FILE *__restrict, const char *__restrict,
    __gnuc_va_list);
extern __ssize_t _IO_padn (_IO_FILE *, int, __ssize_t);
extern size_t _IO_sgetn (_IO_FILE *, void *, size_t);

extern __off64_t _IO_seekoff (_IO_FILE *, __off64_t, int, int);
extern __off64_t _IO_seekpos (_IO_FILE *, __off64_t, int);

extern void _IO_free_backup_area (_IO_FILE *) throw ();
# 554 "/usr/include/libio.h" 3 4
}
# 76 "/usr/include/stdio.h" 2 3 4




typedef __gnuc_va_list va_list;
# 91 "/usr/include/stdio.h" 3 4
typedef __off_t off_t;






typedef __off64_t off64_t;




typedef __ssize_t ssize_t;







typedef _G_fpos_t fpos_t;





typedef _G_fpos64_t fpos64_t;
# 165 "/usr/include/stdio.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/stdio_lim.h" 1 3 4
# 166 "/usr/include/stdio.h" 2 3 4



extern struct _IO_FILE *stdin;
extern struct _IO_FILE *stdout;
extern struct _IO_FILE *stderr;







extern int remove (__const char *__filename) throw ();

extern int rename (__const char *__old, __const char *__new) throw ();




extern int renameat (int __oldfd, __const char *__old, int __newfd,
       __const char *__new) throw ();
# 196 "/usr/include/stdio.h" 3 4
extern FILE *tmpfile (void) ;
# 206 "/usr/include/stdio.h" 3 4
extern FILE *tmpfile64 (void) ;



extern char *tmpnam (char *__s) throw () ;





extern char *tmpnam_r (char *__s) throw () ;
# 228 "/usr/include/stdio.h" 3 4
extern char *tempnam (__const char *__dir, __const char *__pfx)
     throw () __attribute__ ((__malloc__)) ;
# 238 "/usr/include/stdio.h" 3 4
extern int fclose (FILE *__stream);




extern int fflush (FILE *__stream);
# 253 "/usr/include/stdio.h" 3 4
extern int fflush_unlocked (FILE *__stream);
# 263 "/usr/include/stdio.h" 3 4
extern int fcloseall (void);
# 273 "/usr/include/stdio.h" 3 4
extern FILE *fopen (__const char *__restrict __filename,
      __const char *__restrict __modes) ;




extern FILE *freopen (__const char *__restrict __filename,
        __const char *__restrict __modes,
        FILE *__restrict __stream) ;
# 298 "/usr/include/stdio.h" 3 4
extern FILE *fopen64 (__const char *__restrict __filename,
        __const char *__restrict __modes) ;
extern FILE *freopen64 (__const char *__restrict __filename,
   __const char *__restrict __modes,
   FILE *__restrict __stream) ;




extern FILE *fdopen (int __fd, __const char *__modes) throw () ;





extern FILE *fopencookie (void *__restrict __magic_cookie,
     __const char *__restrict __modes,
     _IO_cookie_io_functions_t __io_funcs) throw () ;




extern FILE *fmemopen (void *__s, size_t __len, __const char *__modes)
  throw () ;




extern FILE *open_memstream (char **__bufloc, size_t *__sizeloc) throw () ;






extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) throw ();



extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
      int __modes, size_t __n) throw ();





extern void setbuffer (FILE *__restrict __stream, char *__restrict __buf,
         size_t __size) throw ();


extern void setlinebuf (FILE *__stream) throw ();
# 357 "/usr/include/stdio.h" 3 4
extern int fprintf (FILE *__restrict __stream,
      __const char *__restrict __format, ...);




extern int printf (__const char *__restrict __format, ...);

extern int sprintf (char *__restrict __s,
      __const char *__restrict __format, ...) throw ();





extern int vfprintf (FILE *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg);




extern int vprintf (__const char *__restrict __format, __gnuc_va_list __arg);

extern int vsprintf (char *__restrict __s, __const char *__restrict __format,
       __gnuc_va_list __arg) throw ();





extern int snprintf (char *__restrict __s, size_t __maxlen,
       __const char *__restrict __format, ...)
     throw () __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
        __const char *__restrict __format, __gnuc_va_list __arg)
     throw () __attribute__ ((__format__ (__printf__, 3, 0)));






extern int vasprintf (char **__restrict __ptr, __const char *__restrict __f,
        __gnuc_va_list __arg)
     throw () __attribute__ ((__format__ (__printf__, 2, 0))) ;
extern int __asprintf (char **__restrict __ptr,
         __const char *__restrict __fmt, ...)
     throw () __attribute__ ((__format__ (__printf__, 2, 3))) ;
extern int asprintf (char **__restrict __ptr,
       __const char *__restrict __fmt, ...)
     throw () __attribute__ ((__format__ (__printf__, 2, 3))) ;
# 418 "/usr/include/stdio.h" 3 4
extern int vdprintf (int __fd, __const char *__restrict __fmt,
       __gnuc_va_list __arg)
     __attribute__ ((__format__ (__printf__, 2, 0)));
extern int dprintf (int __fd, __const char *__restrict __fmt, ...)
     __attribute__ ((__format__ (__printf__, 2, 3)));
# 431 "/usr/include/stdio.h" 3 4
extern int fscanf (FILE *__restrict __stream,
     __const char *__restrict __format, ...) ;




extern int scanf (__const char *__restrict __format, ...) ;

extern int sscanf (__const char *__restrict __s,
     __const char *__restrict __format, ...) throw ();
# 477 "/usr/include/stdio.h" 3 4
extern int vfscanf (FILE *__restrict __s, __const char *__restrict __format,
      __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 2, 0))) ;





extern int vscanf (__const char *__restrict __format, __gnuc_va_list __arg)
     __attribute__ ((__format__ (__scanf__, 1, 0))) ;


extern int vsscanf (__const char *__restrict __s,
      __const char *__restrict __format, __gnuc_va_list __arg)
     throw () __attribute__ ((__format__ (__scanf__, 2, 0)));
# 537 "/usr/include/stdio.h" 3 4
extern int fgetc (FILE *__stream);
extern int getc (FILE *__stream);





extern int getchar (void);
# 556 "/usr/include/stdio.h" 3 4
extern int getc_unlocked (FILE *__stream);
extern int getchar_unlocked (void);
# 567 "/usr/include/stdio.h" 3 4
extern int fgetc_unlocked (FILE *__stream);
# 579 "/usr/include/stdio.h" 3 4
extern int fputc (int __c, FILE *__stream);
extern int putc (int __c, FILE *__stream);





extern int putchar (int __c);
# 600 "/usr/include/stdio.h" 3 4
extern int fputc_unlocked (int __c, FILE *__stream);







extern int putc_unlocked (int __c, FILE *__stream);
extern int putchar_unlocked (int __c);






extern int getw (FILE *__stream);


extern int putw (int __w, FILE *__stream);
# 628 "/usr/include/stdio.h" 3 4
extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
          ;






extern char *gets (char *__s) ;
# 646 "/usr/include/stdio.h" 3 4
extern char *fgets_unlocked (char *__restrict __s, int __n,
        FILE *__restrict __stream) ;
# 662 "/usr/include/stdio.h" 3 4
extern __ssize_t __getdelim (char **__restrict __lineptr,
          size_t *__restrict __n, int __delimiter,
          FILE *__restrict __stream) ;
extern __ssize_t getdelim (char **__restrict __lineptr,
        size_t *__restrict __n, int __delimiter,
        FILE *__restrict __stream) ;







extern __ssize_t getline (char **__restrict __lineptr,
       size_t *__restrict __n,
       FILE *__restrict __stream) ;
# 686 "/usr/include/stdio.h" 3 4
extern int fputs (__const char *__restrict __s, FILE *__restrict __stream);





extern int puts (__const char *__s);






extern int ungetc (int __c, FILE *__stream);






extern size_t fread (void *__restrict __ptr, size_t __size,
       size_t __n, FILE *__restrict __stream) ;




extern size_t fwrite (__const void *__restrict __ptr, size_t __size,
        size_t __n, FILE *__restrict __s);
# 723 "/usr/include/stdio.h" 3 4
extern int fputs_unlocked (__const char *__restrict __s,
      FILE *__restrict __stream);
# 734 "/usr/include/stdio.h" 3 4
extern size_t fread_unlocked (void *__restrict __ptr, size_t __size,
         size_t __n, FILE *__restrict __stream) ;
extern size_t fwrite_unlocked (__const void *__restrict __ptr, size_t __size,
          size_t __n, FILE *__restrict __stream);
# 746 "/usr/include/stdio.h" 3 4
extern int fseek (FILE *__stream, long int __off, int __whence);




extern long int ftell (FILE *__stream) ;




extern void rewind (FILE *__stream);
# 770 "/usr/include/stdio.h" 3 4
extern int fseeko (FILE *__stream, __off_t __off, int __whence);




extern __off_t ftello (FILE *__stream) ;
# 795 "/usr/include/stdio.h" 3 4
extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos);




extern int fsetpos (FILE *__stream, __const fpos_t *__pos);
# 815 "/usr/include/stdio.h" 3 4
extern int fseeko64 (FILE *__stream, __off64_t __off, int __whence);
extern __off64_t ftello64 (FILE *__stream) ;
extern int fgetpos64 (FILE *__restrict __stream, fpos64_t *__restrict __pos);
extern int fsetpos64 (FILE *__stream, __const fpos64_t *__pos);




extern void clearerr (FILE *__stream) throw ();

extern int feof (FILE *__stream) throw () ;

extern int ferror (FILE *__stream) throw () ;




extern void clearerr_unlocked (FILE *__stream) throw ();
extern int feof_unlocked (FILE *__stream) throw () ;
extern int ferror_unlocked (FILE *__stream) throw () ;
# 843 "/usr/include/stdio.h" 3 4
extern void perror (__const char *__s);







# 1 "/usr/include/x86_64-linux-gnu/bits/sys_errlist.h" 1 3 4
# 27 "/usr/include/x86_64-linux-gnu/bits/sys_errlist.h" 3 4
extern int sys_nerr;
extern __const char *__const sys_errlist[];


extern int _sys_nerr;
extern __const char *__const _sys_errlist[];
# 851 "/usr/include/stdio.h" 2 3 4




extern int fileno (FILE *__stream) throw () ;




extern int fileno_unlocked (FILE *__stream) throw () ;
# 870 "/usr/include/stdio.h" 3 4
extern FILE *popen (__const char *__command, __const char *__modes) ;





extern int pclose (FILE *__stream);





extern char *ctermid (char *__s) throw ();





extern char *cuserid (char *__s);




struct obstack;


extern int obstack_printf (struct obstack *__restrict __obstack,
      __const char *__restrict __format, ...)
     throw () __attribute__ ((__format__ (__printf__, 2, 3)));
extern int obstack_vprintf (struct obstack *__restrict __obstack,
       __const char *__restrict __format,
       __gnuc_va_list __args)
     throw () __attribute__ ((__format__ (__printf__, 2, 0)));







extern void flockfile (FILE *__stream) throw ();



extern int ftrylockfile (FILE *__stream) throw () ;


extern void funlockfile (FILE *__stream) throw ();
# 940 "/usr/include/stdio.h" 3 4
}
# 2 "tree-NI.cpp" 2
# 1 "/usr/include/stdlib.h" 1 3 4
# 33 "/usr/include/stdlib.h" 3 4
# 1 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stddef.h" 1 3 4
# 34 "/usr/include/stdlib.h" 2 3 4

extern "C" {







# 1 "/usr/include/x86_64-linux-gnu/bits/waitflags.h" 1 3 4
# 43 "/usr/include/stdlib.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/waitstatus.h" 1 3 4
# 65 "/usr/include/x86_64-linux-gnu/bits/waitstatus.h" 3 4
# 1 "/usr/include/endian.h" 1 3 4
# 37 "/usr/include/endian.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/endian.h" 1 3 4
# 38 "/usr/include/endian.h" 2 3 4
# 61 "/usr/include/endian.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 1 3 4
# 28 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 29 "/usr/include/x86_64-linux-gnu/bits/byteswap.h" 2 3 4
# 62 "/usr/include/endian.h" 2 3 4
# 66 "/usr/include/x86_64-linux-gnu/bits/waitstatus.h" 2 3 4

union wait
  {
    int w_status;
    struct
      {

 unsigned int __w_termsig:7;
 unsigned int __w_coredump:1;
 unsigned int __w_retcode:8;
 unsigned int:16;







      } __wait_terminated;
    struct
      {

 unsigned int __w_stopval:8;
 unsigned int __w_stopsig:8;
 unsigned int:16;






      } __wait_stopped;
  };
# 44 "/usr/include/stdlib.h" 2 3 4
# 98 "/usr/include/stdlib.h" 3 4
typedef struct
  {
    int quot;
    int rem;
  } div_t;



typedef struct
  {
    long int quot;
    long int rem;
  } ldiv_t;







__extension__ typedef struct
  {
    long long int quot;
    long long int rem;
  } lldiv_t;
# 140 "/usr/include/stdlib.h" 3 4
extern size_t __ctype_get_mb_cur_max (void) throw () ;




extern double atof (__const char *__nptr)
     throw () __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern int atoi (__const char *__nptr)
     throw () __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;

extern long int atol (__const char *__nptr)
     throw () __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;





__extension__ extern long long int atoll (__const char *__nptr)
     throw () __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;





extern double strtod (__const char *__restrict __nptr,
        char **__restrict __endptr)
     throw () __attribute__ ((__nonnull__ (1))) ;





extern float strtof (__const char *__restrict __nptr,
       char **__restrict __endptr) throw () __attribute__ ((__nonnull__ (1))) ;

extern long double strtold (__const char *__restrict __nptr,
       char **__restrict __endptr)
     throw () __attribute__ ((__nonnull__ (1))) ;





extern long int strtol (__const char *__restrict __nptr,
   char **__restrict __endptr, int __base)
     throw () __attribute__ ((__nonnull__ (1))) ;

extern unsigned long int strtoul (__const char *__restrict __nptr,
      char **__restrict __endptr, int __base)
     throw () __attribute__ ((__nonnull__ (1))) ;




__extension__
extern long long int strtoq (__const char *__restrict __nptr,
        char **__restrict __endptr, int __base)
     throw () __attribute__ ((__nonnull__ (1))) ;

__extension__
extern unsigned long long int strtouq (__const char *__restrict __nptr,
           char **__restrict __endptr, int __base)
     throw () __attribute__ ((__nonnull__ (1))) ;





__extension__
extern long long int strtoll (__const char *__restrict __nptr,
         char **__restrict __endptr, int __base)
     throw () __attribute__ ((__nonnull__ (1))) ;

__extension__
extern unsigned long long int strtoull (__const char *__restrict __nptr,
     char **__restrict __endptr, int __base)
     throw () __attribute__ ((__nonnull__ (1))) ;
# 236 "/usr/include/stdlib.h" 3 4
# 1 "/usr/include/xlocale.h" 1 3 4
# 28 "/usr/include/xlocale.h" 3 4
typedef struct __locale_struct
{

  struct __locale_data *__locales[13];


  const unsigned short int *__ctype_b;
  const int *__ctype_tolower;
  const int *__ctype_toupper;


  const char *__names[13];
} *__locale_t;


typedef __locale_t locale_t;
# 237 "/usr/include/stdlib.h" 2 3 4



extern long int strtol_l (__const char *__restrict __nptr,
     char **__restrict __endptr, int __base,
     __locale_t __loc) throw () __attribute__ ((__nonnull__ (1, 4))) ;

extern unsigned long int strtoul_l (__const char *__restrict __nptr,
        char **__restrict __endptr,
        int __base, __locale_t __loc)
     throw () __attribute__ ((__nonnull__ (1, 4))) ;

__extension__
extern long long int strtoll_l (__const char *__restrict __nptr,
    char **__restrict __endptr, int __base,
    __locale_t __loc)
     throw () __attribute__ ((__nonnull__ (1, 4))) ;

__extension__
extern unsigned long long int strtoull_l (__const char *__restrict __nptr,
       char **__restrict __endptr,
       int __base, __locale_t __loc)
     throw () __attribute__ ((__nonnull__ (1, 4))) ;

extern double strtod_l (__const char *__restrict __nptr,
   char **__restrict __endptr, __locale_t __loc)
     throw () __attribute__ ((__nonnull__ (1, 3))) ;

extern float strtof_l (__const char *__restrict __nptr,
         char **__restrict __endptr, __locale_t __loc)
     throw () __attribute__ ((__nonnull__ (1, 3))) ;

extern long double strtold_l (__const char *__restrict __nptr,
         char **__restrict __endptr,
         __locale_t __loc)
     throw () __attribute__ ((__nonnull__ (1, 3))) ;
# 311 "/usr/include/stdlib.h" 3 4
extern char *l64a (long int __n) throw () ;


extern long int a64l (__const char *__s)
     throw () __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))) ;





# 1 "/usr/include/x86_64-linux-gnu/sys/types.h" 1 3 4
# 28 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
extern "C" {





typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;




typedef __loff_t loff_t;



typedef __ino_t ino_t;






typedef __ino64_t ino64_t;




typedef __dev_t dev_t;




typedef __gid_t gid_t;




typedef __mode_t mode_t;




typedef __nlink_t nlink_t;




typedef __uid_t uid_t;
# 99 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
typedef __pid_t pid_t;





typedef __id_t id_t;
# 116 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;





typedef __key_t key_t;
# 133 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
# 1 "/usr/include/time.h" 1 3 4
# 60 "/usr/include/time.h" 3 4
typedef __clock_t clock_t;
# 76 "/usr/include/time.h" 3 4
typedef __time_t time_t;
# 92 "/usr/include/time.h" 3 4
typedef __clockid_t clockid_t;
# 104 "/usr/include/time.h" 3 4
typedef __timer_t timer_t;
# 134 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4



typedef __useconds_t useconds_t;



typedef __suseconds_t suseconds_t;






# 1 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stddef.h" 1 3 4
# 148 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4



typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;
# 195 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
typedef int int8_t __attribute__ ((__mode__ (__QI__)));
typedef int int16_t __attribute__ ((__mode__ (__HI__)));
typedef int int32_t __attribute__ ((__mode__ (__SI__)));
typedef int int64_t __attribute__ ((__mode__ (__DI__)));


typedef unsigned int u_int8_t __attribute__ ((__mode__ (__QI__)));
typedef unsigned int u_int16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int u_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int u_int64_t __attribute__ ((__mode__ (__DI__)));

typedef int register_t __attribute__ ((__mode__ (__word__)));
# 220 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/sys/select.h" 1 3 4
# 31 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/select.h" 1 3 4
# 23 "/usr/include/x86_64-linux-gnu/bits/select.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 24 "/usr/include/x86_64-linux-gnu/bits/select.h" 2 3 4
# 32 "/usr/include/x86_64-linux-gnu/sys/select.h" 2 3 4


# 1 "/usr/include/x86_64-linux-gnu/bits/sigset.h" 1 3 4
# 24 "/usr/include/x86_64-linux-gnu/bits/sigset.h" 3 4
typedef int __sig_atomic_t;




typedef struct
  {
    unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
  } __sigset_t;
# 35 "/usr/include/x86_64-linux-gnu/sys/select.h" 2 3 4



typedef __sigset_t sigset_t;






# 1 "/usr/include/time.h" 1 3 4
# 120 "/usr/include/time.h" 3 4
struct timespec
  {
    __time_t tv_sec;
    long int tv_nsec;
  };
# 45 "/usr/include/x86_64-linux-gnu/sys/select.h" 2 3 4

# 1 "/usr/include/x86_64-linux-gnu/bits/time.h" 1 3 4
# 31 "/usr/include/x86_64-linux-gnu/bits/time.h" 3 4
struct timeval
  {
    __time_t tv_sec;
    __suseconds_t tv_usec;
  };
# 47 "/usr/include/x86_64-linux-gnu/sys/select.h" 2 3 4








typedef long int __fd_mask;
# 65 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
typedef struct
  {



    __fd_mask fds_bits[1024 / (8 * (int) sizeof (__fd_mask))];





  } fd_set;






typedef __fd_mask fd_mask;
# 97 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
extern "C" {
# 107 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
# 119 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);
# 132 "/usr/include/x86_64-linux-gnu/sys/select.h" 3 4
}
# 221 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4


# 1 "/usr/include/x86_64-linux-gnu/sys/sysmacros.h" 1 3 4
# 30 "/usr/include/x86_64-linux-gnu/sys/sysmacros.h" 3 4
extern "C" {

__extension__
extern unsigned int gnu_dev_major (unsigned long long int __dev)
     throw () __attribute__ ((__const__));
__extension__
extern unsigned int gnu_dev_minor (unsigned long long int __dev)
     throw () __attribute__ ((__const__));
__extension__
extern unsigned long long int gnu_dev_makedev (unsigned int __major,
            unsigned int __minor)
     throw () __attribute__ ((__const__));
# 64 "/usr/include/x86_64-linux-gnu/sys/sysmacros.h" 3 4
}
# 224 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4





typedef __blksize_t blksize_t;






typedef __blkcnt_t blkcnt_t;



typedef __fsblkcnt_t fsblkcnt_t;



typedef __fsfilcnt_t fsfilcnt_t;
# 263 "/usr/include/x86_64-linux-gnu/sys/types.h" 3 4
typedef __blkcnt64_t blkcnt64_t;
typedef __fsblkcnt64_t fsblkcnt64_t;
typedef __fsfilcnt64_t fsfilcnt64_t;






# 1 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 1 3 4
# 23 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 24 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 2 3 4
# 50 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 3 4
typedef unsigned long int pthread_t;


typedef union
{
  char __size[56];
  long int __align;
} pthread_attr_t;



typedef struct __pthread_internal_list
{
  struct __pthread_internal_list *__prev;
  struct __pthread_internal_list *__next;
} __pthread_list_t;
# 76 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 3 4
typedef union
{
  struct __pthread_mutex_s
  {
    int __lock;
    unsigned int __count;
    int __owner;

    unsigned int __nusers;



    int __kind;

    int __spins;
    __pthread_list_t __list;
# 101 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 3 4
  } __data;
  char __size[40];
  long int __align;
} pthread_mutex_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_mutexattr_t;




typedef union
{
  struct
  {
    int __lock;
    unsigned int __futex;
    __extension__ unsigned long long int __total_seq;
    __extension__ unsigned long long int __wakeup_seq;
    __extension__ unsigned long long int __woken_seq;
    void *__mutex;
    unsigned int __nwaiters;
    unsigned int __broadcast_seq;
  } __data;
  char __size[48];
  __extension__ long long int __align;
} pthread_cond_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;



typedef unsigned int pthread_key_t;



typedef int pthread_once_t;





typedef union
{

  struct
  {
    int __lock;
    unsigned int __nr_readers;
    unsigned int __readers_wakeup;
    unsigned int __writer_wakeup;
    unsigned int __nr_readers_queued;
    unsigned int __nr_writers_queued;
    int __writer;
    int __shared;
    unsigned long int __pad1;
    unsigned long int __pad2;


    unsigned int __flags;
  } __data;
# 187 "/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h" 3 4
  char __size[56];
  long int __align;
} pthread_rwlock_t;

typedef union
{
  char __size[8];
  long int __align;
} pthread_rwlockattr_t;





typedef volatile int pthread_spinlock_t;




typedef union
{
  char __size[32];
  long int __align;
} pthread_barrier_t;

typedef union
{
  char __size[4];
  int __align;
} pthread_barrierattr_t;
# 272 "/usr/include/x86_64-linux-gnu/sys/types.h" 2 3 4


}
# 321 "/usr/include/stdlib.h" 2 3 4






extern long int random (void) throw ();


extern void srandom (unsigned int __seed) throw ();





extern char *initstate (unsigned int __seed, char *__statebuf,
   size_t __statelen) throw () __attribute__ ((__nonnull__ (2)));



extern char *setstate (char *__statebuf) throw () __attribute__ ((__nonnull__ (1)));







struct random_data
  {
    int32_t *fptr;
    int32_t *rptr;
    int32_t *state;
    int rand_type;
    int rand_deg;
    int rand_sep;
    int32_t *end_ptr;
  };

extern int random_r (struct random_data *__restrict __buf,
       int32_t *__restrict __result) throw () __attribute__ ((__nonnull__ (1, 2)));

extern int srandom_r (unsigned int __seed, struct random_data *__buf)
     throw () __attribute__ ((__nonnull__ (2)));

extern int initstate_r (unsigned int __seed, char *__restrict __statebuf,
   size_t __statelen,
   struct random_data *__restrict __buf)
     throw () __attribute__ ((__nonnull__ (2, 4)));

extern int setstate_r (char *__restrict __statebuf,
         struct random_data *__restrict __buf)
     throw () __attribute__ ((__nonnull__ (1, 2)));






extern int rand (void) throw ();

extern void srand (unsigned int __seed) throw ();




extern int rand_r (unsigned int *__seed) throw ();







extern double drand48 (void) throw ();
extern double erand48 (unsigned short int __xsubi[3]) throw () __attribute__ ((__nonnull__ (1)));


extern long int lrand48 (void) throw ();
extern long int nrand48 (unsigned short int __xsubi[3])
     throw () __attribute__ ((__nonnull__ (1)));


extern long int mrand48 (void) throw ();
extern long int jrand48 (unsigned short int __xsubi[3])
     throw () __attribute__ ((__nonnull__ (1)));


extern void srand48 (long int __seedval) throw ();
extern unsigned short int *seed48 (unsigned short int __seed16v[3])
     throw () __attribute__ ((__nonnull__ (1)));
extern void lcong48 (unsigned short int __param[7]) throw () __attribute__ ((__nonnull__ (1)));





struct drand48_data
  {
    unsigned short int __x[3];
    unsigned short int __old_x[3];
    unsigned short int __c;
    unsigned short int __init;
    unsigned long long int __a;
  };


extern int drand48_r (struct drand48_data *__restrict __buffer,
        double *__restrict __result) throw () __attribute__ ((__nonnull__ (1, 2)));
extern int erand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        double *__restrict __result) throw () __attribute__ ((__nonnull__ (1, 2)));


extern int lrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     throw () __attribute__ ((__nonnull__ (1, 2)));
extern int nrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     throw () __attribute__ ((__nonnull__ (1, 2)));


extern int mrand48_r (struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     throw () __attribute__ ((__nonnull__ (1, 2)));
extern int jrand48_r (unsigned short int __xsubi[3],
        struct drand48_data *__restrict __buffer,
        long int *__restrict __result)
     throw () __attribute__ ((__nonnull__ (1, 2)));


extern int srand48_r (long int __seedval, struct drand48_data *__buffer)
     throw () __attribute__ ((__nonnull__ (2)));

extern int seed48_r (unsigned short int __seed16v[3],
       struct drand48_data *__buffer) throw () __attribute__ ((__nonnull__ (1, 2)));

extern int lcong48_r (unsigned short int __param[7],
        struct drand48_data *__buffer)
     throw () __attribute__ ((__nonnull__ (1, 2)));
# 471 "/usr/include/stdlib.h" 3 4
extern void *malloc (size_t __size) throw () __attribute__ ((__malloc__)) ;

extern void *calloc (size_t __nmemb, size_t __size)
     throw () __attribute__ ((__malloc__)) ;
# 485 "/usr/include/stdlib.h" 3 4
extern void *realloc (void *__ptr, size_t __size)
     throw () __attribute__ ((__warn_unused_result__));

extern void free (void *__ptr) throw ();




extern void cfree (void *__ptr) throw ();




# 1 "/usr/include/alloca.h" 1 3 4
# 25 "/usr/include/alloca.h" 3 4
# 1 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stddef.h" 1 3 4
# 26 "/usr/include/alloca.h" 2 3 4

extern "C" {





extern void *alloca (size_t __size) throw ();





}
# 498 "/usr/include/stdlib.h" 2 3 4





extern void *valloc (size_t __size) throw () __attribute__ ((__malloc__)) ;




extern int posix_memalign (void **__memptr, size_t __alignment, size_t __size)
     throw () __attribute__ ((__nonnull__ (1))) ;




extern void abort (void) throw () __attribute__ ((__noreturn__));



extern int atexit (void (*__func) (void)) throw () __attribute__ ((__nonnull__ (1)));






extern "C++" int at_quick_exit (void (*__func) (void))
     throw () __asm ("at_quick_exit") __attribute__ ((__nonnull__ (1)));
# 536 "/usr/include/stdlib.h" 3 4
extern int on_exit (void (*__func) (int __status, void *__arg), void *__arg)
     throw () __attribute__ ((__nonnull__ (1)));






extern void exit (int __status) throw () __attribute__ ((__noreturn__));







extern void quick_exit (int __status) throw () __attribute__ ((__noreturn__));







extern void _Exit (int __status) throw () __attribute__ ((__noreturn__));






extern char *getenv (__const char *__name) throw () __attribute__ ((__nonnull__ (1))) ;




extern char *__secure_getenv (__const char *__name)
     throw () __attribute__ ((__nonnull__ (1))) ;





extern int putenv (char *__string) throw () __attribute__ ((__nonnull__ (1)));





extern int setenv (__const char *__name, __const char *__value, int __replace)
     throw () __attribute__ ((__nonnull__ (2)));


extern int unsetenv (__const char *__name) throw () __attribute__ ((__nonnull__ (1)));






extern int clearenv (void) throw ();
# 606 "/usr/include/stdlib.h" 3 4
extern char *mktemp (char *__template) throw () __attribute__ ((__nonnull__ (1))) ;
# 620 "/usr/include/stdlib.h" 3 4
extern int mkstemp (char *__template) __attribute__ ((__nonnull__ (1))) ;
# 630 "/usr/include/stdlib.h" 3 4
extern int mkstemp64 (char *__template) __attribute__ ((__nonnull__ (1))) ;
# 642 "/usr/include/stdlib.h" 3 4
extern int mkstemps (char *__template, int __suffixlen) __attribute__ ((__nonnull__ (1))) ;
# 652 "/usr/include/stdlib.h" 3 4
extern int mkstemps64 (char *__template, int __suffixlen)
     __attribute__ ((__nonnull__ (1))) ;
# 663 "/usr/include/stdlib.h" 3 4
extern char *mkdtemp (char *__template) throw () __attribute__ ((__nonnull__ (1))) ;
# 674 "/usr/include/stdlib.h" 3 4
extern int mkostemp (char *__template, int __flags) __attribute__ ((__nonnull__ (1))) ;
# 684 "/usr/include/stdlib.h" 3 4
extern int mkostemp64 (char *__template, int __flags) __attribute__ ((__nonnull__ (1))) ;
# 694 "/usr/include/stdlib.h" 3 4
extern int mkostemps (char *__template, int __suffixlen, int __flags)
     __attribute__ ((__nonnull__ (1))) ;
# 706 "/usr/include/stdlib.h" 3 4
extern int mkostemps64 (char *__template, int __suffixlen, int __flags)
     __attribute__ ((__nonnull__ (1))) ;
# 717 "/usr/include/stdlib.h" 3 4
extern int system (__const char *__command) ;






extern char *canonicalize_file_name (__const char *__name)
     throw () __attribute__ ((__nonnull__ (1))) ;
# 734 "/usr/include/stdlib.h" 3 4
extern char *realpath (__const char *__restrict __name,
         char *__restrict __resolved) throw () ;






typedef int (*__compar_fn_t) (__const void *, __const void *);


typedef __compar_fn_t comparison_fn_t;



typedef int (*__compar_d_fn_t) (__const void *, __const void *, void *);





extern void *bsearch (__const void *__key, __const void *__base,
        size_t __nmemb, size_t __size, __compar_fn_t __compar)
     __attribute__ ((__nonnull__ (1, 2, 5))) ;



extern void qsort (void *__base, size_t __nmemb, size_t __size,
     __compar_fn_t __compar) __attribute__ ((__nonnull__ (1, 4)));

extern void qsort_r (void *__base, size_t __nmemb, size_t __size,
       __compar_d_fn_t __compar, void *__arg)
  __attribute__ ((__nonnull__ (1, 4)));




extern int abs (int __x) throw () __attribute__ ((__const__)) ;
extern long int labs (long int __x) throw () __attribute__ ((__const__)) ;



__extension__ extern long long int llabs (long long int __x)
     throw () __attribute__ ((__const__)) ;







extern div_t div (int __numer, int __denom)
     throw () __attribute__ ((__const__)) ;
extern ldiv_t ldiv (long int __numer, long int __denom)
     throw () __attribute__ ((__const__)) ;




__extension__ extern lldiv_t lldiv (long long int __numer,
        long long int __denom)
     throw () __attribute__ ((__const__)) ;
# 808 "/usr/include/stdlib.h" 3 4
extern char *ecvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) throw () __attribute__ ((__nonnull__ (3, 4))) ;




extern char *fcvt (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign) throw () __attribute__ ((__nonnull__ (3, 4))) ;




extern char *gcvt (double __value, int __ndigit, char *__buf)
     throw () __attribute__ ((__nonnull__ (3))) ;




extern char *qecvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     throw () __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qfcvt (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign)
     throw () __attribute__ ((__nonnull__ (3, 4))) ;
extern char *qgcvt (long double __value, int __ndigit, char *__buf)
     throw () __attribute__ ((__nonnull__ (3))) ;




extern int ecvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) throw () __attribute__ ((__nonnull__ (3, 4, 5)));
extern int fcvt_r (double __value, int __ndigit, int *__restrict __decpt,
     int *__restrict __sign, char *__restrict __buf,
     size_t __len) throw () __attribute__ ((__nonnull__ (3, 4, 5)));

extern int qecvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     throw () __attribute__ ((__nonnull__ (3, 4, 5)));
extern int qfcvt_r (long double __value, int __ndigit,
      int *__restrict __decpt, int *__restrict __sign,
      char *__restrict __buf, size_t __len)
     throw () __attribute__ ((__nonnull__ (3, 4, 5)));







extern int mblen (__const char *__s, size_t __n) throw () ;


extern int mbtowc (wchar_t *__restrict __pwc,
     __const char *__restrict __s, size_t __n) throw () ;


extern int wctomb (char *__s, wchar_t __wchar) throw () ;



extern size_t mbstowcs (wchar_t *__restrict __pwcs,
   __const char *__restrict __s, size_t __n) throw ();

extern size_t wcstombs (char *__restrict __s,
   __const wchar_t *__restrict __pwcs, size_t __n)
     throw ();
# 885 "/usr/include/stdlib.h" 3 4
extern int rpmatch (__const char *__response) throw () __attribute__ ((__nonnull__ (1))) ;
# 896 "/usr/include/stdlib.h" 3 4
extern int getsubopt (char **__restrict __optionp,
        char *__const *__restrict __tokens,
        char **__restrict __valuep)
     throw () __attribute__ ((__nonnull__ (1, 2, 3))) ;





extern void setkey (__const char *__key) throw () __attribute__ ((__nonnull__ (1)));







extern int posix_openpt (int __oflag) ;







extern int grantpt (int __fd) throw ();



extern int unlockpt (int __fd) throw ();




extern char *ptsname (int __fd) throw () ;






extern int ptsname_r (int __fd, char *__buf, size_t __buflen)
     throw () __attribute__ ((__nonnull__ (2)));


extern int getpt (void);






extern int getloadavg (double __loadavg[], int __nelem)
     throw () __attribute__ ((__nonnull__ (1)));
# 964 "/usr/include/stdlib.h" 3 4
}
# 3 "tree-NI.cpp" 2

# 1 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task_scheduler_init.h" 1
# 32 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task_scheduler_init.h"
# 1 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h" 1
# 103 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h"
# 1 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_config.h" 1
# 104 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h" 2
# 121 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h"
# 1 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/cstddef" 1 3
# 42 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/cstddef" 3

# 1 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/x86_64-linux-gnu/bits/c++config.h" 1 3
# 153 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/x86_64-linux-gnu/bits/c++config.h" 3
namespace std
{
  typedef long unsigned int size_t;
  typedef long int ptrdiff_t;


  typedef decltype(nullptr) nullptr_t;

}
# 393 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/x86_64-linux-gnu/bits/c++config.h" 3
# 1 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/x86_64-linux-gnu/bits/os_defines.h" 1 3
# 394 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/x86_64-linux-gnu/bits/c++config.h" 2 3


# 1 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/x86_64-linux-gnu/bits/cpu_defines.h" 1 3
# 397 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/x86_64-linux-gnu/bits/c++config.h" 2 3
# 44 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/cstddef" 2 3
# 1 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stddef.h" 1 3
# 45 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/cstddef" 2 3
# 122 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h" 2







# 1 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stdint.h" 1 3
# 64 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stdint.h" 3
# 1 "/usr/include/stdint.h" 1 3 4
# 27 "/usr/include/stdint.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wchar.h" 1 3 4
# 28 "/usr/include/stdint.h" 2 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 29 "/usr/include/stdint.h" 2 3 4
# 49 "/usr/include/stdint.h" 3 4
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;

typedef unsigned int uint32_t;



typedef unsigned long int uint64_t;
# 66 "/usr/include/stdint.h" 3 4
typedef signed char int_least8_t;
typedef short int int_least16_t;
typedef int int_least32_t;

typedef long int int_least64_t;






typedef unsigned char uint_least8_t;
typedef unsigned short int uint_least16_t;
typedef unsigned int uint_least32_t;

typedef unsigned long int uint_least64_t;
# 91 "/usr/include/stdint.h" 3 4
typedef signed char int_fast8_t;

typedef long int int_fast16_t;
typedef long int int_fast32_t;
typedef long int int_fast64_t;
# 104 "/usr/include/stdint.h" 3 4
typedef unsigned char uint_fast8_t;

typedef unsigned long int uint_fast16_t;
typedef unsigned long int uint_fast32_t;
typedef unsigned long int uint_fast64_t;
# 120 "/usr/include/stdint.h" 3 4
typedef long int intptr_t;


typedef unsigned long int uintptr_t;
# 135 "/usr/include/stdint.h" 3 4
typedef long int intmax_t;
typedef unsigned long int uintmax_t;
# 65 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stdint.h" 2 3
# 130 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h" 2



typedef void(*assertion_handler_type)( const char* filename, int line, const char* expression, const char * comment );
# 176 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h"
namespace tbb {
# 190 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h"
    namespace internal {
        using ::int8_t;
        using ::int16_t;
        using ::int32_t;
        using ::int64_t;
        using ::uint8_t;
        using ::uint16_t;
        using ::uint32_t;
        using ::uint64_t;
    }


    using std::size_t;
    using std::ptrdiff_t;






extern "C" int TBB_runtime_interface_version();






class split {
};





namespace internal {





const size_t NFS_MaxLineSize = 128;
# 253 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h"
template<class T, int S>
struct padded_base : T {
    char pad[NFS_MaxLineSize - sizeof(T) % NFS_MaxLineSize];
};
template<class T> struct padded_base<T, 0> : T {};


template<class T>
struct padded : padded_base<T, sizeof(T)> {};
# 274 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h"
void handle_perror( int error_code, const char* aux_info );
# 290 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h"
void runtime_warning( const char* format, ... );
# 303 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h"
template<typename T>
inline void poison_pointer( T* ) { }







template<typename T, typename U>
inline T punned_cast( U* ptr ) {
    uintptr_t x = reinterpret_cast<uintptr_t>(ptr);
    return reinterpret_cast<T>(x);
}


class no_assign {

    void operator=( const no_assign& );
public:


    no_assign() {}

};


class no_copy: no_assign {

    no_copy( const no_copy& );
public:

    no_copy() {}
};


template<typename T>
struct allocator_type {
    typedef T value_type;
};
# 353 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_stddef.h"
template <unsigned u, unsigned long long ull >
struct select_size_t_constant {


    static const size_t value = (size_t)((sizeof(size_t)==sizeof(u)) ? u : ull);
};


template<typename T>
inline bool is_aligned(T* pointer, uintptr_t alignment) {
    return 0==((uintptr_t)pointer & (alignment-1));
}


template<typename integer_type>
inline bool is_power_of_two(integer_type arg) {
    return arg && (0 == (arg & (arg - 1)));
}


template<typename argument_integer_type, typename divisor_integer_type>
inline argument_integer_type modulo_power_of_two(argument_integer_type arg, divisor_integer_type divisor) {

    ((void)0);
    return (arg & (divisor - 1));
}





template<typename argument_integer_type, typename divisor_integer_type>
inline bool is_power_of_two_factor(argument_integer_type arg, divisor_integer_type divisor) {

    ((void)0);
    return 0 == (arg & (arg - divisor));
}


template<typename T>
void suppress_unused_warning( const T& ) {}




struct version_tag_v3 {};

typedef version_tag_v3 version_tag;

}


}

namespace tbb { namespace internal {
template <bool condition>
struct STATIC_ASSERTION_FAILED;

template <>
struct STATIC_ASSERTION_FAILED<false> { enum {value=1};};

template<>
struct STATIC_ASSERTION_FAILED<true>;
}}
# 33 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task_scheduler_init.h" 2
# 1 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/limits.h" 1 3
# 38 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/limits.h" 3
# 1 "/usr/include/limits.h" 1 3 4
# 145 "/usr/include/limits.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/posix1_lim.h" 1 3 4
# 157 "/usr/include/x86_64-linux-gnu/bits/posix1_lim.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/local_lim.h" 1 3 4
# 39 "/usr/include/x86_64-linux-gnu/bits/local_lim.h" 3 4
# 1 "/usr/include/linux/limits.h" 1 3 4
# 40 "/usr/include/x86_64-linux-gnu/bits/local_lim.h" 2 3 4
# 158 "/usr/include/x86_64-linux-gnu/bits/posix1_lim.h" 2 3 4
# 146 "/usr/include/limits.h" 2 3 4



# 1 "/usr/include/x86_64-linux-gnu/bits/posix2_lim.h" 1 3 4
# 150 "/usr/include/limits.h" 2 3 4



# 1 "/usr/include/x86_64-linux-gnu/bits/xopen_lim.h" 1 3 4
# 34 "/usr/include/x86_64-linux-gnu/bits/xopen_lim.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/stdio_lim.h" 1 3 4
# 35 "/usr/include/x86_64-linux-gnu/bits/xopen_lim.h" 2 3 4
# 154 "/usr/include/limits.h" 2 3 4
# 39 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/limits.h" 2 3
# 34 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task_scheduler_init.h" 2

namespace tbb {

typedef std::size_t stack_size_type;


namespace internal {


    class scheduler;
}
# 61 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task_scheduler_init.h"
class task_scheduler_init: internal::no_copy {
    enum ExceptionPropagationMode {
        propagation_mode_exact = 1u,
        propagation_mode_captured = 2u,
        propagation_mode_mask = propagation_mode_exact | propagation_mode_captured
    };







    internal::scheduler* my_scheduler;
public:


    static const int automatic = -1;


    static const int deferred = -2;
# 95 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task_scheduler_init.h"
    void initialize( int number_of_threads=automatic );



    void initialize( int number_of_threads, stack_size_type thread_stack_size );


    void terminate();





    task_scheduler_init( int number_of_threads=automatic, stack_size_type thread_stack_size=0 )

  : my_scheduler(__null) {







        ((void)0);

        thread_stack_size |= 0 ? propagation_mode_captured : propagation_mode_exact;





        initialize( number_of_threads, thread_stack_size );
    }


    ~task_scheduler_init() {
        if( my_scheduler )
            terminate();
        internal::poison_pointer( my_scheduler );
    }
# 153 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task_scheduler_init.h"
    static int default_num_threads ();


    bool is_active() const { return my_scheduler != __null; }
};

}
# 5 "tree-NI.cpp" 2
# 1 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/parallel_invoke.h" 1
# 32 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/parallel_invoke.h"
# 1 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h" 1
# 33 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
# 1 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h" 1
# 127 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
namespace tbb {
namespace internal {







template <typename T, std::size_t S>
struct machine_load_store;

template <typename T, std::size_t S>
struct machine_load_store_relaxed;

template <typename T, std::size_t S>
struct machine_load_store_seq_cst;




template<size_t S> struct atomic_selector;

template<> struct atomic_selector<1> {
    typedef int8_t word;
    inline static word fetch_store ( volatile void* location, word value );
};

template<> struct atomic_selector<2> {
    typedef int16_t word;
    inline static word fetch_store ( volatile void* location, word value );
};

template<> struct atomic_selector<4> {




    typedef int32_t word;

    inline static word fetch_store ( volatile void* location, word value );
};

template<> struct atomic_selector<8> {
    typedef int64_t word;
    inline static word fetch_store ( volatile void* location, word value );
};

}}
# 246 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
# 1 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/linux_intel64.h" 1
# 36 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/linux_intel64.h"
# 1 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/gcc_ia32_common.h" 1
# 36 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/gcc_ia32_common.h"
static inline intptr_t __TBB_machine_lg( uintptr_t x ) {
    ((void)0);
    uintptr_t j;
    __asm__ ("bsr %1,%0" : "=r"(j) : "r"(x));
    return j;
}
# 54 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/gcc_ia32_common.h"
static inline void __TBB_machine_pause( int32_t delay ) {
    for (int32_t i = 0; i < delay; i++) {
       __asm__ __volatile__("pause;");
    }
    return;
}







struct __TBB_cpu_ctl_env_t {
    int mxcsr;
    short x87cw;
};
inline void __TBB_get_cpu_ctl_env ( __TBB_cpu_ctl_env_t* ctl ) {
# 81 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/gcc_ia32_common.h"
    __asm__ __volatile__ (
            "stmxcsr %0\n\t"
            "fstcw %1"
            : "=m"(ctl->mxcsr), "=m"(ctl->x87cw)
    );

}
inline void __TBB_set_cpu_ctl_env ( const __TBB_cpu_ctl_env_t* ctl ) {
    __asm__ __volatile__ (
            "ldmxcsr %0\n\t"
            "fldcw %1"
            : : "m"(ctl->mxcsr), "m"(ctl->x87cw)
    );
}
# 37 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/linux_intel64.h" 2
# 82 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/linux_intel64.h"
static inline int8_t __TBB_machine_cmpswp1 (volatile void *ptr, int8_t value, int8_t comparand ) { int8_t result; __asm__ __volatile__("lock\ncmpxchg" "" " %2,%1" : "=a"(result), "=m"(*(volatile int8_t*)ptr) : "q"(value), "0"(comparand), "m"(*(volatile int8_t*)ptr) : "memory"); return result; } static inline int8_t __TBB_machine_fetchadd1(volatile void *ptr, int8_t addend) { int8_t result; __asm__ __volatile__("lock\nxadd" "" " %0,%1" : "=r"(result),"=m"(*(volatile int8_t*)ptr) : "0"(addend), "m"(*(volatile int8_t*)ptr) : "memory"); return result; } static inline int8_t __TBB_machine_fetchstore1(volatile void *ptr, int8_t value) { int8_t result; __asm__ __volatile__("lock\nxchg" "" " %0,%1" : "=r"(result),"=m"(*(volatile int8_t*)ptr) : "0"(value), "m"(*(volatile int8_t*)ptr) : "memory"); return result; }
static inline int16_t __TBB_machine_cmpswp2 (volatile void *ptr, int16_t value, int16_t comparand ) { int16_t result; __asm__ __volatile__("lock\ncmpxchg" "" " %2,%1" : "=a"(result), "=m"(*(volatile int16_t*)ptr) : "q"(value), "0"(comparand), "m"(*(volatile int16_t*)ptr) : "memory"); return result; } static inline int16_t __TBB_machine_fetchadd2(volatile void *ptr, int16_t addend) { int16_t result; __asm__ __volatile__("lock\nxadd" "" " %0,%1" : "=r"(result),"=m"(*(volatile int16_t*)ptr) : "0"(addend), "m"(*(volatile int16_t*)ptr) : "memory"); return result; } static inline int16_t __TBB_machine_fetchstore2(volatile void *ptr, int16_t value) { int16_t result; __asm__ __volatile__("lock\nxchg" "" " %0,%1" : "=r"(result),"=m"(*(volatile int16_t*)ptr) : "0"(value), "m"(*(volatile int16_t*)ptr) : "memory"); return result; }
static inline int32_t __TBB_machine_cmpswp4 (volatile void *ptr, int32_t value, int32_t comparand ) { int32_t result; __asm__ __volatile__("lock\ncmpxchg" "" " %2,%1" : "=a"(result), "=m"(*(volatile int32_t*)ptr) : "q"(value), "0"(comparand), "m"(*(volatile int32_t*)ptr) : "memory"); return result; } static inline int32_t __TBB_machine_fetchadd4(volatile void *ptr, int32_t addend) { int32_t result; __asm__ __volatile__("lock\nxadd" "" " %0,%1" : "=r"(result),"=m"(*(volatile int32_t*)ptr) : "0"(addend), "m"(*(volatile int32_t*)ptr) : "memory"); return result; } static inline int32_t __TBB_machine_fetchstore4(volatile void *ptr, int32_t value) { int32_t result; __asm__ __volatile__("lock\nxchg" "" " %0,%1" : "=r"(result),"=m"(*(volatile int32_t*)ptr) : "0"(value), "m"(*(volatile int32_t*)ptr) : "memory"); return result; }
static inline int64_t __TBB_machine_cmpswp8 (volatile void *ptr, int64_t value, int64_t comparand ) { int64_t result; __asm__ __volatile__("lock\ncmpxchg" "q" " %2,%1" : "=a"(result), "=m"(*(volatile int64_t*)ptr) : "q"(value), "0"(comparand), "m"(*(volatile int64_t*)ptr) : "memory"); return result; } static inline int64_t __TBB_machine_fetchadd8(volatile void *ptr, int64_t addend) { int64_t result; __asm__ __volatile__("lock\nxadd" "q" " %0,%1" : "=r"(result),"=m"(*(volatile int64_t*)ptr) : "0"(addend), "m"(*(volatile int64_t*)ptr) : "memory"); return result; } static inline int64_t __TBB_machine_fetchstore8(volatile void *ptr, int64_t value) { int64_t result; __asm__ __volatile__("lock\nxchg" "q" " %0,%1" : "=r"(result),"=m"(*(volatile int64_t*)ptr) : "0"(value), "m"(*(volatile int64_t*)ptr) : "memory"); return result; }



static inline void __TBB_machine_or( volatile void *ptr, uint64_t value ) {
    __asm__ __volatile__("lock\norq %1,%0" : "=m"(*(volatile uint64_t*)ptr) : "r"(value), "m"(*(volatile uint64_t*)ptr) : "memory");
}

static inline void __TBB_machine_and( volatile void *ptr, uint64_t value ) {
    __asm__ __volatile__("lock\nandq %1,%0" : "=m"(*(volatile uint64_t*)ptr) : "r"(value), "m"(*(volatile uint64_t*)ptr) : "memory");
}
# 247 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h" 2
# 256 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
# 1 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/linux_common.h" 1
# 33 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/linux_common.h"
# 1 "/usr/include/sched.h" 1 3 4
# 30 "/usr/include/sched.h" 3 4
# 1 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stddef.h" 1 3 4
# 31 "/usr/include/sched.h" 2 3 4



# 1 "/usr/include/time.h" 1 3 4
# 35 "/usr/include/sched.h" 2 3 4








# 1 "/usr/include/x86_64-linux-gnu/bits/sched.h" 1 3 4
# 74 "/usr/include/x86_64-linux-gnu/bits/sched.h" 3 4
struct sched_param
  {
    int __sched_priority;
  };

extern "C" {



extern int clone (int (*__fn) (void *__arg), void *__child_stack,
    int __flags, void *__arg, ...) throw ();


extern int unshare (int __flags) throw ();


extern int sched_getcpu (void) throw ();


extern int setns (int __fd, int __nstype) throw ();



}







struct __sched_param
  {
    int __sched_priority;
  };
# 120 "/usr/include/x86_64-linux-gnu/bits/sched.h" 3 4
typedef unsigned long int __cpu_mask;






typedef struct
{
  __cpu_mask __bits[1024 / (8 * sizeof (__cpu_mask))];
} cpu_set_t;
# 203 "/usr/include/x86_64-linux-gnu/bits/sched.h" 3 4
extern "C" {

extern int __sched_cpucount (size_t __setsize, const cpu_set_t *__setp)
  throw ();
extern cpu_set_t *__sched_cpualloc (size_t __count) throw () ;
extern void __sched_cpufree (cpu_set_t *__set) throw ();

}
# 44 "/usr/include/sched.h" 2 3 4




extern "C" {


extern int sched_setparam (__pid_t __pid, __const struct sched_param *__param)
     throw ();


extern int sched_getparam (__pid_t __pid, struct sched_param *__param) throw ();


extern int sched_setscheduler (__pid_t __pid, int __policy,
          __const struct sched_param *__param) throw ();


extern int sched_getscheduler (__pid_t __pid) throw ();


extern int sched_yield (void) throw ();


extern int sched_get_priority_max (int __algorithm) throw ();


extern int sched_get_priority_min (int __algorithm) throw ();


extern int sched_rr_get_interval (__pid_t __pid, struct timespec *__t) throw ();
# 118 "/usr/include/sched.h" 3 4
extern int sched_setaffinity (__pid_t __pid, size_t __cpusetsize,
         __const cpu_set_t *__cpuset) throw ();


extern int sched_getaffinity (__pid_t __pid, size_t __cpusetsize,
         cpu_set_t *__cpuset) throw ();


}
# 34 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/linux_common.h" 2


# 1 "/usr/include/unistd.h" 1 3 4
# 28 "/usr/include/unistd.h" 3 4
extern "C" {
# 203 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/posix_opt.h" 1 3 4
# 204 "/usr/include/unistd.h" 2 3 4



# 1 "/usr/include/x86_64-linux-gnu/bits/environments.h" 1 3 4
# 23 "/usr/include/x86_64-linux-gnu/bits/environments.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 24 "/usr/include/x86_64-linux-gnu/bits/environments.h" 2 3 4
# 208 "/usr/include/unistd.h" 2 3 4
# 227 "/usr/include/unistd.h" 3 4
# 1 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stddef.h" 1 3 4
# 228 "/usr/include/unistd.h" 2 3 4
# 275 "/usr/include/unistd.h" 3 4
typedef __socklen_t socklen_t;
# 288 "/usr/include/unistd.h" 3 4
extern int access (__const char *__name, int __type) throw () __attribute__ ((__nonnull__ (1)));




extern int euidaccess (__const char *__name, int __type)
     throw () __attribute__ ((__nonnull__ (1)));


extern int eaccess (__const char *__name, int __type)
     throw () __attribute__ ((__nonnull__ (1)));






extern int faccessat (int __fd, __const char *__file, int __type, int __flag)
     throw () __attribute__ ((__nonnull__ (2))) ;
# 335 "/usr/include/unistd.h" 3 4
extern __off_t lseek (int __fd, __off_t __offset, int __whence) throw ();
# 346 "/usr/include/unistd.h" 3 4
extern __off64_t lseek64 (int __fd, __off64_t __offset, int __whence)
     throw ();






extern int close (int __fd);






extern ssize_t read (int __fd, void *__buf, size_t __nbytes) ;





extern ssize_t write (int __fd, __const void *__buf, size_t __n) ;
# 377 "/usr/include/unistd.h" 3 4
extern ssize_t pread (int __fd, void *__buf, size_t __nbytes,
        __off_t __offset) ;






extern ssize_t pwrite (int __fd, __const void *__buf, size_t __n,
         __off_t __offset) ;
# 405 "/usr/include/unistd.h" 3 4
extern ssize_t pread64 (int __fd, void *__buf, size_t __nbytes,
   __off64_t __offset) ;


extern ssize_t pwrite64 (int __fd, __const void *__buf, size_t __n,
    __off64_t __offset) ;







extern int pipe (int __pipedes[2]) throw () ;




extern int pipe2 (int __pipedes[2], int __flags) throw () ;
# 433 "/usr/include/unistd.h" 3 4
extern unsigned int alarm (unsigned int __seconds) throw ();
# 445 "/usr/include/unistd.h" 3 4
extern unsigned int sleep (unsigned int __seconds);







extern __useconds_t ualarm (__useconds_t __value, __useconds_t __interval)
     throw ();






extern int usleep (__useconds_t __useconds);
# 470 "/usr/include/unistd.h" 3 4
extern int pause (void);



extern int chown (__const char *__file, __uid_t __owner, __gid_t __group)
     throw () __attribute__ ((__nonnull__ (1))) ;



extern int fchown (int __fd, __uid_t __owner, __gid_t __group) throw () ;




extern int lchown (__const char *__file, __uid_t __owner, __gid_t __group)
     throw () __attribute__ ((__nonnull__ (1))) ;






extern int fchownat (int __fd, __const char *__file, __uid_t __owner,
       __gid_t __group, int __flag)
     throw () __attribute__ ((__nonnull__ (2))) ;



extern int chdir (__const char *__path) throw () __attribute__ ((__nonnull__ (1))) ;



extern int fchdir (int __fd) throw () ;
# 512 "/usr/include/unistd.h" 3 4
extern char *getcwd (char *__buf, size_t __size) throw () ;





extern char *get_current_dir_name (void) throw ();







extern char *getwd (char *__buf)
     throw () __attribute__ ((__nonnull__ (1))) __attribute__ ((__deprecated__)) ;




extern int dup (int __fd) throw () ;


extern int dup2 (int __fd, int __fd2) throw ();




extern int dup3 (int __fd, int __fd2, int __flags) throw ();



extern char **__environ;

extern char **environ;





extern int execve (__const char *__path, char *__const __argv[],
     char *__const __envp[]) throw () __attribute__ ((__nonnull__ (1, 2)));




extern int fexecve (int __fd, char *__const __argv[], char *__const __envp[])
     throw () __attribute__ ((__nonnull__ (2)));




extern int execv (__const char *__path, char *__const __argv[])
     throw () __attribute__ ((__nonnull__ (1, 2)));



extern int execle (__const char *__path, __const char *__arg, ...)
     throw () __attribute__ ((__nonnull__ (1, 2)));



extern int execl (__const char *__path, __const char *__arg, ...)
     throw () __attribute__ ((__nonnull__ (1, 2)));



extern int execvp (__const char *__file, char *__const __argv[])
     throw () __attribute__ ((__nonnull__ (1, 2)));




extern int execlp (__const char *__file, __const char *__arg, ...)
     throw () __attribute__ ((__nonnull__ (1, 2)));




extern int execvpe (__const char *__file, char *__const __argv[],
      char *__const __envp[])
     throw () __attribute__ ((__nonnull__ (1, 2)));





extern int nice (int __inc) throw () ;




extern void _exit (int __status) __attribute__ ((__noreturn__));






# 1 "/usr/include/x86_64-linux-gnu/bits/confname.h" 1 3 4
# 26 "/usr/include/x86_64-linux-gnu/bits/confname.h" 3 4
enum
  {
    _PC_LINK_MAX,

    _PC_MAX_CANON,

    _PC_MAX_INPUT,

    _PC_NAME_MAX,

    _PC_PATH_MAX,

    _PC_PIPE_BUF,

    _PC_CHOWN_RESTRICTED,

    _PC_NO_TRUNC,

    _PC_VDISABLE,

    _PC_SYNC_IO,

    _PC_ASYNC_IO,

    _PC_PRIO_IO,

    _PC_SOCK_MAXBUF,

    _PC_FILESIZEBITS,

    _PC_REC_INCR_XFER_SIZE,

    _PC_REC_MAX_XFER_SIZE,

    _PC_REC_MIN_XFER_SIZE,

    _PC_REC_XFER_ALIGN,

    _PC_ALLOC_SIZE_MIN,

    _PC_SYMLINK_MAX,

    _PC_2_SYMLINKS

  };


enum
  {
    _SC_ARG_MAX,

    _SC_CHILD_MAX,

    _SC_CLK_TCK,

    _SC_NGROUPS_MAX,

    _SC_OPEN_MAX,

    _SC_STREAM_MAX,

    _SC_TZNAME_MAX,

    _SC_JOB_CONTROL,

    _SC_SAVED_IDS,

    _SC_REALTIME_SIGNALS,

    _SC_PRIORITY_SCHEDULING,

    _SC_TIMERS,

    _SC_ASYNCHRONOUS_IO,

    _SC_PRIORITIZED_IO,

    _SC_SYNCHRONIZED_IO,

    _SC_FSYNC,

    _SC_MAPPED_FILES,

    _SC_MEMLOCK,

    _SC_MEMLOCK_RANGE,

    _SC_MEMORY_PROTECTION,

    _SC_MESSAGE_PASSING,

    _SC_SEMAPHORES,

    _SC_SHARED_MEMORY_OBJECTS,

    _SC_AIO_LISTIO_MAX,

    _SC_AIO_MAX,

    _SC_AIO_PRIO_DELTA_MAX,

    _SC_DELAYTIMER_MAX,

    _SC_MQ_OPEN_MAX,

    _SC_MQ_PRIO_MAX,

    _SC_VERSION,

    _SC_PAGESIZE,


    _SC_RTSIG_MAX,

    _SC_SEM_NSEMS_MAX,

    _SC_SEM_VALUE_MAX,

    _SC_SIGQUEUE_MAX,

    _SC_TIMER_MAX,




    _SC_BC_BASE_MAX,

    _SC_BC_DIM_MAX,

    _SC_BC_SCALE_MAX,

    _SC_BC_STRING_MAX,

    _SC_COLL_WEIGHTS_MAX,

    _SC_EQUIV_CLASS_MAX,

    _SC_EXPR_NEST_MAX,

    _SC_LINE_MAX,

    _SC_RE_DUP_MAX,

    _SC_CHARCLASS_NAME_MAX,


    _SC_2_VERSION,

    _SC_2_C_BIND,

    _SC_2_C_DEV,

    _SC_2_FORT_DEV,

    _SC_2_FORT_RUN,

    _SC_2_SW_DEV,

    _SC_2_LOCALEDEF,


    _SC_PII,

    _SC_PII_XTI,

    _SC_PII_SOCKET,

    _SC_PII_INTERNET,

    _SC_PII_OSI,

    _SC_POLL,

    _SC_SELECT,

    _SC_UIO_MAXIOV,

    _SC_IOV_MAX = _SC_UIO_MAXIOV,

    _SC_PII_INTERNET_STREAM,

    _SC_PII_INTERNET_DGRAM,

    _SC_PII_OSI_COTS,

    _SC_PII_OSI_CLTS,

    _SC_PII_OSI_M,

    _SC_T_IOV_MAX,



    _SC_THREADS,

    _SC_THREAD_SAFE_FUNCTIONS,

    _SC_GETGR_R_SIZE_MAX,

    _SC_GETPW_R_SIZE_MAX,

    _SC_LOGIN_NAME_MAX,

    _SC_TTY_NAME_MAX,

    _SC_THREAD_DESTRUCTOR_ITERATIONS,

    _SC_THREAD_KEYS_MAX,

    _SC_THREAD_STACK_MIN,

    _SC_THREAD_THREADS_MAX,

    _SC_THREAD_ATTR_STACKADDR,

    _SC_THREAD_ATTR_STACKSIZE,

    _SC_THREAD_PRIORITY_SCHEDULING,

    _SC_THREAD_PRIO_INHERIT,

    _SC_THREAD_PRIO_PROTECT,

    _SC_THREAD_PROCESS_SHARED,


    _SC_NPROCESSORS_CONF,

    _SC_NPROCESSORS_ONLN,

    _SC_PHYS_PAGES,

    _SC_AVPHYS_PAGES,

    _SC_ATEXIT_MAX,

    _SC_PASS_MAX,


    _SC_XOPEN_VERSION,

    _SC_XOPEN_XCU_VERSION,

    _SC_XOPEN_UNIX,

    _SC_XOPEN_CRYPT,

    _SC_XOPEN_ENH_I18N,

    _SC_XOPEN_SHM,


    _SC_2_CHAR_TERM,

    _SC_2_C_VERSION,

    _SC_2_UPE,


    _SC_XOPEN_XPG2,

    _SC_XOPEN_XPG3,

    _SC_XOPEN_XPG4,


    _SC_CHAR_BIT,

    _SC_CHAR_MAX,

    _SC_CHAR_MIN,

    _SC_INT_MAX,

    _SC_INT_MIN,

    _SC_LONG_BIT,

    _SC_WORD_BIT,

    _SC_MB_LEN_MAX,

    _SC_NZERO,

    _SC_SSIZE_MAX,

    _SC_SCHAR_MAX,

    _SC_SCHAR_MIN,

    _SC_SHRT_MAX,

    _SC_SHRT_MIN,

    _SC_UCHAR_MAX,

    _SC_UINT_MAX,

    _SC_ULONG_MAX,

    _SC_USHRT_MAX,


    _SC_NL_ARGMAX,

    _SC_NL_LANGMAX,

    _SC_NL_MSGMAX,

    _SC_NL_NMAX,

    _SC_NL_SETMAX,

    _SC_NL_TEXTMAX,


    _SC_XBS5_ILP32_OFF32,

    _SC_XBS5_ILP32_OFFBIG,

    _SC_XBS5_LP64_OFF64,

    _SC_XBS5_LPBIG_OFFBIG,


    _SC_XOPEN_LEGACY,

    _SC_XOPEN_REALTIME,

    _SC_XOPEN_REALTIME_THREADS,


    _SC_ADVISORY_INFO,

    _SC_BARRIERS,

    _SC_BASE,

    _SC_C_LANG_SUPPORT,

    _SC_C_LANG_SUPPORT_R,

    _SC_CLOCK_SELECTION,

    _SC_CPUTIME,

    _SC_THREAD_CPUTIME,

    _SC_DEVICE_IO,

    _SC_DEVICE_SPECIFIC,

    _SC_DEVICE_SPECIFIC_R,

    _SC_FD_MGMT,

    _SC_FIFO,

    _SC_PIPE,

    _SC_FILE_ATTRIBUTES,

    _SC_FILE_LOCKING,

    _SC_FILE_SYSTEM,

    _SC_MONOTONIC_CLOCK,

    _SC_MULTI_PROCESS,

    _SC_SINGLE_PROCESS,

    _SC_NETWORKING,

    _SC_READER_WRITER_LOCKS,

    _SC_SPIN_LOCKS,

    _SC_REGEXP,

    _SC_REGEX_VERSION,

    _SC_SHELL,

    _SC_SIGNALS,

    _SC_SPAWN,

    _SC_SPORADIC_SERVER,

    _SC_THREAD_SPORADIC_SERVER,

    _SC_SYSTEM_DATABASE,

    _SC_SYSTEM_DATABASE_R,

    _SC_TIMEOUTS,

    _SC_TYPED_MEMORY_OBJECTS,

    _SC_USER_GROUPS,

    _SC_USER_GROUPS_R,

    _SC_2_PBS,

    _SC_2_PBS_ACCOUNTING,

    _SC_2_PBS_LOCATE,

    _SC_2_PBS_MESSAGE,

    _SC_2_PBS_TRACK,

    _SC_SYMLOOP_MAX,

    _SC_STREAMS,

    _SC_2_PBS_CHECKPOINT,


    _SC_V6_ILP32_OFF32,

    _SC_V6_ILP32_OFFBIG,

    _SC_V6_LP64_OFF64,

    _SC_V6_LPBIG_OFFBIG,


    _SC_HOST_NAME_MAX,

    _SC_TRACE,

    _SC_TRACE_EVENT_FILTER,

    _SC_TRACE_INHERIT,

    _SC_TRACE_LOG,


    _SC_LEVEL1_ICACHE_SIZE,

    _SC_LEVEL1_ICACHE_ASSOC,

    _SC_LEVEL1_ICACHE_LINESIZE,

    _SC_LEVEL1_DCACHE_SIZE,

    _SC_LEVEL1_DCACHE_ASSOC,

    _SC_LEVEL1_DCACHE_LINESIZE,

    _SC_LEVEL2_CACHE_SIZE,

    _SC_LEVEL2_CACHE_ASSOC,

    _SC_LEVEL2_CACHE_LINESIZE,

    _SC_LEVEL3_CACHE_SIZE,

    _SC_LEVEL3_CACHE_ASSOC,

    _SC_LEVEL3_CACHE_LINESIZE,

    _SC_LEVEL4_CACHE_SIZE,

    _SC_LEVEL4_CACHE_ASSOC,

    _SC_LEVEL4_CACHE_LINESIZE,



    _SC_IPV6 = _SC_LEVEL1_ICACHE_SIZE + 50,

    _SC_RAW_SOCKETS,


    _SC_V7_ILP32_OFF32,

    _SC_V7_ILP32_OFFBIG,

    _SC_V7_LP64_OFF64,

    _SC_V7_LPBIG_OFFBIG,


    _SC_SS_REPL_MAX,


    _SC_TRACE_EVENT_NAME_MAX,

    _SC_TRACE_NAME_MAX,

    _SC_TRACE_SYS_MAX,

    _SC_TRACE_USER_EVENT_MAX,


    _SC_XOPEN_STREAMS,


    _SC_THREAD_ROBUST_PRIO_INHERIT,

    _SC_THREAD_ROBUST_PRIO_PROTECT

  };


enum
  {
    _CS_PATH,


    _CS_V6_WIDTH_RESTRICTED_ENVS,



    _CS_GNU_LIBC_VERSION,

    _CS_GNU_LIBPTHREAD_VERSION,


    _CS_V5_WIDTH_RESTRICTED_ENVS,



    _CS_V7_WIDTH_RESTRICTED_ENVS,



    _CS_LFS_CFLAGS = 1000,

    _CS_LFS_LDFLAGS,

    _CS_LFS_LIBS,

    _CS_LFS_LINTFLAGS,

    _CS_LFS64_CFLAGS,

    _CS_LFS64_LDFLAGS,

    _CS_LFS64_LIBS,

    _CS_LFS64_LINTFLAGS,


    _CS_XBS5_ILP32_OFF32_CFLAGS = 1100,

    _CS_XBS5_ILP32_OFF32_LDFLAGS,

    _CS_XBS5_ILP32_OFF32_LIBS,

    _CS_XBS5_ILP32_OFF32_LINTFLAGS,

    _CS_XBS5_ILP32_OFFBIG_CFLAGS,

    _CS_XBS5_ILP32_OFFBIG_LDFLAGS,

    _CS_XBS5_ILP32_OFFBIG_LIBS,

    _CS_XBS5_ILP32_OFFBIG_LINTFLAGS,

    _CS_XBS5_LP64_OFF64_CFLAGS,

    _CS_XBS5_LP64_OFF64_LDFLAGS,

    _CS_XBS5_LP64_OFF64_LIBS,

    _CS_XBS5_LP64_OFF64_LINTFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_CFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_LDFLAGS,

    _CS_XBS5_LPBIG_OFFBIG_LIBS,

    _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS,


    _CS_POSIX_V6_ILP32_OFF32_CFLAGS,

    _CS_POSIX_V6_ILP32_OFF32_LDFLAGS,

    _CS_POSIX_V6_ILP32_OFF32_LIBS,

    _CS_POSIX_V6_ILP32_OFF32_LINTFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_CFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_LDFLAGS,

    _CS_POSIX_V6_ILP32_OFFBIG_LIBS,

    _CS_POSIX_V6_ILP32_OFFBIG_LINTFLAGS,

    _CS_POSIX_V6_LP64_OFF64_CFLAGS,

    _CS_POSIX_V6_LP64_OFF64_LDFLAGS,

    _CS_POSIX_V6_LP64_OFF64_LIBS,

    _CS_POSIX_V6_LP64_OFF64_LINTFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_CFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LDFLAGS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LIBS,

    _CS_POSIX_V6_LPBIG_OFFBIG_LINTFLAGS,


    _CS_POSIX_V7_ILP32_OFF32_CFLAGS,

    _CS_POSIX_V7_ILP32_OFF32_LDFLAGS,

    _CS_POSIX_V7_ILP32_OFF32_LIBS,

    _CS_POSIX_V7_ILP32_OFF32_LINTFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_CFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_LDFLAGS,

    _CS_POSIX_V7_ILP32_OFFBIG_LIBS,

    _CS_POSIX_V7_ILP32_OFFBIG_LINTFLAGS,

    _CS_POSIX_V7_LP64_OFF64_CFLAGS,

    _CS_POSIX_V7_LP64_OFF64_LDFLAGS,

    _CS_POSIX_V7_LP64_OFF64_LIBS,

    _CS_POSIX_V7_LP64_OFF64_LINTFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_CFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LDFLAGS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LIBS,

    _CS_POSIX_V7_LPBIG_OFFBIG_LINTFLAGS,


    _CS_V6_ENV,

    _CS_V7_ENV

  };
# 611 "/usr/include/unistd.h" 2 3 4


extern long int pathconf (__const char *__path, int __name)
     throw () __attribute__ ((__nonnull__ (1)));


extern long int fpathconf (int __fd, int __name) throw ();


extern long int sysconf (int __name) throw ();



extern size_t confstr (int __name, char *__buf, size_t __len) throw ();




extern __pid_t getpid (void) throw ();


extern __pid_t getppid (void) throw ();




extern __pid_t getpgrp (void) throw ();
# 647 "/usr/include/unistd.h" 3 4
extern __pid_t __getpgid (__pid_t __pid) throw ();

extern __pid_t getpgid (__pid_t __pid) throw ();






extern int setpgid (__pid_t __pid, __pid_t __pgid) throw ();
# 673 "/usr/include/unistd.h" 3 4
extern int setpgrp (void) throw ();
# 690 "/usr/include/unistd.h" 3 4
extern __pid_t setsid (void) throw ();



extern __pid_t getsid (__pid_t __pid) throw ();



extern __uid_t getuid (void) throw ();


extern __uid_t geteuid (void) throw ();


extern __gid_t getgid (void) throw ();


extern __gid_t getegid (void) throw ();




extern int getgroups (int __size, __gid_t __list[]) throw () ;



extern int group_member (__gid_t __gid) throw ();






extern int setuid (__uid_t __uid) throw ();




extern int setreuid (__uid_t __ruid, __uid_t __euid) throw ();




extern int seteuid (__uid_t __uid) throw ();






extern int setgid (__gid_t __gid) throw ();




extern int setregid (__gid_t __rgid, __gid_t __egid) throw ();




extern int setegid (__gid_t __gid) throw ();





extern int getresuid (__uid_t *__ruid, __uid_t *__euid, __uid_t *__suid)
     throw ();



extern int getresgid (__gid_t *__rgid, __gid_t *__egid, __gid_t *__sgid)
     throw ();



extern int setresuid (__uid_t __ruid, __uid_t __euid, __uid_t __suid)
     throw ();



extern int setresgid (__gid_t __rgid, __gid_t __egid, __gid_t __sgid)
     throw ();






extern __pid_t fork (void) throw ();







extern __pid_t vfork (void) throw ();





extern char *ttyname (int __fd) throw ();



extern int ttyname_r (int __fd, char *__buf, size_t __buflen)
     throw () __attribute__ ((__nonnull__ (2))) ;



extern int isatty (int __fd) throw ();





extern int ttyslot (void) throw ();




extern int link (__const char *__from, __const char *__to)
     throw () __attribute__ ((__nonnull__ (1, 2))) ;




extern int linkat (int __fromfd, __const char *__from, int __tofd,
     __const char *__to, int __flags)
     throw () __attribute__ ((__nonnull__ (2, 4))) ;




extern int symlink (__const char *__from, __const char *__to)
     throw () __attribute__ ((__nonnull__ (1, 2))) ;




extern ssize_t readlink (__const char *__restrict __path,
    char *__restrict __buf, size_t __len)
     throw () __attribute__ ((__nonnull__ (1, 2))) ;




extern int symlinkat (__const char *__from, int __tofd,
        __const char *__to) throw () __attribute__ ((__nonnull__ (1, 3))) ;


extern ssize_t readlinkat (int __fd, __const char *__restrict __path,
      char *__restrict __buf, size_t __len)
     throw () __attribute__ ((__nonnull__ (2, 3))) ;



extern int unlink (__const char *__name) throw () __attribute__ ((__nonnull__ (1)));



extern int unlinkat (int __fd, __const char *__name, int __flag)
     throw () __attribute__ ((__nonnull__ (2)));



extern int rmdir (__const char *__path) throw () __attribute__ ((__nonnull__ (1)));



extern __pid_t tcgetpgrp (int __fd) throw ();


extern int tcsetpgrp (int __fd, __pid_t __pgrp_id) throw ();






extern char *getlogin (void);







extern int getlogin_r (char *__name, size_t __name_len) __attribute__ ((__nonnull__ (1)));




extern int setlogin (__const char *__name) throw () __attribute__ ((__nonnull__ (1)));
# 894 "/usr/include/unistd.h" 3 4
# 1 "/usr/include/getopt.h" 1 3 4
# 50 "/usr/include/getopt.h" 3 4
extern "C" {
# 59 "/usr/include/getopt.h" 3 4
extern char *optarg;
# 73 "/usr/include/getopt.h" 3 4
extern int optind;




extern int opterr;



extern int optopt;
# 152 "/usr/include/getopt.h" 3 4
extern int getopt (int ___argc, char *const *___argv, const char *__shortopts)
       throw ();
# 187 "/usr/include/getopt.h" 3 4
}
# 895 "/usr/include/unistd.h" 2 3 4







extern int gethostname (char *__name, size_t __len) throw () __attribute__ ((__nonnull__ (1)));






extern int sethostname (__const char *__name, size_t __len)
     throw () __attribute__ ((__nonnull__ (1))) ;



extern int sethostid (long int __id) throw () ;





extern int getdomainname (char *__name, size_t __len)
     throw () __attribute__ ((__nonnull__ (1))) ;
extern int setdomainname (__const char *__name, size_t __len)
     throw () __attribute__ ((__nonnull__ (1))) ;





extern int vhangup (void) throw ();


extern int revoke (__const char *__file) throw () __attribute__ ((__nonnull__ (1))) ;







extern int profil (unsigned short int *__sample_buffer, size_t __size,
     size_t __offset, unsigned int __scale)
     throw () __attribute__ ((__nonnull__ (1)));





extern int acct (__const char *__name) throw ();



extern char *getusershell (void) throw ();
extern void endusershell (void) throw ();
extern void setusershell (void) throw ();





extern int daemon (int __nochdir, int __noclose) throw () ;






extern int chroot (__const char *__path) throw () __attribute__ ((__nonnull__ (1))) ;



extern char *getpass (__const char *__prompt) __attribute__ ((__nonnull__ (1)));
# 980 "/usr/include/unistd.h" 3 4
extern int fsync (int __fd);






extern int syncfs (int __fd) throw ();






extern long int gethostid (void);


extern void sync (void) throw ();





extern int getpagesize (void) throw () __attribute__ ((__const__));




extern int getdtablesize (void) throw ();
# 1018 "/usr/include/unistd.h" 3 4
extern int truncate (__const char *__file, __off_t __length)
     throw () __attribute__ ((__nonnull__ (1))) ;
# 1030 "/usr/include/unistd.h" 3 4
extern int truncate64 (__const char *__file, __off64_t __length)
     throw () __attribute__ ((__nonnull__ (1))) ;
# 1040 "/usr/include/unistd.h" 3 4
extern int ftruncate (int __fd, __off_t __length) throw () ;
# 1050 "/usr/include/unistd.h" 3 4
extern int ftruncate64 (int __fd, __off64_t __length) throw () ;
# 1061 "/usr/include/unistd.h" 3 4
extern int brk (void *__addr) throw () ;





extern void *sbrk (intptr_t __delta) throw ();
# 1082 "/usr/include/unistd.h" 3 4
extern long int syscall (long int __sysno, ...) throw ();
# 1105 "/usr/include/unistd.h" 3 4
extern int lockf (int __fd, int __cmd, __off_t __len) ;
# 1115 "/usr/include/unistd.h" 3 4
extern int lockf64 (int __fd, int __cmd, __off64_t __len) ;
# 1136 "/usr/include/unistd.h" 3 4
extern int fdatasync (int __fildes);







extern char *crypt (__const char *__key, __const char *__salt)
     throw () __attribute__ ((__nonnull__ (1, 2)));



extern void encrypt (char *__libc_block, int __edflag) throw () __attribute__ ((__nonnull__ (1)));






extern void swab (__const void *__restrict __from, void *__restrict __to,
    ssize_t __n) throw () __attribute__ ((__nonnull__ (1, 2)));







extern char *ctermid (char *__s) throw ();
# 1174 "/usr/include/unistd.h" 3 4
}
# 37 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/linux_common.h" 2

# 1 "/usr/include/x86_64-linux-gnu/sys/syscall.h" 1 3 4
# 25 "/usr/include/x86_64-linux-gnu/sys/syscall.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/asm/unistd.h" 1 3 4



# 1 "/usr/include/x86_64-linux-gnu/asm/unistd_64.h" 1 3 4
# 5 "/usr/include/x86_64-linux-gnu/asm/unistd.h" 2 3 4
# 26 "/usr/include/x86_64-linux-gnu/sys/syscall.h" 2 3 4






# 1 "/usr/include/x86_64-linux-gnu/bits/syscall.h" 1 3 4






# 1 "/usr/include/x86_64-linux-gnu/bits/wordsize.h" 1 3 4
# 8 "/usr/include/x86_64-linux-gnu/bits/syscall.h" 2 3 4
# 33 "/usr/include/x86_64-linux-gnu/sys/syscall.h" 2 3 4
# 39 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/linux_common.h" 2





# 1 "/usr/include/errno.h" 1 3 4
# 32 "/usr/include/errno.h" 3 4
extern "C" {




# 1 "/usr/include/x86_64-linux-gnu/bits/errno.h" 1 3 4
# 25 "/usr/include/x86_64-linux-gnu/bits/errno.h" 3 4
# 1 "/usr/include/linux/errno.h" 1 3 4



# 1 "/usr/include/x86_64-linux-gnu/asm/errno.h" 1 3 4
# 1 "/usr/include/asm-generic/errno.h" 1 3 4



# 1 "/usr/include/asm-generic/errno-base.h" 1 3 4
# 5 "/usr/include/asm-generic/errno.h" 2 3 4
# 2 "/usr/include/x86_64-linux-gnu/asm/errno.h" 2 3 4
# 5 "/usr/include/linux/errno.h" 2 3 4
# 26 "/usr/include/x86_64-linux-gnu/bits/errno.h" 2 3 4
# 47 "/usr/include/x86_64-linux-gnu/bits/errno.h" 3 4
extern int *__errno_location (void) throw () __attribute__ ((__const__));
# 37 "/usr/include/errno.h" 2 3 4
# 55 "/usr/include/errno.h" 3 4
extern char *program_invocation_name, *program_invocation_short_name;



}
# 69 "/usr/include/errno.h" 3 4
typedef int error_t;
# 45 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/linux_common.h" 2
# 63 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/machine/linux_common.h"
namespace tbb {

namespace internal {

inline int futex_wait( void *futex, int comparand ) {
    int r = syscall( 202,futex,0,comparand,__null,__null,0 );




    return r;
}

inline int futex_wakeup_one( void *futex ) {
    int r = ::syscall( 202,futex,1,1,__null,__null,0 );
    ((void)0);
    return r;
}

inline int futex_wakeup_all( void *futex ) {
    int r = ::syscall( 202,futex,1,2147483647,__null,__null,0 );
    ((void)0);
    return r;
}

}

}
# 257 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h" 2
# 347 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
namespace tbb {


inline void atomic_fence () { __asm__ __volatile__("mfence": : :"memory"); }

namespace internal {



class atomic_backoff : no_copy {



    static const int32_t LOOPS_BEFORE_YIELD = 16;
    int32_t count;
public:



    atomic_backoff() : count(1) {}

    atomic_backoff( bool ) : count(1) { pause(); }


    void pause() {
        if( count<=LOOPS_BEFORE_YIELD ) {
            __TBB_machine_pause(count);

            count*=2;
        } else {

            sched_yield();
        }
    }


    bool bounded_pause() {
        if( count<=LOOPS_BEFORE_YIELD ) {
            __TBB_machine_pause(count);

            count*=2;
            return true;
        } else {
            return false;
        }
    }

    void reset() {
        count = 1;
    }
};



template<typename T, typename U>
void spin_wait_while_eq( const volatile T& location, U value ) {
    atomic_backoff backoff;
    while( location==value ) backoff.pause();
}



template<typename T, typename U>
void spin_wait_until_eq( const volatile T& location, const U value ) {
    atomic_backoff backoff;
    while( location!=value ) backoff.pause();
}
# 437 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
template<typename T>
inline T __TBB_MaskedCompareAndSwap (volatile T * const ptr, const T value, const T comparand ) {
    struct endianness{ static bool is_big_endian(){




            return 0==1;



    }};

    const uint32_t byte_offset = (uint32_t) ((uintptr_t)ptr & 0x3);
    volatile uint32_t * const aligned_ptr = (uint32_t*)((uintptr_t)ptr - byte_offset );


    const uint32_t bits_to_shift = 8*(endianness::is_big_endian() ? (4 - sizeof(T) - (byte_offset)) : byte_offset);
    const uint32_t mask = (((uint32_t)1<<(sizeof(T)*8)) - 1 )<<bits_to_shift;

    const uint32_t shifted_comparand = ((uint32_t)comparand << bits_to_shift)&mask;
    const uint32_t shifted_value = ((uint32_t)value << bits_to_shift)&mask;

    for( atomic_backoff b;;b.pause() ) {
        const uint32_t surroundings = *aligned_ptr & ~mask ;
        const uint32_t big_comparand = surroundings | shifted_comparand ;
        const uint32_t big_value = surroundings | shifted_value ;


        const uint32_t big_result = (uint32_t)__TBB_machine_cmpswp4( aligned_ptr, big_value, big_comparand );
        if( big_result == big_comparand
          || ((big_result ^ big_comparand) & mask) != 0)
        {
            return T((big_result & mask) >> bits_to_shift);
        }
        else continue;
    }
}



template<size_t S, typename T>
inline T __TBB_CompareAndSwapGeneric (volatile void *ptr, T value, T comparand );

template<>
inline uint8_t __TBB_CompareAndSwapGeneric <1,uint8_t> (volatile void *ptr, uint8_t value, uint8_t comparand ) {



    return __TBB_machine_cmpswp1(ptr,value,comparand);

}

template<>
inline uint16_t __TBB_CompareAndSwapGeneric <2,uint16_t> (volatile void *ptr, uint16_t value, uint16_t comparand ) {



    return __TBB_machine_cmpswp2(ptr,value,comparand);

}

template<>
inline uint32_t __TBB_CompareAndSwapGeneric <4,uint32_t> (volatile void *ptr, uint32_t value, uint32_t comparand ) {

    return (uint32_t)__TBB_machine_cmpswp4(ptr,value,comparand);
}


template<>
inline uint64_t __TBB_CompareAndSwapGeneric <8,uint64_t> (volatile void *ptr, uint64_t value, uint64_t comparand ) {
    return __TBB_machine_cmpswp8(ptr,value,comparand);
}


template<size_t S, typename T>
inline T __TBB_FetchAndAddGeneric (volatile void *ptr, T addend) {
    T result;
    for( atomic_backoff b;;b.pause() ) {
        result = *reinterpret_cast<volatile T *>(ptr);

        if( __TBB_CompareAndSwapGeneric<S,T> ( ptr, result+addend, result )==result )
            break;
    }
    return result;
}

template<size_t S, typename T>
inline T __TBB_FetchAndStoreGeneric (volatile void *ptr, T value) {
    T result;
    for( atomic_backoff b;;b.pause() ) {
        result = *reinterpret_cast<volatile T *>(ptr);

        if( __TBB_CompareAndSwapGeneric<S,T> ( ptr, value, result )==result )
            break;
    }
    return result;
}
# 573 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
atomic_selector<1>::word atomic_selector<1>::fetch_store ( volatile void* location, word value ) { return __TBB_machine_fetchstore1( location, value ); }
atomic_selector<2>::word atomic_selector<2>::fetch_store ( volatile void* location, word value ) { return __TBB_machine_fetchstore2( location, value ); }
atomic_selector<4>::word atomic_selector<4>::fetch_store ( volatile void* location, word value ) { return __TBB_machine_fetchstore4( location, value ); }
atomic_selector<8>::word atomic_selector<8>::fetch_store ( volatile void* location, word value ) { return __TBB_machine_fetchstore8( location, value ); }
# 610 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
template <typename T, size_t S>
struct machine_load_store {
    static T load_with_acquire ( const volatile T& location ) {
        T to_return = location;
        __asm__ __volatile__("": : :"memory");
        return to_return;
    }
    static void store_with_release ( volatile T &location, T value ) {
        __asm__ __volatile__("": : :"memory");
        location = value;
    }
};
# 638 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
template <typename T, size_t S>
struct machine_load_store_seq_cst {
    static T load ( const volatile T& location ) {
        __asm__ __volatile__("mfence": : :"memory");
        return machine_load_store<T,S>::load_with_acquire( location );
    }

    static void store ( volatile T &location, T value ) {
        atomic_selector<S>::fetch_store( (volatile void*)&location, (typename atomic_selector<S>::word)value );
    }






};
# 681 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
template <typename T, size_t S>
struct machine_load_store_relaxed {
    static inline T load ( const volatile T& location ) {
        return location;
    }
    static inline void store ( volatile T& location, T value ) {
        location = value;
    }
};
# 706 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
template<typename T>
inline T __TBB_load_with_acquire(const volatile T &location) {
    return machine_load_store<T,sizeof(T)>::load_with_acquire( location );
}
template<typename T, typename V>
inline void __TBB_store_with_release(volatile T& location, V value) {
    machine_load_store<T,sizeof(T)>::store_with_release( location, T(value) );
}

inline void __TBB_store_with_release(volatile size_t& location, size_t value) {
    machine_load_store<size_t,sizeof(size_t)>::store_with_release( location, value );
}

template<typename T>
inline T __TBB_load_full_fence(const volatile T &location) {
    return machine_load_store_seq_cst<T,sizeof(T)>::load( location );
}
template<typename T, typename V>
inline void __TBB_store_full_fence(volatile T& location, V value) {
    machine_load_store_seq_cst<T,sizeof(T)>::store( location, T(value) );
}

inline void __TBB_store_full_fence(volatile size_t& location, size_t value) {
    machine_load_store_seq_cst<size_t,sizeof(size_t)>::store( location, value );
}

template<typename T>
inline T __TBB_load_relaxed (const volatile T& location) {
    return machine_load_store_relaxed<T,sizeof(T)>::load( const_cast<T&>(location) );
}
template<typename T, typename V>
inline void __TBB_store_relaxed ( volatile T& location, V value ) {
    machine_load_store_relaxed<T,sizeof(T)>::store( const_cast<T&>(location), T(value) );
}

inline void __TBB_store_relaxed ( volatile size_t& location, size_t value ) {
    machine_load_store_relaxed<size_t,sizeof(size_t)>::store( const_cast<size_t&>(location), value );
}
# 777 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
struct __TBB_machine_type_with_alignment_16 { uint32_t member[16/sizeof(uint32_t)]; } __attribute__((aligned(16)));
struct __TBB_machine_type_with_alignment_32 { uint32_t member[32/sizeof(uint32_t)]; } __attribute__((aligned(32)));
struct __TBB_machine_type_with_alignment_64 { uint32_t member[64/sizeof(uint32_t)]; } __attribute__((aligned(64)));

typedef __TBB_machine_type_with_alignment_64 __TBB_machine_type_with_strictest_alignment;


template<size_t N> struct type_with_alignment;


template<> struct type_with_alignment<1> { char member; };
template<> struct type_with_alignment<2> { uint16_t member; };
template<> struct type_with_alignment<4> { uint32_t member; };
template<> struct type_with_alignment<8> { uint64_t member; };
template<> struct type_with_alignment<16> {__TBB_machine_type_with_alignment_16 member; };
template<> struct type_with_alignment<32> {__TBB_machine_type_with_alignment_32 member; };
template<> struct type_with_alignment<64> {__TBB_machine_type_with_alignment_64 member; };
# 811 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
template<typename T>
struct reverse {
    static const T byte_table[256];
};


template<typename T>
const T reverse<T>::byte_table[256] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

}
}


using tbb::internal::__TBB_load_with_acquire;
using tbb::internal::__TBB_store_with_release;
# 897 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tbb_machine.h"
typedef unsigned char __TBB_Flag;

typedef __TBB_Flag __TBB_atomic_flag;


inline bool __TBB_TryLockByte( __TBB_atomic_flag &flag ) {
    return __TBB_machine_cmpswp1(&flag,1,0)==0;
}



inline __TBB_Flag __TBB_LockByte( __TBB_atomic_flag& flag ) {
    tbb::internal::atomic_backoff backoff;
    while( !__TBB_TryLockByte(flag) ) backoff.pause();
    return 0;
}







inline unsigned char __TBB_ReverseByte(unsigned char src) {
    return tbb::internal::reverse<unsigned char>::byte_table[src];
}


template<typename T>
T __TBB_ReverseBits(T src) {
    T dst;
    unsigned char *original = (unsigned char *) &src;
    unsigned char *reversed = (unsigned char *) &dst;

    for( int i = sizeof(T)-1; i >= 0; i-- )
        reversed[i] = __TBB_ReverseByte( original[sizeof(T)-i-1] );

    return dst;
}
# 34 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h" 2
# 1 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/climits" 1 3
# 42 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/climits" 3
# 35 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h" 2

typedef struct ___itt_caller *__itt_caller;

namespace tbb {

class task;
class task_list;


class task_group_context;
# 55 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
namespace internal {

    class allocate_additional_child_of_proxy: no_assign {

        task* self;
        task& parent;
    public:
        explicit allocate_additional_child_of_proxy( task& parent_ ) : self(__null), parent(parent_) {}
        task& allocate( size_t size ) const;
        void free( task& ) const;
    };

}

namespace interface5 {
    namespace internal {






        class task_base: tbb::internal::no_copy {
        private:
            friend class tbb::task;


            static void spawn( task& t );


            static void spawn( task_list& list );




            static tbb::internal::allocate_additional_child_of_proxy allocate_additional_child_of( task& t ) {
                return tbb::internal::allocate_additional_child_of_proxy(t);
            }






            static void destroy( task& victim );
        };
    }
}


namespace internal {

    class scheduler: no_copy {
    public:

        virtual void spawn( task& first, task*& next ) = 0;


        virtual void wait_for_all( task& parent, task* child ) = 0;


        virtual void spawn_root_and_wait( task& first, task*& next ) = 0;



        virtual ~scheduler() = 0;


        virtual void enqueue( task& t, void* reserved ) = 0;
    };



    typedef intptr_t reference_count;


    typedef unsigned short affinity_id;


    class generic_scheduler;

    struct context_list_node_t {
        context_list_node_t *my_prev,
                            *my_next;
    };

    class allocate_root_with_context_proxy: no_assign {
        task_group_context& my_context;
    public:
        allocate_root_with_context_proxy ( task_group_context& ctx ) : my_context(ctx) {}
        task& allocate( size_t size ) const;
        void free( task& ) const;
    };


    class allocate_root_proxy: no_assign {
    public:
        static task& allocate( size_t size );
        static void free( task& );
    };

    class allocate_continuation_proxy: no_assign {
    public:
        task& allocate( size_t size ) const;
        void free( task& ) const;
    };

    class allocate_child_proxy: no_assign {
    public:
        task& allocate( size_t size ) const;
        void free( task& ) const;
    };
# 180 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
    class task_prefix {
    private:
        friend class tbb::task;
        friend class tbb::interface5::internal::task_base;
        friend class tbb::task_list;
        friend class internal::scheduler;
        friend class internal::allocate_root_proxy;
        friend class internal::allocate_child_proxy;
        friend class internal::allocate_continuation_proxy;
        friend class internal::allocate_additional_child_of_proxy;






        task_group_context *context;
# 205 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
        scheduler* origin;


        union {




        scheduler* owner;




        task* next_offloaded;
        };






        tbb::task* parent;






                     reference_count ref_count;




        int depth;



        unsigned char state;
# 251 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
        unsigned char extra_state;

        affinity_id affinity;


        tbb::task* next;


        tbb::task& task() {return *reinterpret_cast<tbb::task*>(this+1);}
    };

}





namespace internal {
    static const int priority_stride_v4 = 2147483647 / 4;
}

enum priority_t {
    priority_normal = internal::priority_stride_v4 * 2,
    priority_low = priority_normal - internal::priority_stride_v4,
    priority_high = priority_normal + internal::priority_stride_v4
};






    namespace internal {
        class tbb_exception_ptr;
    }


class task_scheduler_init;
# 311 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
class task_group_context : internal::no_copy {
private:
    friend class internal::generic_scheduler;
    friend class task_scheduler_init;




    typedef internal::tbb_exception_ptr exception_container_type;


    enum version_traits_word_layout {
        traits_offset = 16,
        version_mask = 0xFFFF,
        traits_mask = 0xFFFFul << traits_offset
    };

public:
    enum kind_type {
        isolated,
        bound
    };

    enum traits_type {
        exact_exception = 0x0001ul << traits_offset,
        concurrent_wait = 0x0004ul << traits_offset,



        default_traits = exact_exception

    };

private:
    enum state {
        may_have_children = 1
    };

    union {

        kind_type my_kind;
        uintptr_t _my_kind_aligner;
    };


    task_group_context *my_parent;




    internal::context_list_node_t my_node;


    __itt_caller itt_caller;





    char _leading_padding[internal::NFS_MaxLineSize
                          - 2 * sizeof(uintptr_t)- sizeof(void*) - sizeof(internal::context_list_node_t)
                          - sizeof(__itt_caller)];


    uintptr_t my_cancellation_requested;





    uintptr_t my_version_and_traits;


    exception_container_type *my_exception;


    internal::generic_scheduler *my_owner;


    uintptr_t my_state;



    intptr_t my_priority;




    char _trailing_padding[internal::NFS_MaxLineSize - 2 * sizeof(uintptr_t) - 2 * sizeof(void*)

                            - sizeof(intptr_t)

                          ];

public:
# 435 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
    task_group_context ( kind_type relation_with_parent = bound,
                         uintptr_t traits = default_traits )
        : my_kind(relation_with_parent)
        , my_version_and_traits(1 | traits)
    {
        init();
    }

                          ~task_group_context ();
# 453 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
    void reset ();
# 463 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
    bool cancel_group_execution ();


    bool is_group_execution_cancelled () const;
# 475 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
    void register_pending_exception ();



    void set_priority ( priority_t );


    priority_t priority () const;


protected:


    void init ();

private:
    friend class task;
    friend class internal::allocate_root_with_context_proxy;

    static const kind_type binding_required = bound;
    static const kind_type binding_completed = kind_type(bound+1);
    static const kind_type detached = kind_type(binding_completed+1);
    static const kind_type dying = kind_type(detached+1);




    template <typename T>
    void propagate_state_from_ancestors ( T task_group_context::*mptr_state, T new_state );


    inline void finish_initialization ( internal::generic_scheduler *local_sched );


    void bind_to ( internal::generic_scheduler *local_sched );


    void register_with ( internal::generic_scheduler *local_sched );

};





class task: private interface5::internal::task_base {


    void internal_set_ref_count( int count );


    internal::reference_count internal_decrement_ref_count();

protected:

    task() {prefix().extra_state=1;}

public:

    virtual ~task() {}


    virtual task* execute() = 0;


    enum state_type {

        executing,

        reexecute,

        ready,

        allocated,

        freed,

        recycle




    };






    static internal::allocate_root_proxy allocate_root() {
        return internal::allocate_root_proxy();
    }



    static internal::allocate_root_with_context_proxy allocate_root( task_group_context& ctx ) {
        return internal::allocate_root_with_context_proxy(ctx);
    }




    internal::allocate_continuation_proxy& allocate_continuation() {
        return *reinterpret_cast<internal::allocate_continuation_proxy*>(this);
    }


    internal::allocate_child_proxy& allocate_child() {
        return *reinterpret_cast<internal::allocate_child_proxy*>(this);
    }


    using task_base::allocate_additional_child_of;
# 598 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
    using task_base::destroy;
# 612 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
    void recycle_as_continuation() {
        ((void)0);
        prefix().state = allocated;
    }




    void recycle_as_safe_continuation() {
        ((void)0);
        prefix().state = recycle;
    }


    void recycle_as_child_of( task& new_parent ) {
        internal::task_prefix& p = prefix();
        ((void)0);
        ((void)0);
        ((void)0);
        ((void)0);
        ((void)0);
        p.state = allocated;
        p.parent = &new_parent;

        p.context = new_parent.prefix().context;

    }



    void recycle_to_reexecute() {
        ((void)0);
        ((void)0);
        prefix().state = reexecute;
    }
# 659 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
    intptr_t depth() const {return 0;}
    void set_depth( intptr_t ) {}
    void add_to_depth( int ) {}







    void set_ref_count( int count ) {



        prefix().ref_count = count;

    }



    void increment_ref_count() {
        __TBB_machine_fetchadd8(&prefix().ref_count,1);
    }



    int decrement_ref_count() {



        return int(__TBB_machine_fetchadd8(&prefix().ref_count,(-1)))-1;

    }


    using task_base::spawn;


    void spawn_and_wait_for_all( task& child ) {
        prefix().owner->wait_for_all( *this, &child );
    }


    void spawn_and_wait_for_all( task_list& list );


    static void spawn_root_and_wait( task& root ) {
        root.prefix().owner->spawn_root_and_wait( root, root.prefix().next );
    }




    static void spawn_root_and_wait( task_list& root_list );



    void wait_for_all() {
        prefix().owner->wait_for_all( *this, __null );
    }
# 733 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
    static void enqueue( task& t ) {
        t.prefix().owner->enqueue( t, __null );
    }



    static void enqueue( task& t, priority_t p ) {
        ((void)0);
        t.prefix().owner->enqueue( t, (void*)p );
    }



    static task& self();


    task* parent() const {return prefix().parent;}


    void set_parent(task* p) {

        ((void)0);

        prefix().parent = p;
    }




    task_group_context* context() {return prefix().context;}


    task_group_context* group () { return prefix().context; }



    bool is_stolen_task() const {
        return (prefix().extra_state & 0x80)!=0;
    }






    state_type state() const {return state_type(prefix().state);}


    int ref_count() const {




        return int(prefix().ref_count);
    }


    bool is_owned_by_current_thread() const;







    typedef internal::affinity_id affinity_id;


    void set_affinity( affinity_id id ) {prefix().affinity = id;}


    affinity_id affinity() const {return prefix().affinity;}






    virtual void note_affinity( affinity_id id );
# 825 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/task.h"
    void change_group ( task_group_context& ctx );



    bool cancel_group_execution () { return prefix().context->cancel_group_execution(); }


    bool is_cancelled () const { return prefix().context->is_group_execution_cancelled(); }






    void set_group_priority ( priority_t p ) { prefix().context->set_priority(p); }


    priority_t group_priority () const { return prefix().context->priority(); }



private:
    friend class interface5::internal::task_base;
    friend class task_list;
    friend class internal::scheduler;
    friend class internal::allocate_root_proxy;

    friend class internal::allocate_root_with_context_proxy;

    friend class internal::allocate_continuation_proxy;
    friend class internal::allocate_child_proxy;
    friend class internal::allocate_additional_child_of_proxy;



    internal::task_prefix& prefix( internal::version_tag* = __null ) const {
        return reinterpret_cast<internal::task_prefix*>(const_cast<task*>(this))[-1];
    }
};



class empty_task: public task {
                 task* execute() {
        return __null;
    }
};




class task_list: internal::no_copy {
private:
    task* first;
    task** next_ptr;
    friend class task;
    friend class interface5::internal::task_base;
public:

    task_list() : first(__null), next_ptr(&first) {}


    ~task_list() {}


    bool empty() const {return !first;}


    void push_back( task& task ) {
        task.prefix().next = __null;
        *next_ptr = &task;
        next_ptr = &task.prefix().next;
    }


    task& pop_front() {
        ((void)0);
        task* result = first;
        first = result->prefix().next;
        if( !first ) next_ptr = &first;
        return *result;
    }


    void clear() {
        first=__null;
        next_ptr=&first;
    }
};

inline void interface5::internal::task_base::spawn( task& t ) {
    t.prefix().owner->spawn( t, t.prefix().next );
}

inline void interface5::internal::task_base::spawn( task_list& list ) {
    if( task* t = list.first ) {
        t->prefix().owner->spawn( *t, *list.next_ptr );
        list.clear();
    }
}

inline void task::spawn_root_and_wait( task_list& root_list ) {
    if( task* t = root_list.first ) {
        t->prefix().owner->spawn_root_and_wait( *t, *root_list.next_ptr );
        root_list.clear();
    }
}

}

inline void *operator new( size_t bytes, const tbb::internal::allocate_root_proxy& ) {
    return &tbb::internal::allocate_root_proxy::allocate(bytes);
}

inline void operator delete( void* task, const tbb::internal::allocate_root_proxy& ) {
    tbb::internal::allocate_root_proxy::free( *static_cast<tbb::task*>(task) );
}


inline void *operator new( size_t bytes, const tbb::internal::allocate_root_with_context_proxy& p ) {
    return &p.allocate(bytes);
}

inline void operator delete( void* task, const tbb::internal::allocate_root_with_context_proxy& p ) {
    p.free( *static_cast<tbb::task*>(task) );
}


inline void *operator new( size_t bytes, const tbb::internal::allocate_continuation_proxy& p ) {
    return &p.allocate(bytes);
}

inline void operator delete( void* task, const tbb::internal::allocate_continuation_proxy& p ) {
    p.free( *static_cast<tbb::task*>(task) );
}

inline void *operator new( size_t bytes, const tbb::internal::allocate_child_proxy& p ) {
    return &p.allocate(bytes);
}

inline void operator delete( void* task, const tbb::internal::allocate_child_proxy& p ) {
    p.free( *static_cast<tbb::task*>(task) );
}

inline void *operator new( size_t bytes, const tbb::internal::allocate_additional_child_of_proxy& p ) {
    return &p.allocate(bytes);
}

inline void operator delete( void* task, const tbb::internal::allocate_additional_child_of_proxy& p ) {
    p.free( *static_cast<tbb::task*>(task) );
}
# 33 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/parallel_invoke.h" 2

namespace tbb {







namespace internal {

    template<typename function>
    class function_invoker : public task{
    public:
        function_invoker(const function& _function) : my_function(_function) {}
    private:
        const function &my_function;

        task* execute()
        {
            my_function();
            return __null;
        }
    };


    template <size_t N, typename function1, typename function2, typename function3>
    class spawner : public task {
    private:
        const function1& my_func1;
        const function2& my_func2;
        const function3& my_func3;
        bool is_recycled;

        task* execute (){
            if(is_recycled){
                return __null;
            }else{
                ((void)0);
                set_ref_count(N);
                recycle_as_safe_continuation();
                internal::function_invoker<function2>* invoker2 = new (allocate_child()) internal::function_invoker<function2>(my_func2);
                ((void)0);
                spawn(*invoker2);
                size_t n = N;
                if (n>2) {
                    internal::function_invoker<function3>* invoker3 = new (allocate_child()) internal::function_invoker<function3>(my_func3);
                    ((void)0);
                    spawn(*invoker3);
                }
                my_func1();
                is_recycled = true;
                return __null;
            }
        }

    public:
        spawner(const function1& _func1, const function2& _func2, const function3& _func3) : my_func1(_func1), my_func2(_func2), my_func3(_func3), is_recycled(false) {}
    };


    class parallel_invoke_helper : public empty_task {
    public:

        class parallel_invoke_noop {
        public:
            void operator() () const {}
        };

        parallel_invoke_helper(int number_of_children)
        {
            set_ref_count(number_of_children + 1);
        }

        template <typename function>
        void add_child (const function &_func)
        {
            internal::function_invoker<function>* invoker = new (allocate_child()) internal::function_invoker<function>(_func);
            ((void)0);
            spawn(*invoker);
        }



        template <typename function1, typename function2>
        void add_children (const function1& _func1, const function2& _func2)
        {

            parallel_invoke_noop noop;
            internal::spawner<2, function1, function2, parallel_invoke_noop>& sub_root = *new(allocate_child())internal::spawner<2, function1, function2, parallel_invoke_noop>(_func1, _func2, noop);
            spawn(sub_root);
        }

        template <typename function1, typename function2, typename function3>
        void add_children (const function1& _func1, const function2& _func2, const function3& _func3)
        {
            internal::spawner<3, function1, function2, function3>& sub_root = *new(allocate_child())internal::spawner<3, function1, function2, function3>(_func1, _func2, _func3);
            spawn(sub_root);
        }


        template <typename F0>
        void run_and_finish(const F0& f0)
        {
            internal::function_invoker<F0>* invoker = new (allocate_child()) internal::function_invoker<F0>(f0);
            ((void)0);
            spawn_and_wait_for_all(*invoker);
        }
    };

    class parallel_invoke_cleaner: internal::no_copy {
    public:

        parallel_invoke_cleaner(int number_of_children, tbb::task_group_context& context)
            : root(*new(task::allocate_root(context)) internal::parallel_invoke_helper(number_of_children))




        {}

        ~parallel_invoke_cleaner(){
            root.destroy(root);
        }
        internal::parallel_invoke_helper& root;
    };
}
# 170 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/parallel_invoke.h"
template<typename F0, typename F1 >
void parallel_invoke(const F0& f0, const F1& f1, tbb::task_group_context& context) {
    internal::parallel_invoke_cleaner cleaner(2, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_child(f1);

    root.run_and_finish(f0);
}


template<typename F0, typename F1, typename F2 >
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, tbb::task_group_context& context) {
    internal::parallel_invoke_cleaner cleaner(3, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_child(f2);
    root.add_child(f1);

    root.run_and_finish(f0);
}


template<typename F0, typename F1, typename F2, typename F3>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3,
                     tbb::task_group_context& context)
{
    internal::parallel_invoke_cleaner cleaner(4, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_child(f3);
    root.add_child(f2);
    root.add_child(f1);

    root.run_and_finish(f0);
}


template<typename F0, typename F1, typename F2, typename F3, typename F4 >
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3, const F4& f4,
                     tbb::task_group_context& context)
{
    internal::parallel_invoke_cleaner cleaner(3, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_children(f4, f3);
    root.add_children(f2, f1);

    root.run_and_finish(f0);
}


template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3, const F4& f4, const F5& f5,
                     tbb::task_group_context& context)
{
    internal::parallel_invoke_cleaner cleaner(3, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_children(f5, f4, f3);
    root.add_children(f2, f1);

    root.run_and_finish(f0);
}


template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3, const F4& f4,
                     const F5& f5, const F6& f6,
                     tbb::task_group_context& context)
{
    internal::parallel_invoke_cleaner cleaner(3, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_children(f6, f5, f4);
    root.add_children(f3, f2, f1);

    root.run_and_finish(f0);
}


template<typename F0, typename F1, typename F2, typename F3, typename F4,
         typename F5, typename F6, typename F7>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3, const F4& f4,
                     const F5& f5, const F6& f6, const F7& f7,
                     tbb::task_group_context& context)
{
    internal::parallel_invoke_cleaner cleaner(4, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_children(f7, f6, f5);
    root.add_children(f4, f3);
    root.add_children(f2, f1);

    root.run_and_finish(f0);
}


template<typename F0, typename F1, typename F2, typename F3, typename F4,
         typename F5, typename F6, typename F7, typename F8>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3, const F4& f4,
                     const F5& f5, const F6& f6, const F7& f7, const F8& f8,
                     tbb::task_group_context& context)
{
    internal::parallel_invoke_cleaner cleaner(4, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_children(f8, f7, f6);
    root.add_children(f5, f4, f3);
    root.add_children(f2, f1);

    root.run_and_finish(f0);
}


template<typename F0, typename F1, typename F2, typename F3, typename F4,
         typename F5, typename F6, typename F7, typename F8, typename F9>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3, const F4& f4,
                     const F5& f5, const F6& f6, const F7& f7, const F8& f8, const F9& f9,
                     tbb::task_group_context& context)
{
    internal::parallel_invoke_cleaner cleaner(4, context);
    internal::parallel_invoke_helper& root = cleaner.root;

    root.add_children(f9, f8, f7);
    root.add_children(f6, f5, f4);
    root.add_children(f3, f2, f1);

    root.run_and_finish(f0);
}


template<typename F0, typename F1>
void parallel_invoke(const F0& f0, const F1& f1) {
    task_group_context context;
    parallel_invoke<F0, F1>(f0, f1, context);
}

template<typename F0, typename F1, typename F2>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2) {
    task_group_context context;
    parallel_invoke<F0, F1, F2>(f0, f1, f2, context);
}

template<typename F0, typename F1, typename F2, typename F3 >
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3) {
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3>(f0, f1, f2, f3, context);
}

template<typename F0, typename F1, typename F2, typename F3, typename F4>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3, const F4& f4) {
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3, F4>(f0, f1, f2, f3, f4, context);
}

template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3, const F4& f4, const F5& f5) {
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3, F4, F5>(f0, f1, f2, f3, f4, f5, context);
}

template<typename F0, typename F1, typename F2, typename F3, typename F4, typename F5, typename F6>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3, const F4& f4,
                     const F5& f5, const F6& f6)
{
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3, F4, F5, F6>(f0, f1, f2, f3, f4, f5, f6, context);
}

template<typename F0, typename F1, typename F2, typename F3, typename F4,
         typename F5, typename F6, typename F7>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3, const F4& f4,
                     const F5& f5, const F6& f6, const F7& f7)
{
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3, F4, F5, F6, F7>(f0, f1, f2, f3, f4, f5, f6, f7, context);
}

template<typename F0, typename F1, typename F2, typename F3, typename F4,
         typename F5, typename F6, typename F7, typename F8>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3, const F4& f4,
                     const F5& f5, const F6& f6, const F7& f7, const F8& f8)
{
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3, F4, F5, F6, F7, F8>(f0, f1, f2, f3, f4, f5, f6, f7, f8, context);
}

template<typename F0, typename F1, typename F2, typename F3, typename F4,
         typename F5, typename F6, typename F7, typename F8, typename F9>
void parallel_invoke(const F0& f0, const F1& f1, const F2& f2, const F3& f3, const F4& f4,
                     const F5& f5, const F6& f6, const F7& f7, const F8& f8, const F9& f9)
{
    task_group_context context;
    parallel_invoke<F0, F1, F2, F3, F4, F5, F6, F7, F8, F9>(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, context);
}



}
# 6 "tree-NI.cpp" 2
# 1 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tick_count.h" 1
# 37 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tick_count.h"
# 1 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/ctime" 1 3
# 42 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/ctime" 3


# 1 "/usr/include/time.h" 1 3 4
# 30 "/usr/include/time.h" 3 4
extern "C" {








# 1 "/home/atzannes/src/asp/github/build/Release+Asserts/bin/../lib/clang/3.4/include/stddef.h" 1 3 4
# 39 "/usr/include/time.h" 2 3 4



# 1 "/usr/include/x86_64-linux-gnu/bits/time.h" 1 3 4
# 86 "/usr/include/x86_64-linux-gnu/bits/time.h" 3 4
# 1 "/usr/include/x86_64-linux-gnu/bits/timex.h" 1 3 4
# 24 "/usr/include/x86_64-linux-gnu/bits/timex.h" 3 4
struct timex
{
  unsigned int modes;
  long int offset;
  long int freq;
  long int maxerror;
  long int esterror;
  int status;
  long int constant;
  long int precision;
  long int tolerance;
  struct timeval time;
  long int tick;

  long int ppsfreq;
  long int jitter;
  int shift;
  long int stabil;
  long int jitcnt;
  long int calcnt;
  long int errcnt;
  long int stbcnt;

  int tai;


  int :32; int :32; int :32; int :32;
  int :32; int :32; int :32; int :32;
  int :32; int :32; int :32;
};
# 87 "/usr/include/x86_64-linux-gnu/bits/time.h" 2 3 4

extern "C" {


extern int clock_adjtime (__clockid_t __clock_id, struct timex *__utx) throw ();

}
# 43 "/usr/include/time.h" 2 3 4
# 133 "/usr/include/time.h" 3 4
struct tm
{
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;


  long int tm_gmtoff;
  __const char *tm_zone;




};
# 161 "/usr/include/time.h" 3 4
struct itimerspec
  {
    struct timespec it_interval;
    struct timespec it_value;
  };


struct sigevent;
# 183 "/usr/include/time.h" 3 4
extern clock_t clock (void) throw ();


extern time_t time (time_t *__timer) throw ();


extern double difftime (time_t __time1, time_t __time0)
     throw () __attribute__ ((__const__));


extern time_t mktime (struct tm *__tp) throw ();





extern size_t strftime (char *__restrict __s, size_t __maxsize,
   __const char *__restrict __format,
   __const struct tm *__restrict __tp) throw ();





extern char *strptime (__const char *__restrict __s,
         __const char *__restrict __fmt, struct tm *__tp)
     throw ();







extern size_t strftime_l (char *__restrict __s, size_t __maxsize,
     __const char *__restrict __format,
     __const struct tm *__restrict __tp,
     __locale_t __loc) throw ();



extern char *strptime_l (__const char *__restrict __s,
    __const char *__restrict __fmt, struct tm *__tp,
    __locale_t __loc) throw ();






extern struct tm *gmtime (__const time_t *__timer) throw ();



extern struct tm *localtime (__const time_t *__timer) throw ();





extern struct tm *gmtime_r (__const time_t *__restrict __timer,
       struct tm *__restrict __tp) throw ();



extern struct tm *localtime_r (__const time_t *__restrict __timer,
          struct tm *__restrict __tp) throw ();





extern char *asctime (__const struct tm *__tp) throw ();


extern char *ctime (__const time_t *__timer) throw ();







extern char *asctime_r (__const struct tm *__restrict __tp,
   char *__restrict __buf) throw ();


extern char *ctime_r (__const time_t *__restrict __timer,
        char *__restrict __buf) throw ();




extern char *__tzname[2];
extern int __daylight;
extern long int __timezone;




extern char *tzname[2];



extern void tzset (void) throw ();



extern int daylight;
extern long int timezone;





extern int stime (__const time_t *__when) throw ();
# 313 "/usr/include/time.h" 3 4
extern time_t timegm (struct tm *__tp) throw ();


extern time_t timelocal (struct tm *__tp) throw ();


extern int dysize (int __year) throw () __attribute__ ((__const__));
# 328 "/usr/include/time.h" 3 4
extern int nanosleep (__const struct timespec *__requested_time,
        struct timespec *__remaining);



extern int clock_getres (clockid_t __clock_id, struct timespec *__res) throw ();


extern int clock_gettime (clockid_t __clock_id, struct timespec *__tp) throw ();


extern int clock_settime (clockid_t __clock_id, __const struct timespec *__tp)
     throw ();






extern int clock_nanosleep (clockid_t __clock_id, int __flags,
       __const struct timespec *__req,
       struct timespec *__rem);


extern int clock_getcpuclockid (pid_t __pid, clockid_t *__clock_id) throw ();




extern int timer_create (clockid_t __clock_id,
    struct sigevent *__restrict __evp,
    timer_t *__restrict __timerid) throw ();


extern int timer_delete (timer_t __timerid) throw ();


extern int timer_settime (timer_t __timerid, int __flags,
     __const struct itimerspec *__restrict __value,
     struct itimerspec *__restrict __ovalue) throw ();


extern int timer_gettime (timer_t __timerid, struct itimerspec *__value)
     throw ();


extern int timer_getoverrun (timer_t __timerid) throw ();
# 390 "/usr/include/time.h" 3 4
extern int getdate_err;
# 399 "/usr/include/time.h" 3 4
extern struct tm *getdate (__const char *__string);
# 413 "/usr/include/time.h" 3 4
extern int getdate_r (__const char *__restrict __string,
        struct tm *__restrict __resbufp);


}
# 45 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/ctime" 2 3
# 60 "/usr/lib/gcc/x86_64-linux-gnu/4.6/../../../../include/c++/4.6/ctime" 3
namespace std
{
  using ::clock_t;
  using ::time_t;
  using ::tm;

  using ::clock;
  using ::difftime;
  using ::mktime;
  using ::time;
  using ::asctime;
  using ::ctime;
  using ::gmtime;
  using ::localtime;
  using ::strftime;
}
# 38 "/home/atzannes/src/tbb/tbb41_20130613oss/include/tbb/tick_count.h" 2




namespace tbb {



class tick_count {
public:

    class interval_t {
        long long value;
        explicit interval_t( long long value_ ) : value(value_) {}
    public:

        interval_t() : value(0) {};


        explicit interval_t( double sec );


        double seconds() const;

        friend class tbb::tick_count;


        friend interval_t operator-( const tick_count& t1, const tick_count& t0 );


        friend interval_t operator+( const interval_t& i, const interval_t& j ) {
            return interval_t(i.value+j.value);
        }


        friend interval_t operator-( const interval_t& i, const interval_t& j ) {
            return interval_t(i.value-j.value);
        }


        interval_t& operator+=( const interval_t& i ) {value += i.value; return *this;}


        interval_t& operator-=( const interval_t& i ) {value -= i.value; return *this;}
    private:
        static long long ticks_per_second(){






            return static_cast<long long>(1E9);



        }
    };


    tick_count() : my_count(0) {};


    static tick_count now();


    friend interval_t operator-( const tick_count& t1, const tick_count& t0 );


    static double resolution() { return 1.0 / interval_t::ticks_per_second(); }

private:
    long long my_count;
};

inline tick_count tick_count::now() {
    tick_count result;






    struct timespec ts;
    int status = clock_gettime( 0, &ts );
    ((void)(1 && (status==0)));
    result.my_count = static_cast<long long>(1000000000UL)*static_cast<long long>(ts.tv_sec) + static_cast<long long>(ts.tv_nsec);






    return result;
}

inline tick_count::interval_t::interval_t( double sec ) {
    value = static_cast<long long>(sec*interval_t::ticks_per_second());
}

inline tick_count::interval_t operator-( const tick_count& t1, const tick_count& t0 ) {
    return tick_count::interval_t( t1.my_count-t0.my_count );
}

inline double tick_count::interval_t::seconds() const {
    return value*tick_count::resolution();
}

}
# 7 "tree-NI.cpp" 2





class TreeNode;
[[asap::region("ReadOnly")]];

class [[asap::param("P_gtl")]] GrowTreeLeft {
  TreeNode *Node [[asap::arg("ReadOnly, P_gtl")]];
  int Depth [[asap::arg("ReadOnly")]];

public:
  GrowTreeLeft(TreeNode *n[[asap::arg("P_gtl")]], int depth) : Node(n), Depth(depth) {}

  void operator() [[asap::reads("ReadOnly, P_gtl:*:TreeNode::V"),
                    asap::writes("P_gtl:TreeNode::L,"
                                 "P_gtl:TreeNode::L:*:TreeNode::L,"
                                 "P_gtl:TreeNode::L:*:TreeNode::R")]]
                  () const;

};

class [[asap::param("P_gtr")]] GrowTreeRight {
  TreeNode *Node [[asap::arg("ReadOnly, P_gtr")]];
  int Depth [[asap::arg("ReadOnly")]];

public:
  GrowTreeRight(TreeNode *n[[asap::arg("P_gtr")]], int depth) : Node(n), Depth(depth) {}

  void operator() [[asap::reads("ReadOnly, P_gtr:*:TreeNode::V"),
                    asap::writes("P_gtr:TreeNode::R,"
                                 "P_gtr:TreeNode::R:*:TreeNode::L,"
                                 "P_gtr:TreeNode::R:*:TreeNode::R")]]
                  () const;

};

class [[asap::param("P"), asap::region("L, R, V, Links")]] TreeNode {
  friend class GrowTreeLeft;
  friend class GrowTreeRight;

  TreeNode *left [[asap::arg("P:L, P:L")]];
  TreeNode *right [[asap::arg("P:R, P:R")]];
  int value [[asap::arg("P:V")]];

public:
  TreeNode(int v = 0) : left(__null), right(__null), value(v) {}

  void addLeftChild [[asap::writes("P:L")]]
                    (TreeNode *N [[asap::arg("P:L")]]) {
    left = N;
  }

  void addRightChild [[asap::writes("P:R")]]
                     (TreeNode *N [[asap::arg("P:R")]]) {
    right = N;
  }

  void growTree [[asap::reads("P:*:V, ReadOnly"), asap::writes("P:*:L, P:*:R")]]
                (int depth) {
    if (depth<=0) return;
# 95 "tree-NI.cpp"
    GrowTreeLeft Left [[asap::arg("P")]] (this, depth);
    GrowTreeRight Right [[asap::arg("P")]] (this, depth);
    tbb::parallel_invoke(Left, Right);

  }

  void printTree [[asap::reads("P:*")]] () {
    printf("%d, ", value);
    if (left)
      left->printTree();
    if (right)
      right->printTree();
  }

};

void GrowTreeLeft::operator() () const {
  if (Node->left==__null) {
    Node->left = new TreeNode(Node->value+1);
    Node->left->growTree(Depth-1);
  }
}

void GrowTreeRight::operator() () const {
  if (Node->right==__null) {
    int newValue = Node->value + (1<<(Depth));
    Node->right = new TreeNode(newValue);
    Node->right->growTree(Depth-1);
  }
}
# 142 "tree-NI.cpp"
int main [[asap::region("MAIN"), asap::reads("ReadOnly"), asap::writes("MAIN:*")]]
         (int argc, char *argv [[asap::arg("Local, Local")]] [])
{
    int nth=-1;
    if (argc > 1) {
        if (argc > 2) {
            printf("ERROR: wrong use of command line arguments. Usage %s <#threads>\n", argv[0]);
            return 1;
        } else {
            nth = atoi( argv[1] );
        }
    }
    int defth = tbb::task_scheduler_init::default_num_threads();
    if (nth<0) nth=defth;
    printf("Default #Threads=%d. Using %d threads\n", defth, nth);
    tbb::task_scheduler_init init(nth);


    int j,k;
    tbb::tick_count t0 = tbb::tick_count::now();

    long result = -1;
    TreeNode *Tree [[asap::arg("MAIN")]] = new TreeNode(0);
    Tree->growTree(30);

    printf("\n");




    tbb::tick_count t1 = tbb::tick_count::now();
    printf("Ticks = %f\n", (t1-t0).seconds());

    return 0;
}
