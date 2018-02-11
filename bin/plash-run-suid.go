package main

import "syscall"
import "bytes"
import "os"
import "io/ioutil"
import "os/exec"
// import "fmt"
import "strconv"
import "path/filepath"
import "path"
import "runtime"

func isint(val string) bool {
	if _, err := strconv.Atoi(val); err == nil {
		return true
	}
	return false
}

func call(name string, arg ...string) {
	err := exec.Command(name, arg...).Run()
	if err != nil {
		panic(err)
	}
}

func main() {

        runtime.LockOSThread() 

        callerUid := syscall.Getuid()

	container := os.Args[1]
	mountpoint, err := ioutil.TempDir("/var/run", "plash-run-suid-")
	if err != nil {
		panic(err)
	}

	if !isint(container) {
		panic("argument must be an container, which is an integer")
	}
	link, err := os.Readlink("/var/lib/plash/index/" + container)

	var buffer bytes.Buffer
	buffer.WriteString("lowerdir=")
	first := true
	for {
		cont := filepath.Base(link)
		link = filepath.Dir(link)
		if !isint(cont) {
			break
		}
		if !first {
			buffer.WriteString(":")
		}
		first = false
		buffer.WriteString("/var/lib/plash/index/")
		buffer.WriteString(cont)
		buffer.WriteString("/_data")
	}
	buffer.WriteString(",nosuid")

        err = syscall.Setreuid(0, 0)
	if err != nil {
		panic(err)
	}

	call("mount", "-t", "overlay", "overlay", "-o", buffer.String(), mountpoint)
	call("mount", "-t", "proc", "-o", "rw,nosuid,nodev,noexec,relatime",
		"/proc", path.Join(mountpoint, "/proc"))
	call("mount", "-t", "none", "-o", "defaults,bind",
		"/home", path.Join(mountpoint, "/home"))
	call("mount", "-t", "none", "-o", "defaults,bind",
		"/etc/resolv.conf", path.Join(mountpoint, "/etc/resolv.conf"))
	call("mount", "-t", "none", "-o", "defaults,bind",
		"/tmp", path.Join(mountpoint, "tmp"))

	pwd, err := os.Getwd()
	if err != nil {
		panic(err)
	}

	err = syscall.Chroot(mountpoint)
	if err != nil {
		panic(err)
	}
        err = syscall.Setreuid(callerUid, callerUid)
	if err != nil {
		panic(err)
	}

	err = os.Chdir(pwd)
	if err != nil {
		err = os.Chdir("/")
		if err != nil {
			panic(err)
		}
	}

	err = syscall.Exec("/usr/bin/env", os.Args[2:], os.Environ())
        if err != nil {
                panic(err)
        }

}
