package main

import "fmt"
import "os"
import "os/exec"
// import "os/user"
import "strings"
import "io/ioutil"
import "syscall"
import "runtime"

func run(name string, args ...string) {
	c := exec.Command(name, args...)
	c.Env = make([]string, 0)
	output, err := c.CombinedOutput()
	if err != nil {
		fmt.Println(string(output)) // print to stderr is better
		panic(err)
	}
}

func check(err error) {
	if err != nil {
		panic(err)
	}
}

func main() {
	// golang has some scheduler or so that runs this in different os threads, but setuid affects only the current thread, soo we need this call
	runtime.LockOSThread()

	squashFile := os.Args[0] + ".squashfs"
	rootfsDir := os.Args[0] + ".dir"
	squashArgs := os.Args[1:]

	var stat syscall.Stat_t
	isSquashfile := false
	err := syscall.Stat(rootfsDir, &stat)
	if os.IsNotExist(err) {
		isSquashfile = true
		err = syscall.Stat(squashFile, &stat)
		check(err)
	}

	inode := fmt.Sprintf("%v", stat.Ino)
	device := fmt.Sprintf("%v", stat.Dev)
	bootId, err := ioutil.ReadFile("/proc/sys/kernel/random/boot_id")
	check(err)
	suffix := fmt.Sprintf("runp.%v.%v.%v.", strings.Trim(string(bootId), "\n"), device, inode) // code repitition

	mountpoint := "/var/tmp/" + suffix + "link"
	// mountpointExists := false
	var mountpointExists bool

	// something more robust
	if _, err := os.Stat(mountpoint); os.IsNotExist(err) {
		mountpointExists = false
	} else {

		mountpointExists = true
	}

	originalUid := syscall.Getuid()
	err = syscall.Setreuid(0, 0)
	check(err)

	if !mountpointExists {

		tempMountpoint, err := ioutil.TempDir("/var/tmp", suffix)
		check(err)

		if isSquashfile {
			run("/bin/mount", squashFile, tempMountpoint) // nosuid?
		} else {
			run("/bin/mount", "--bind", "-o", "ro", rootfsDir, tempMountpoint) // nosuid?
		}
		// mount directories used in container
		run("/bin/mount", "-t", "proc", "proc", tempMountpoint+"/proc")
		run("/bin/mount", "--bind", "-o", "ro", "/etc/resolv.conf", tempMountpoint+"/etc/resolv.conf")
		mounts := [4]string{"/sys", "/dev", "/tmp", "/home"} // home should be noexec
		for _, mount := range mounts {
			// run("/bin/mount", "--bind", mount, tempMountpoint + mount)
			// Mostly use the mount binary for now but switch to the system call if i can set all mounts into stone
			err := syscall.Mount(mount, tempMountpoint+mount, "bind", syscall.MS_MGC_VAL|syscall.MS_BIND, "")
			check(err)
		}
		run("/bin/mount", "--bind", "/run", tempMountpoint+"/run") // we want /var/run but its a symlink, we need to autmotazize following it

		err = os.Symlink(tempMountpoint, mountpoint)
		check(err) // its ok if it already exists
	}

	pwd, err := os.Getwd()
	check(err)
	err = syscall.Chroot(mountpoint)
	check(err)
	err = os.Chdir("/")
	check(err)

	// SECURITY: also do that with the group id and what about supplementary groups
	err = syscall.Setreuid(originalUid, originalUid)
	check(err)
	// some threads in this golang process are still root, i need to check if exec vanishes that TODO: create issue

	// don't handle error, we are fine staying at /
	os.Chdir(pwd)

	myexec, err := os.Readlink("/etc/runp/exec")
	check(err)

	argv0 := []string{myexec}
	err = syscall.Exec(myexec, append(argv0, squashArgs...), os.Environ())
	if err != nil {
		fmt.Fprintln(os.Stderr, "Could not exec inside the chroot:")
		fmt.Fprintln(os.Stderr, err)
		os.Exit(127)
	}
}
