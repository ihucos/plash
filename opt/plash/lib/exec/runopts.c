// usage: plash runopts -c CONTAINER [-d CHANGESDIR] [ -m MOUNT ] [CMD1 [ CMD2 ... ]]
//
// Run a container specifying lower level options.  Usually you'll want to use
// `plash run` instead. If no command is specified the containers default root
// shell will be executed.
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

#include <plash.h>

#define OPTSTRING "c:d:m:"
  
int main(int argc, char *argv[]) { 
  int opt; 
  char *changesdir = NULL;
  char *container = NULL;
  char *nodepath;
  char *plash_data;
  char *origpwd;
  struct passwd *pwd;

  plash_data = getenv("PLASH_DATA");
  assert(plash_data);
    

  while((opt = getopt(argc, argv, OPTSTRING)) != -1) {  
      switch(opt) {  
          case 'c':
              container = optarg;
              break;
          case 'd':
              changesdir = optarg;
              break;
          case ':':
              pl_usage();
          case '?':
              pl_usage();
      }  
  } 
  if (container == NULL) pl_usage();

  nodepath = pl_check_output((char*[]){
        "plash", "nodepath", container, NULL});

  pl_unshare_user();
  pl_unshare_mount();

  origpwd = get_current_dir_name();

  //
  // prepare an empty mountpoint
  //
  if (chdir(plash_data) == -1) pl_fatal("chdir");
  if (mount("tmpfs", "mnt", "tmpfs", MS_MGC_VAL, NULL) == -1) pl_fatal("mount");

  //
  // mount root filesystem
  //
  pl_check_output((char*[]){"plash", "mount", container, "mnt", changesdir, NULL});

  if (chdir("mnt") == -1) pl_fatal("chdir");

  optind = 1;
  while((opt = getopt(argc, argv, OPTSTRING)) != -1) { 
      if (opt == 'm') {
        if (!optarg[0] == '/')
          pl_fatal("mount path must be absolute");
        pl_bind_mount(optarg, optarg + 1);
      }  
  }  

  pl_whitelist_env("TERM");
  pl_whitelist_env("DISPLAY");
  pl_whitelist_env("HOME");
  pl_whitelist_env("PLASH_DATA");
  pl_whitelist_envs_from_env("PLASH_EXPORT");
  pl_whitelist_env(NULL);
    
  chroot(".") != -1 || pl_fatal("chroot");
  pl_chdir(origpwd);

  char **args = calloc(100, sizeof(char*)); // overflow!
  size_t i = 0;
  args[i++] = "/bin/sh";
  args[i++] = "-lc";
  args[i++] = "exec env \"$@\"";
  args[i++] = "--";
  if (argv[optind] == NULL){
    pwd = getpwuid(0);
    args[i++] = (pwd == NULL) ? pwd->pw_shell : "/bin/sh";
  } else {
    for(; argv[optind]; optind++)
      args[i++] = argv[optind];
  }
  args[i++] = NULL;

  execvp(args[0], args);
  fprintf(stderr, "%s: command not found\n", args[0]);
  return 127; 
} 



































//ALWAYS_EXPORT = ['TERM', 'DISPLAY', 'HOME', 'PLASH_DATA']
//
//
//import getopt
//import os
//import re
//import sys
//import tempfile
//from subprocess import CalledProcessError, check_call
//
//from plash import utils
//from plash.utils import (assert_initialized, catch_and_die, die,
//                         mkdtemp)
//
//utils.assert_initialized()
//utils.unshare_user()
//utils.unshare_mount()
//
//with utils.catch_and_die([getopt.GetoptError], debug='runopts'):
//    user_opts, user_args = getopt.getopt(sys.argv[1:], 'c:d:m:')
//
//#
//# prepare an empty mountpoint
//#
//mountpoint = os.path.join(os.environ["PLASH_DATA"], 'mnt')
//with catch_and_die([CalledProcessError]):
//    check_call(['mount', '-t', "tmpfs", "tmpfs", mountpoint], stdout=2)
//
//#
//# parse user options
//#
//container = None
//changesdir = None
//mounts = []
//for opt_key, opt_value in user_opts:
//
//    if opt_key == '-c':
//        container = opt_value
//
//    elif opt_key == '-d':
//        changesdir = opt_value
//
//    if opt_key == '-m':
//        mounts.append(opt_value)
//
//if not container:
//    die('runopts: missing -c option')
//
//#
//# mount root filesystem
//#
//if changesdir:
//    utils.plash_call('mount', '--', container, mountpoint, changesdir)
//else:
//    utils.plash_call('mount', '--', container, mountpoint)
//
//
//
//#
//# mount requested mounts if they exist
//#
//for mount in mounts:
//    mount_to = os.path.join(mountpoint, mount.lstrip('/'))
//    if os.path.exists(mount_to):
//        with catch_and_die([CalledProcessError]):
//            check_call(['mount', '--rbind', mount, mount_to], stdout=2)
//
//
//#
//# setup chroot and exec inside it
//#
//
//pwd_at_start = os.getcwd()
//
//# I had problems opening the files after the chroot (LookupError: unknown encoding: ascii)
//
//# read PATH from /etc/login.defs if available
//envs = []
//for env in os.environ.get('PLASH_EXPORT', '').split(':') + ALWAYS_EXPORT:
//    if env:
//        try:
//            envs.append('{}={}'.format(env, os.environ[env]))
//        except KeyError:
//            pass
//
//if not user_args:
//    cmd = [utils.get_default_shell(os.path.join(mountpoint, 'etc/passwd'))]
//else:
//    cmd = user_args
//
//os.chroot(mountpoint)
//try:
//    os.chdir(pwd_at_start)
//except (ValueError, OSError):
//    os.chdir('/')
//
//with catch_and_die([OSError]):
//    try:
//        os.execve('/bin/sh', ['sh', '-lc', 'exec env "$@"', '--'] + envs + cmd, {})
//    except FileNotFoundError:
//        sys.stderr.write('{}: command not found\n'.format(cmd[0]))
//        sys.exit(127)
