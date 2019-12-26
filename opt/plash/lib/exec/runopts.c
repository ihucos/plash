// usage: plash runopts -c CONTAINER [-d CHANGESDIR] [ -m MOUNT ] [CMD1 [ CMD2 ... ]]
//
// Run a container specifying lower level options.  Usually you'll want to use
// `plash run` instead. If no command is specified the containers default root
// shell will be executed as login shell.
//
// Supported options:
//
// -c CONTAINER
//        the container id to run
//
// -d CHANGESIDR
//        if specified changes to the root file system will be written there
//
// -m MOUNT
//        mount a path to the same location inside the container, can be
//        specified multiple times

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/mount.h>
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <errno.h>

#include <plash.h>

#define OPTSTRING "c:d:m:"
  
int main(int argc, char *argv[]) { 
  char *changesdir = NULL,
       *container = NULL,
       *plash_data,
       *origpwd,
       *default_shell;
  int opt; 
  struct passwd *pwd;

  plash_data = getenv("PLASH_DATA");
  assert(plash_data);

  // don't let getopt print error messages
  opterr = 0;
    
  //
  // save and validate some user options
  //
  while((opt = getopt(argc, argv, OPTSTRING)) != -1) {  
      switch(opt) {  
          case 'c':
              container = optarg;
              break;
          case 'd':
              changesdir = optarg;
              break;
          case 'm':
              if (!(optarg[0] == '/'))
                  pl_fatal("mount path must be absolute");
              break;
          case ':':
              pl_usage();
              break;
          case '?':
              pl_usage();
      }  
  } 
  if (container == NULL) pl_usage();

  //
  // get "userspace root"
  //
  pl_unshare_user();
  pl_unshare_mount();

  origpwd = get_current_dir_name();

  //
  // prepare an empty mountpoint
  //
  if (chdir(plash_data) == -1) pl_fatal("chdir");
  if (mount("tmpfs", "mnt", "tmpfs", MS_MGC_VAL, NULL) == -1) pl_fatal("mount");

  //
  // mount root filesystem at the empty mountpoint
  //
  pl_check_call((char*[]){"plash", "mount", container, "mnt", changesdir, NULL});

  //
  // mount requested mounts
  //
  if (chdir("mnt") == -1) pl_fatal("chdir");
  optind = 1; // reset the getopt
  while((opt = getopt(argc, argv, OPTSTRING)) != -1) { 
      switch(opt){
        case 'm':
          pl_bind_mount(optarg, optarg + 1);
          break;
      }  
  }  

  //
  // only allow certain envs
  //
  pl_whitelist_env("TERM");
  pl_whitelist_env("DISPLAY");
  pl_whitelist_env("HOME");
  pl_whitelist_env("PLASH_DATA");
  pl_whitelist_envs_from_env("PLASH_EXPORT");
  pl_whitelist_env(NULL);
    
  //
  // chroot, then reconstruct working directory
  //
  chroot(".") != -1 || pl_fatal("chroot");
  pl_chdir(origpwd);


  //
  // exec!
  //
  if (argv[optind] == NULL){

    pwd = getpwuid(0);
    default_shell = (pwd == NULL) ? pwd->pw_shell : "/bin/sh";

    execlp(default_shell, default_shell, "-l", NULL);
  } else {
    // use the positional arguments
    execvp(
           *(argv + optind),
             argv + optind);
  }
  // XXXXXXXXXXXXXXXXXX THIS HANDLING HEEEHHRE
  if (errno == ENONENT){
      fprintf(stderr, "%s: command not found\n", argv[0]);
      return 127; 
  }
  pl_fatal("exec");
} 
