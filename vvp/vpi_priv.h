#ifndef __vpi_priv_H
#define __vpi_priv_H
/*
 * Copyright (c) 2001 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#if !defined(WINNT)
#ident "$Id: vpi_priv.h,v 1.36 2002/05/19 05:18:16 steve Exp $"
#endif

# include  "vpi_user.h"
# include  "pointers.h"
# include  "memory.h"

/*
 * This header file contains the internal definitions that the vvp
 * program uses to implement the public interface in the vpi_user.h
 * header file elsewhere.
 */


/*
 * The vpi_mode_flag contains the major mode for VPI use. This is used
 * to generate error messages whe vpi functions are called
 * incorrectly.
 */
enum vpi_mode_t {
      VPI_MODE_NONE =0,
	/* The compiler is calling a register function. */
      VPI_MODE_REGISTER,
	/* The compiler is calling a compiletf function. */
      VPI_MODE_COMPILETF,
	/* The compiler is calling a calltf function. */
      VPI_MODE_CALLTF,
	/* We are in the midst of a RWSync callback. */
      VPI_MODE_RWSYNC,
	/* We are in a ROSync callback. */
      VPI_MODE_ROSYNC
};
extern vpi_mode_t vpi_mode_flag;

/*
 * This structure is the very base of a vpiHandle. Every handle
 * structure starts with this structure, so that the library can
 * internally pass the derived types as pointers to one of these.
 */
struct __vpiHandle {
      const struct __vpirt *vpi_type;
};

/*
 * Objects with this structure are used to represent a type of
 * vpiHandle. A specific object becomes of this type by holding a
 * pointer to an instance of this structure.
 */
struct __vpirt {
      int type_code;

	/* These methods extract information from the handle. */
      int   (*vpi_get_)(int, vpiHandle);
      char* (*vpi_get_str_)(int, vpiHandle);
      void  (*vpi_get_value_)(vpiHandle, p_vpi_value);
      vpiHandle (*vpi_put_value_)(vpiHandle, p_vpi_value, p_vpi_time, int);

	/* These methods follow references. */
      vpiHandle (*handle_)(int, vpiHandle);
      vpiHandle (*iterate_)(int, vpiHandle);
      vpiHandle (*index_)(vpiHandle, int);

	/* This implements the vpi_free_object method. */
      int (*vpi_free_object_)(vpiHandle);
};

/*
 * The vpiHandle for an iterator has this structure. The definition of
 * the methods lives in vpi_iter.c
 *
 * The args and nargs members point to the array of vpiHandle objects
 * that are to be iterated over. The next member is the index of the
 * next item to be returned by a vpi_scan.
 *
 * The free_args_flag member is true if when this iterator object is
 * released it must also free the args array.
 */
struct __vpiIterator {
      struct __vpiHandle base;
      vpiHandle *args;
      unsigned  nargs;
      unsigned  next;
      bool free_args_flag;
};

extern vpiHandle vpip_make_iterator(unsigned nargs, vpiHandle*args,
				    bool free_args_flag);

/*
 * This represents callback handles. There are some privat types that
 * are defined and used in vpi_callback.cc.
 */
struct __vpiCallback {
      struct __vpiHandle base;

	// user supplied callback data
      struct t_cb_data cb_data;
      struct t_vpi_time cb_time;

	// scheduled event
      struct sync_cb* cb_sync;

	// Used for listing callbacks.
      struct __vpiCallback*next;
};

extern struct __vpiCallback* new_vpi_callback();
extern void callback_execute(struct __vpiCallback*cur);

/*
 * Scopes are created by .scope statements in the source.
 */
struct __vpiScope {
      struct __vpiHandle base;
      struct __vpiScope *scope;
	/* The scope has a name. */
      char*name;
      /* Keep an array of internal scope items. */
      struct __vpiHandle**intern;
      unsigned nintern;
	/* Keep a list of threads in the scope. */
      vthread_t threads;
};

extern struct __vpiScope* vpip_peek_current_scope(void);
extern void vpip_attach_to_current_scope(vpiHandle obj);
extern vpiHandle vpip_make_root_iterator(void);

/*
 * Signals include the variable types (reg, integer, time) and are
 * distinguished by the vpiType code. They also have a parent scope,
 * a declared name and declaration indices.
 */
struct __vpiSignal {
      struct __vpiHandle base;
      struct __vpiScope* scope;
	/* The name of this reg/net object */
      char*name;
	/* The indices that define the width and access offset. */
      int msb, lsb;
	/* Flags */
      unsigned signed_flag  : 1;
	/* The represented value is here. */
      vvp_fvector_t bits;
        /* This is the callback event functor */
      struct callback_functor_s *callback;
};
extern vpiHandle vpip_make_reg(char*name, int msb, int lsb, bool signed_flag,
			       vvp_fvector_t vec);
extern vpiHandle vpip_make_net(char*name, int msb, int lsb, bool signed_flag,
			       vvp_fvector_t vec);

/*
 * These methods support the vpi creation of events. The name string
 * passed in will be saved, so the caller must allocate it (or not
 * free it) after it is handed to this function.
 */
struct __vpiNamedEvent {
      struct __vpiHandle base;
	/* base name of the event object */
      char*name;
	/* Parent scope of this object. */
      struct __vpiScope*scope;
	/* List of callbacks interested in this event. */
      struct __vpiCallback*callbacks;
};

extern vpiHandle vpip_make_named_event(char*name);
extern void vpip_run_named_event_callbacks(vpiHandle ref);

/*
 * Memory is an array of bits that is accessible in N-bit chunks, with
 * N being the width of a word. The memory word handle just points
 * back to the memory and uses an index to identify its position in
 * the memory.
 */

extern vpiHandle vpip_make_memory(vvp_memory_t mem);

/*
 * When a loaded VPI module announces a system task/function, one
 * __vpiUserSystf object is created to hold the definition of that
 * task/function. The distinction between task and function is stored
 * in the vpi_systf_data structure data that was supplied by the
 * external module.
 *
 * When the compiler encounters a %vpi_call statement, it creates a
 * __vpiSysTaskCall to represent that particular call. The call refers
 * to the definition handle so that when the %vpi_call instruction is
 * encountered at run-time, the definition can be located and used.
 *
 * The vpiSysTaskCall handles both functions and tasks, as the two are
 * extremely similar. The different VPI type is reflected in a
 * different vpi_type pointer in the base structure. The only
 * additional part is the vbit/vwid that is used by the put of the
 * system function call to place the values in the vthread bit space.
 */
struct __vpiUserSystf {
      struct __vpiHandle base;
      s_vpi_systf_data info;
};

struct __vpiSysTaskCall {
      struct __vpiHandle base;
      struct __vpiScope* scope;
      struct __vpiUserSystf*defn;
      unsigned nargs;
      vpiHandle*args;

	/* These represent where in the vthread to put the return value. */
      unsigned short vbit, vwid;
};

extern struct __vpiSysTaskCall*vpip_cur_task;

/*
 * These are implemented in vpi_const.cc. These are vpiHandles for
 * constants.
 */
struct __vpiStringConst {
      struct __vpiHandle base;
      const char*value;
};

vpiHandle vpip_make_string_const(char*text);

struct __vpiBinaryConst {
      struct __vpiHandle base;
      unsigned nbits;
      unsigned char*bits;
      unsigned signed_flag :1;
};

vpiHandle vpip_make_binary_const(unsigned wid, char*bits);

struct __vpiDecConst {
      struct __vpiHandle base;
      int value;
};

vpiHandle vpip_make_dec_const(int value);
vpiHandle vpip_make_dec_const(struct __vpiDecConst*obj, int value);

/*
 *  This one looks like a constant, but really is a vector in the current 
 *  thread. 
 */

vpiHandle vpip_make_vthr_vector(unsigned base, unsigned wid, bool signed_flag);

/*
 * This function is called before any compilation to load VPI
 * modules. This gives the modules a chance to announce their
 * contained functions before compilation commences. It is called only
 * once per module.
 */
extern void vpip_load_module(const char*name);

# define VPIP_MODULE_PATH_MAX 64
extern const char* vpip_module_path[64];
extern unsigned vpip_module_path_cnt;

/*
 * The vpip_build_vpi_call function creates a __vpiSysTaskCall object
 * and returns the handle. The compiler uses this function when it
 * encounters a %vpi_call statement.
 *
 * The %vpi_call instruction has as its only parameter the handle that
 * is returned by the vpip_build_vpi_call. This includes all the
 * information needed by vpip_execute_vpi_call to actually execute the
 * call. However, the vpiSysTaskCall that is the returned handle,
 * holds a parameter argument list that is passed in here.
 *
 * The vbit and vwid fields are used if this turns out to be a system
 * function. In that case, the vbit and vwid are used to address the
 * vector is thread bit space where the result is supposed to go.
 *
 * Note that the argv array is saved in the handle, and should should
 * not be released by the caller.
 */
extern vpiHandle vpip_build_vpi_call(const char*name,
				     unsigned vbit, unsigned vwid,
				     unsigned argc,
				     vpiHandle*argv);

extern vthread_t vpip_current_vthread;

extern void vpip_execute_vpi_call(vthread_t thr, vpiHandle obj);


/*
 * These are functions used by the compiler to prepare for compilation
 * and to finish compilation in preparation for execution.
 */

vpiHandle vpip_sim_time(void);

extern int vpip_get_time_precision(void);
extern void vpip_set_time_precision(int pres);

extern void vpip_time_to_timestruct(struct t_vpi_time*ts, vvp_time64_t ti);
extern vvp_time64_t vpip_timestruct_to_time(const struct t_vpi_time*ts);


/*
**  Functions defined in vpi_scope.cc, to keep track of functor scope.
*/

extern vpiHandle ipoint_get_scope(vvp_ipoint_t ipt);
extern void functor_set_scope(vpiHandle scope);

/*
 * This function is used to make decimal string versions of various
 * vectors. The input format is an array of bit values (0, 1, 2, 3)
 * lsb first, and the result is written into buf, without overflowing
 * nbuf.
 */
extern unsigned vpip_bits_to_dec_str(const unsigned char *bits,
				     unsigned int nbits,
				     char *buf, unsigned int nbuf,
				     int signed_flag);

extern void vpip_dec_str_to_bits(unsigned char*bits, unsigned nbits,
				 const char*buf, bool signed_flag);

extern void vpip_bin_str_to_bits(unsigned char*bits, unsigned nbits,
				 const char*buf, bool signed_flag);

extern void vpip_hex_str_to_bits(unsigned char*bits, unsigned nbits,
				 const char*buf, bool signed_flag);

extern void vpip_bits_to_oct_str(const unsigned char*bits, unsigned nbits,
				 char*buf, unsigned nbuf, bool signed_flag);

extern void vpip_oct_str_to_bits(unsigned char*bits, unsigned nbits,
				 const char*buf, bool signed_flag);

/*
 * $Log: vpi_priv.h,v $
 * Revision 1.36  2002/05/19 05:18:16  steve
 *  Add callbacks for vpiNamedEvent objects.
 *
 * Revision 1.35  2002/05/18 02:34:11  steve
 *  Add vpi support for named events.
 *
 *  Add vpi_mode_flag to track the mode of the
 *  vpi engine. This is for error checking.
 *
 * Revision 1.34  2002/05/17 04:12:19  steve
 *  Rewire vpiMemory and vpiMemoryWord handles to
 *  support proper iteration of words, and the
 *  vpiIndex value.
 *
 * Revision 1.33  2002/05/11 04:39:35  steve
 *  Set and get memory words by string value.
 *
 * Revision 1.32  2002/05/03 15:44:11  steve
 *  Add vpiModule iterator to vpiScope objects.
 *
 * Revision 1.31  2002/04/20 04:33:23  steve
 *  Support specified times in cbReadOnlySync, and
 *  add support for cbReadWriteSync.
 *  Keep simulation time in a 64bit number.
 *
 * Revision 1.30  2002/04/14 03:53:20  steve
 *  Allow signed constant vectors for call_vpi parameters.
 *
 * Revision 1.29  2002/04/14 02:56:19  steve
 *  Support signed expressions through to VPI.
 *
 * Revision 1.28  2002/02/03 01:01:51  steve
 *  Use Larrys bits-to-decimal-string code.
 *
 * Revision 1.27  2002/01/31 04:28:17  steve
 *  Full support for $readmem ranges (Tom Verbeure)
 *
 * Revision 1.26  2002/01/06 17:50:50  steve
 *  Support scope for functors. (Stephan Boettcher)
 *
 * Revision 1.25  2002/01/06 00:48:39  steve
 *  VPI access to root module scopes.
 *
 * Revision 1.24  2001/10/31 04:27:47  steve
 *  Rewrite the functor type to have fewer functor modes,
 *  and use objects to manage the different types.
 *  (Stephan Boettcher)
 *
 * Revision 1.23  2001/10/15 01:49:50  steve
 *  Support getting scope of scope, and scope of signals.
 *
 * Revision 1.22  2001/08/08 01:05:06  steve
 *  Initial implementation of vvp_fvectors.
 *  (Stephan Boettcher)
 *
 * Revision 1.21  2001/07/30 02:44:05  steve
 *  Cleanup defines and types for mingw compile.
 *
 * Revision 1.20  2001/07/26 03:13:51  steve
 *  Make the -M flag add module search paths.
 *
 * Revision 1.19  2001/07/11 02:27:21  steve
 *  Add support for REadOnlySync and monitors.
 *
 * Revision 1.18  2001/06/30 23:03:17  steve
 *  support fast programming by only writing the bits
 *  that are listed in the input file.
 *
 * Revision 1.17  2001/06/21 22:54:12  steve
 *  Support cbValueChange callbacks.
 *
 * Revision 1.16  2001/05/20 00:46:12  steve
 *  Add support for system function calls.
 *
 * Revision 1.15  2001/05/10 00:26:53  steve
 *  VVP support for memories in expressions,
 *  including general support for thread bit
 *  vectors as system task parameters.
 *  (Stephan Boettcher)
 *
 * Revision 1.14  2001/05/08 23:59:33  steve
 *  Add ivl and vvp.tgt support for memories in
 *  expressions and l-values. (Stephan Boettcher)
 */
#endif
