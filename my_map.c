#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <asm/current.h>

/* find "task_struct" by pid */
static struct task_struct *find_task_by_pid(pid_t pid)
{
  struct pid *st_pid = find_get_pid(pid); /* get "pid struct" by pid */
  if(!st_pid){
    printk("could not find pid %d's taskn", pid);
    return NULL;
  }
  return pid_task(st_pid, PIDTYPE_PID);
}

/* padding by ' ' */
static void pad_by_space(int len){
  int count;
  for(count=0; count<len; count++) printk(KERN_CONT " ");
}

/* print information of vma */
static void print_line(unsigned long start, unsigned long end,
               vm_flags_t flags, unsigned long long pgoff,
               dev_t dev, unsigned long ino,const char *name){
 /* following is the display section */
  int pad_len = 73;
  int length = 0;
  length = printk("%08lx-%08lx %c%c%c%c %08llx %02x:%02x %lu",
         start,
         end,
         flags & VM_READ ? 'r' : '-',
         flags & VM_WRITE ? 'w' : '-',
         flags & VM_EXEC ? 'x' : '-',
         flags & VM_MAYSHARE ? 's' : 'p',
         pgoff,
         MAJOR(dev),
         MINOR(dev),
         ino
         );
  pad_by_space(pad_len-length);
  printk(KERN_CONT "%sn",name);
}

/* show information of vma */
static void my_show_maps_vma(struct vm_area_struct *vma)
{
  struct mm_struct *mm = vma->vm_mm; /* memory descriptor that owns vma */
  struct file * file = vma->vm_file; /* the file mapped to vma */
  vm_flags_t flags = vma->vm_flags;  /* flags of vma */
  unsigned long ino = 0;             /* inode number of "file" variable */
  unsigned long long pgoff = 0;      /* offset in the "file" */
  unsigned long start, end;          /* range of addresses managed by vma */
  dev_t dev = 0;                     /* Device number of the superblock object that is managing the file */
  const char *name = NULL;                 /* special name of memory region (e.g. [vsyscall]) */
  const char *filename = NULL;
  char tmp[256];

  /* if vma is an area mapped from "file" */
  /* (if "file" variable not NULL) */
  if(file) {
    struct inode *inode = file_inode(vma->vm_file); /* inode object */
    dev = inode->i_sb->s_dev;
    ino = inode->i_ino;
    pgoff = ((loff_t)vma->vm_pgoff) << PAGE_SHIFT;  /* vm_pgoff is expressed as a multiple of page size */
  }

  start = vma->vm_start;
  end = vma->vm_end;

  /* print filename (absolute path)*/
  if(file) {
    filename = d_path(&file->f_path, tmp, 100);  /* write absolute path to buffer */
    goto done;
  }

  /* set area name of vma to "name" variable (if exist) */
  if(vma->vm_ops && vma->vm_ops->name) {   /* if vma have special name, name function is NULL */
    name = vma->vm_ops->name(vma);         /* call name function defined in vm_operations_struct (available from kernel-v4 (maybe))*/
    if(name) goto done;
  }

  /* if vma has not special name */
  if(!name) {
    if(!mm) {
      /* 
       * kernel has not memory descriptor. 
       * kernel has a fixed linear memory space, 
       * so, kernel doesn't need memory descriptor.
       */
      name = "[vdso]";
      goto done;
    }

    if(vma->vm_start <= mm->brk && vma->vm_end >= mm->start_brk){
      name = "[heap]";
      goto done;
    }

    if(vma->vm_start <= vma->vm_mm->start_stack && vma->vm_end >= vma->vm_mm->start_stack){
      name = "[stack]";
      goto done;
    }
  }
 done:
  /* print */
  if(filename) {
    print_line(start, end, flags, pgoff, dev, ino, filename);
  }else if(name) {
    print_line(start, end, flags, pgoff, dev, ino, name);
  }else{
    print_line(start, end, flags, pgoff, dev, ino, "");
  }

}

/* show memory maps */
static int my_show_maps(struct task_struct *process)
{
  struct mm_struct *mm;
  struct vm_area_struct *vma;

  mm = process->mm;        /* get process's memory descriptor */
  if(mm){
    vma = mm->mmap;          /* get head of the list of memory region */
    while(vma){              /* while vma is not NULL */
      my_show_maps_vma(vma);
      vma = vma->vm_next;
    }
    vma = get_gate_vma(mm);
    if(vma) {
      my_show_maps_vma(vma);
      return mm->map_count + 1;
    }
    return mm->map_count;
  }
  return 0;
}

/* syscall 442 */
SYSCALL_DEFINE1(my_maps, int, pid_from_user){
  pid_t pid = pid_from_user;  /* pit_t is int(typedef) */
  struct task_struct *process = find_task_by_pid(pid);
  int ret;
  if(process) ret=my_show_maps(process);
  return ret;
}
