// usage: plash run CONTAINER [ CMD1 [ CMD2 ... ] ]
//
// Run a container. If no command is specified, the containers default root
// shell is executed.
//
// The following host file systems are mapped to the container:
// - /tmp
// - /home
// - /root
// - /etc/resolv.conf
// - /sys
// - /dev
// - /proc
//
// If you want more control about how the container is run, use `plash runopts`

#include <stddef.h>
#include <plash.h>

int main(int argc, char *argv[]) {

  if (argc < 2)
    pl_usage();

  char *container_id = argv[1];
  char *changesdir = pl_call("mkdtemp");

  pl_exec_add("plash");
  pl_exec_add("runopts");
  pl_exec_add("-c");
  pl_exec_add(container_id);
  pl_exec_add("-d");
  pl_exec_add(changesdir);
  
  // environment variables
  pl_exec_add("-e");
  pl_exec_add("TERM");
  pl_exec_add("-e");
  pl_exec_add("DISPLAY");
  pl_exec_add("-e");
  pl_exec_add("HOME");
  pl_exec_add("-e");
  pl_exec_add("PLASH_DATA");
  pl_exec_add("-E");
  pl_exec_add("PLASH_EXPORT");
  
  // mountpoints
  pl_exec_add("-m");
  pl_exec_add("/tmp");
  pl_exec_add("-m");
  pl_exec_add("/home");
  pl_exec_add("-m");
  pl_exec_add("/root");
  pl_exec_add("-m");
  pl_exec_add("/etc/resolv.conf");
  pl_exec_add("-m");
  pl_exec_add("/sys");
  pl_exec_add("-m");
  pl_exec_add("/dev");
  pl_exec_add("-m");
  pl_exec_add("/proc");

  // command-starts-now  marker
  pl_exec_add("--");

  if (argv[2]){ // if any command specified
    pl_exec_add("sh");
    pl_exec_add("-lc");
    pl_exec_add("exec env \"$@\"");
    pl_exec_add("--");
  }

  argv++; // chop argv[0]
  argv++; // chop the container id
  while (argv[0])
    pl_exec_add((argv++)[0]);

  pl_exec_add(NULL);
  
}

