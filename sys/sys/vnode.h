/*	$OpenBSD: vnode.h,v 1.22 1999/12/05 07:19:28 art Exp $	*/
/*	$NetBSD: vnode.h,v 1.38 1996/02/29 20:59:05 cgd Exp $	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vnode.h	8.11 (Berkeley) 11/21/94
 */

#include <sys/queue.h>
#include <sys/lock.h>
#ifdef UVM			/* XXX: clean up includes later */
#include <vm/pglist.h>		/* XXX */
#include <vm/vm_param.h>	/* XXX */
#include <sys/lock.h>		/* XXX */
#include <uvm/uvm_object.h>	/* XXX */
#include <uvm/uvm_vnode.h>	/* XXX */
#endif /* UVM */

/*
 * The vnode is the focus of all file activity in UNIX.  There is a
 * unique vnode allocated for each active file, each current directory,
 * each mounted-on file, text file, and the root.
 */

/*
 * Vnode types.  VNON means no type.
 */
enum vtype	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD };

/*
 * Vnode tag types.
 * These are for the benefit of external programs only (e.g., pstat)
 * and should NEVER be inspected by the kernel.
 *
 * Note that v_tag is actually used to tell MFS from FFS, and EXT2FS from
 * the rest, so don't believe the above comment!
 */
enum vtagtype	{
	VT_NON, VT_UFS, VT_NFS, VT_MFS, VT_MSDOSFS, VT_LFS, VT_LOFS, VT_FDESC,
	VT_PORTAL, VT_NULL, VT_UMAP, VT_KERNFS, VT_PROCFS, VT_AFS, VT_ISOFS,
	VT_UNION, VT_ADOSFS, VT_EXT2FS, VT_NCPFS, VT_VFS, VT_XFS
};

/*
 * Each underlying filesystem allocates its own private area and hangs
 * it from v_data.  If non-null, this area is freed in getnewvnode().
 */
LIST_HEAD(buflists, buf);

/*
 * Reading or writing any of these items requires holding the appropriate lock.
 * v_freelist is locked by the global vnode_free_list simple lock.
 * v_mntvnodes is locked by the global mntvnodes simple lock.
 * v_flag, v_usecount, v_holdcount and v_writecount are
 *    locked by the v_interlock simple lock.
 */

struct vnode {
#ifdef UVM
	struct uvm_vnode v_uvm;			/* uvm data */
#endif
	u_long	v_flag;				/* vnode flags (see below) */
	short	v_usecount;			/* reference count of users */
	short	v_writecount;			/* reference count of writers */
	long	v_holdcnt;			/* page & buffer references */
	daddr_t	v_lastr;			/* last read (read-ahead) */
	u_long	v_id;				/* capability identifier */
	struct	mount *v_mount;			/* ptr to vfs we are in */
	int 	(**v_op) __P((void *));		/* vnode operations vector */
	TAILQ_ENTRY(vnode) v_freelist;		/* vnode freelist */
	LIST_ENTRY(vnode) v_mntvnodes;		/* vnodes for mount point */
	struct	buflists v_cleanblkhd;		/* clean blocklist head */
	struct	buflists v_dirtyblkhd;		/* dirty blocklist head */
	long	v_numoutput;			/* num of writes in progress */
	LIST_ENTRY(vnode) v_synclist;           /* vnode with dirty buffers */
	enum	vtype v_type;			/* vnode type */
	union {
		struct mount	*vu_mountedhere;/* ptr to mounted vfs (VDIR) */
		struct socket	*vu_socket;	/* unix ipc (VSOCK) */
		caddr_t		vu_vmdata;	/* private data for vm (VREG) */
		struct specinfo	*vu_specinfo;	/* device (VCHR, VBLK) */
		struct fifoinfo	*vu_fifoinfo;	/* fifo (VFIFO) */
	} v_un;
	struct	nqlease *v_lease;		/* Soft reference to lease */
	daddr_t	v_lastw;			/* last write (write cluster) */
	daddr_t	v_cstart;			/* start block of cluster */
	daddr_t	v_lasta;			/* last allocation */
	int	v_clen;				/* length of current cluster */
	int	v_ralen;			/* Read-ahead length */
	daddr_t	v_maxra;			/* last readahead block */
	struct  simplelock v_interlock;		/* lock on usecount and flag */
	struct  lock *v_vnlock;			/* used for non-locking fs's */
#ifdef UVM
#else
	long	v_spare[3];			/* round to 128 bytes */
#endif
	enum	vtagtype v_tag;			/* type of underlying data */
	void 	*v_data;			/* private data for fs */
};
#define	v_mountedhere	v_un.vu_mountedhere
#define	v_socket	v_un.vu_socket
#define	v_vmdata	v_un.vu_vmdata
#define	v_specinfo	v_un.vu_specinfo
#define	v_fifoinfo	v_un.vu_fifoinfo

/*
 * Vnode flags.
 */
#define	VROOT		0x0001	/* root of its file system */
#define	VTEXT		0x0002	/* vnode is a pure text prototype */
#define	VSYSTEM		0x0004	/* vnode being used by kernel */
#define	VISTTY		0x0008	/* vnode represents a tty */
#define	VXLOCK		0x0100	/* vnode is locked to change underlying type */
#define	VXWANT		0x0200	/* process is waiting for vnode */
#define	VBWAIT		0x0400	/* waiting for output to complete */
#define	VALIASED	0x0800	/* vnode has an alias */
#define	VDIROP		0x1000	/* LFS: vnode is involved in a directory op */
#define VONFREELIST     0x2000  /* Vnode is on a free list */
#define VLOCKSWORK      0x4000  /* FS supports locking discipline */
#define VONSYNCLIST	0x8000	/* Vnode is on syncer worklist */

/*
 * Vnode attributes.  A field value of VNOVAL represents a field whose value
 * is unavailable (getattr) or which is not to be changed (setattr).
 */
struct vattr {
	enum vtype	va_type;	/* vnode type (for create) */
	u_short		va_mode;	/* files access mode and type */
	short		va_nlink;	/* number of references to file */
	uid_t		va_uid;		/* owner user id */
	gid_t		va_gid;		/* owner group id */
	long		va_fsid;	/* file system id (dev for now) */
	long		va_fileid;	/* file id */
	u_quad_t	va_size;	/* file size in bytes */
	long		va_blocksize;	/* blocksize preferred for i/o */
	struct timespec	va_atime;	/* time of last access */
	struct timespec	va_mtime;	/* time of last modification */
	struct timespec	va_ctime;	/* time file changed */
	u_long		va_gen;		/* generation number of file */
	u_long		va_flags;	/* flags defined for file */
	dev_t		va_rdev;	/* device the special file represents */
	u_quad_t	va_bytes;	/* bytes of disk space held by file */
	u_quad_t	va_filerev;	/* file modification number */
	u_int		va_vaflags;	/* operations flags, see below */
	long		va_spare;	/* remain quad aligned */
};

/*
 * Flags for va_cflags.
 */
#define	VA_UTIMES_NULL	0x01		/* utimes argument was NULL */
#define VA_EXCLUSIVE    0x02            /* exclusive create request */
/*
 * Flags for ioflag.
 */
#define	IO_UNIT		0x01		/* do I/O as atomic unit */
#define	IO_APPEND	0x02		/* append write to end */
#define	IO_SYNC		0x04		/* do I/O synchronously */
#define	IO_NODELOCKED	0x08		/* underlying node already locked */
#define	IO_NDELAY	0x10		/* FNDELAY flag set in file table */

/*
 *  Modes.  Some values same as Ixxx entries from inode.h for now.
 */
#define	VSUID	04000		/* set user id on execution */
#define	VSGID	02000		/* set group id on execution */
#define	VSVTX	01000		/* save swapped text even after use */
#define	VREAD	00400		/* read, write, execute permissions */
#define	VWRITE	00200
#define	VEXEC	00100

/*
 * Token indicating no attribute value yet assigned.
 */
#define	VNOVAL	(-1)

#ifdef _KERNEL
/*
 * Convert between vnode types and inode formats (since POSIX.1
 * defines mode word of stat structure in terms of inode formats).
 */
extern enum vtype	iftovt_tab[];
extern int		vttoif_tab[];
#define IFTOVT(mode)	(iftovt_tab[((mode) & S_IFMT) >> 12])
#define VTTOIF(indx)	(vttoif_tab[(int)(indx)])
#define MAKEIMODE(indx, mode)	(int)(VTTOIF(indx) | (mode))

/*
 * Flags to various vnode functions.
 */
#define	SKIPSYSTEM	0x0001		/* vflush: skip vnodes marked VSYSTEM */
#define	FORCECLOSE	0x0002		/* vflush: force file closeure */
#define	WRITECLOSE	0x0004		/* vflush: only close writeable files */
#define	DOCLOSE		0x0008		/* vclean: close active files */
#define	V_SAVE		0x0001		/* vinvalbuf: sync file first */
#define	V_SAVEMETA	0x0002		/* vinvalbuf: leave indirect blocks */

#define REVOKEALL       0x0001          /* vop_reovke: revoke all aliases */


TAILQ_HEAD(freelst, vnode);
extern struct freelst vnode_hold_list;	/* free vnodes referencing buffers */
extern struct freelst vnode_free_list;	/* vnode free list */
extern struct simplelock vnode_free_list_slock;

#ifdef DIAGNOSTIC
#define	HOLDRELE(vp)	holdrele(vp)
#define	VATTR_NULL(vap)	vattr_null(vap)
#define	VHOLD(vp)	vhold(vp)
#define	VREF(vp)	vref(vp)

void	holdrele __P((struct vnode *));
void	vhold __P((struct vnode *));
void	vref __P((struct vnode *));
#else
#define	HOLDRELE(vp)	holdrele(vp); 	/* decrease buf or page ref */
#define	VATTR_NULL(vap)	(*(vap) = va_null)	/* initialize a vattr */

static __inline void holdrele __P((struct vnode *));
static __inline void vhold __P((struct vnode *));
static __inline void vref __P((struct vnode *));

static __inline void
holdrele(vp)
	struct vnode *vp;
{
	simple_lock(&vp->v_interlock);
	vp->v_holdcnt--;
 	if ((vp->v_flag & VONFREELIST) &&
 	    vp->v_holdcnt == 0 && vp->v_usecount == 0) {
 		simple_lock(&vnode_free_list_slock);
 		TAILQ_REMOVE(&vnode_hold_list, vp, v_freelist);
 		TAILQ_INSERT_TAIL(&vnode_free_list, vp, v_freelist);
 		simple_unlock(&vnode_free_list_slock);
 	}
	simple_unlock(&vp->v_interlock);
}

#define	VHOLD(vp)	vhold(vp)		/* increase buf or page ref */
static __inline void
vhold(vp)
	struct vnode *vp;
{
	simple_lock(&vp->v_interlock);
 	if ((vp->v_flag & VONFREELIST) &&
 	    vp->v_holdcnt == 0 && vp->v_usecount == 0) {
 		simple_lock(&vnode_free_list_slock);
 		TAILQ_REMOVE(&vnode_free_list, vp, v_freelist);
 		TAILQ_INSERT_TAIL(&vnode_hold_list, vp, v_freelist);
 		simple_unlock(&vnode_free_list_slock);
 	}
	vp->v_holdcnt++;
	simple_unlock(&vp->v_interlock);
}

#define	VREF(vp)	vref(vp)		/* increase reference */
static __inline void
vref(vp)
	struct vnode *vp;
{
	simple_lock(&vp->v_interlock);
	vp->v_usecount++;
	simple_unlock(&vp->v_interlock);
}
#endif /* DIAGNOSTIC */

#define	NULLVP	((struct vnode *)NULL)

/*
 * Global vnode data.
 */
extern	struct vnode *rootvnode;	/* root (i.e. "/") vnode */
extern	int desiredvnodes;		/* number of vnodes desired */
extern	time_t syncdelay;		/* time to delay syncing vnodes */
extern	int rushjob;			/* # of slots syncer should run ASAP */
extern	struct vattr va_null;		/* predefined null vattr structure */

/*
 * Macro/function to check for client cache inconsistency w.r.t. leasing.
 */
#define	LEASE_READ	0x1		/* Check lease for readers */
#define	LEASE_WRITE	0x2		/* Check lease for modifiers */

#endif /* _KERNEL */


/*
 * Mods for exensibility.
 */

/*
 * Flags for vdesc_flags:
 */
#define VDESC_MAX_VPS		16
/* Low order 16 flag bits are reserved for willrele flags for vp arguments. */
#define VDESC_VP0_WILLRELE	0x0001
#define VDESC_VP1_WILLRELE	0x0002
#define VDESC_VP2_WILLRELE	0x0004
#define VDESC_VP3_WILLRELE	0x0008
#define VDESC_NOMAP_VPP		0x0100
#define VDESC_VPP_WILLRELE	0x0200

/*
 * VDESC_NO_OFFSET is used to identify the end of the offset list
 * and in places where no such field exists.
 */
#define VDESC_NO_OFFSET -1

/*
 * This structure describes the vnode operation taking place.
 */
struct vnodeop_desc {
	int	vdesc_offset;		/* offset in vector--first for speed */
	char    *vdesc_name;		/* a readable name for debugging */
	int	vdesc_flags;		/* VDESC_* flags */

	/*
	 * These ops are used by bypass routines to map and locate arguments.
	 * Creds and procs are not needed in bypass routines, but sometimes
	 * they are useful to (for example) transport layers.
	 * Nameidata is useful because it has a cred in it.
	 */
	int	*vdesc_vp_offsets;	/* list ended by VDESC_NO_OFFSET */
	int	vdesc_vpp_offset;	/* return vpp location */
	int	vdesc_cred_offset;	/* cred location, if any */
	int	vdesc_proc_offset;	/* proc location, if any */
	int	vdesc_componentname_offset; /* if any */
	/*
	 * Finally, we've got a list of private data (about each operation)
	 * for each transport layer.  (Support to manage this list is not
	 * yet part of BSD.)
	 */
	caddr_t	*vdesc_transports;
};

#ifdef _KERNEL
/*
 * A list of all the operation descs.
 */
extern struct vnodeop_desc *vnodeop_descs[];


/*
 * Interlock for scanning list of vnodes attached to a mountpoint
 */
struct simplelock mntvnode_slock;

/*
 * This macro is very helpful in defining those offsets in the vdesc struct.
 *
 * This is stolen from X11R4.  I ingored all the fancy stuff for
 * Crays, so if you decide to port this to such a serious machine,
 * you might want to consult Intrisics.h's XtOffset{,Of,To}.
 */
#define VOPARG_OFFSET(p_type,field) \
        ((int) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))
#define VOPARG_OFFSETOF(s_type,field) \
	VOPARG_OFFSET(s_type*,field)
#define VOPARG_OFFSETTO(S_TYPE,S_OFFSET,STRUCT_P) \
	((S_TYPE)(((char*)(STRUCT_P))+(S_OFFSET)))


/*
 * This structure is used to configure the new vnodeops vector.
 */
struct vnodeopv_entry_desc {
	struct vnodeop_desc *opve_op;   /* which operation this is */
	int (*opve_impl) __P((void *));	/* code implementing this operation */
};
struct vnodeopv_desc {
			/* ptr to the ptr to the vector where op should go */
	int (***opv_desc_vector_p) __P((void *));
	struct vnodeopv_entry_desc *opv_desc_ops;   /* null terminated list */
};

/*
 * A default routine which just returns an error.
 */
int vn_default_error __P((void *));

/*
 * A generic structure.
 * This can be used by bypass routines to identify generic arguments.
 */
struct vop_generic_args {
	struct vnodeop_desc *a_desc;
	/* other random data follows, presumably */
};

/*
 * VOCALL calls an op given an ops vector.  We break it out because BSD's
 * vclean changes the ops vector and then wants to call ops with the old
 * vector.
 */
#define VOCALL(OPSV,OFF,AP) (( *((OPSV)[(OFF)])) (AP))

/*
 * This call works for vnodes in the kernel.
 */
#define VCALL(VP,OFF,AP) VOCALL((VP)->v_op,(OFF),(AP))
#define VDESC(OP) (& __CONCAT(OP,_desc))
#define VOFFSET(OP) (VDESC(OP)->vdesc_offset)

/*
 * Finally, include the default set of vnode operations.
 */
#include <sys/vnode_if.h>

/*
 * Public vnode manipulation functions.
 */
struct file;
struct filedesc;
struct mount;
struct nameidata;
struct proc;
struct stat;
struct ucred;
struct uio;
struct vattr;
struct vnode;

int 	bdevvp __P((dev_t dev, struct vnode **vpp));
int 	cdevvp __P((dev_t dev, struct vnode **vpp));
int 	getnewvnode __P((enum vtagtype tag, struct mount *mp,
			 int (**vops) __P((void *)), struct vnode **vpp));
int	getvnode __P((struct filedesc *fdp, int fd, struct file **fpp));
void	getnewfsid __P((struct mount *, int));
void 	vattr_null __P((struct vattr *vap));
int 	vcount __P((struct vnode *vp));
void	vclean __P((struct vnode *, int, struct proc *));
int	vfinddev __P((dev_t, enum vtype, struct vnode **));
void	vflushbuf __P((struct vnode *vp, int sync));
int	vflush __P((struct mount *mp, struct vnode *vp, int flags));
void	vntblinit __P((void));
void    vn_initialize_syncerd __P((void));
void	vwakeup __P((struct buf *));
void	vdevgone __P((int, int, int, enum vtype));
int 	vget __P((struct vnode *vp, int lockflag, struct proc *p));
void 	vgone __P((struct vnode *vp));
void    vgonel __P((struct vnode *, struct proc *));
void 	vgoneall __P((struct vnode *vp));
int	vinvalbuf __P((struct vnode *vp, int save, struct ucred *cred,
	    struct proc *p, int slpflag, int slptimeo));
void	vprint __P((char *label, struct vnode *vp));
int	vop_generic_bwrite __P((void *ap));
void	vn_update __P((void));
int 	vn_close __P((struct vnode *vp,
	    int flags, struct ucred *cred, struct proc *p));
int 	vn_closefile __P((struct file *fp, struct proc *p));
int	vn_ioctl __P((struct file *fp, u_long com, caddr_t data,
	    struct proc *p));
int 	vn_open __P((struct nameidata *ndp, int fmode, int cmode));
int	vrecycle __P((struct vnode *vp, struct simplelock *inter_lkp,
	    struct proc *p));
int 	vn_rdwr __P((enum uio_rw rw, struct vnode *vp, caddr_t base,
	    int len, off_t offset, enum uio_seg segflg, int ioflg,
	    struct ucred *cred, size_t *aresid, struct proc *p));
int	vn_lock __P((struct vnode *vp, int flags, struct proc *p));

int     vop_generic_abortop __P((void *));
int	vop_generic_islocked __P((void *));
int	vop_generic_lock __P((void *));
int	vop_generic_unlock __P((void *));
int	vop_generic_revoke __P((void *));

int	vn_read __P((struct file *fp, struct uio *uio, struct ucred *cred));
int	vn_select __P((struct file *fp, int which, struct proc *p));
int	vn_stat __P((struct vnode *vp, struct stat *sb, struct proc *p));
int	vn_write __P((struct file *fp, struct uio *uio, struct ucred *cred));
int	vn_writechk __P((struct vnode *vp));
void	vn_syncer_add_to_worklist __P((struct vnode *vp, int delay));
void    sched_sync __P((struct proc *));

struct vnode *
	checkalias __P((struct vnode *vp, dev_t nvp_rdev, struct mount *mp));
void 	vput __P((struct vnode *vp));
void 	vrele __P((struct vnode *vp));
int	vaccess __P((mode_t file_mode, uid_t uid, gid_t gid,
		     mode_t acc_mode, struct ucred *cred));
#endif /* _KERNEL */
