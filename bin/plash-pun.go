package main

import "syscall"
import "bytes"
import "os"
import "io/ioutil"
import "os/exec"
import "fmt"
import "strconv"
import "path/filepath"
import "path"
import "runtime"

// think of a reason for a specific exit code number
const ERROR_EXIT_NUMBER = 120

const ERR_SETREUID_CONTEXT = "calling setreuid(0, 0)"
const ERR_SETREUID_HINT = "Run as root or set suid bit"

const ERR_READ_BOOT_ID_CONTEXT = "reading the file /proc/sys/kernel/random/boot_id"
const ERR_READ_BOOT_ID_HINT = "Maybe /proc is not mounted"

const ERR_READ_PLASH_ID_CONTEXT = "reading the file /var/lib/plash/id"
const ERR_READ_PLASH_ID_HINT = "run plash-init"

const ERR_TMP_DIR_CONTEXT = "create temporary directory in /var/tmp"
const ERR_TMP_DIR_HINT = ""

const ERR_READLINK_CONTEXT = "readlink %s"
const ERR_READLINK_HINT = ""

const ERR_SYMLINK_CONTEXT = "create symlink from %s to %s"
const ERR_SYMLINK_HINT = ""

const ERR_GETWD_CONTEXT = "get current directory"
const ERR_GETWD_HINT = ""

const ERR_CHROOT_CONTEXT = "crhoot into container filesystem at %s"
const ERR_CHROOT_HINT = ""

const ERR_CHDIR_ROOT_CONTEXT = "chdir to / after chroot into container"
const ERR_CHDIR_ROOT_HINT = ""

const ERR_EXEC_CONTEXT = "execing inside container" 
const ERR_EXEC_HINT = ""

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

func checkErr(err error, contextMsg string, hint string) {
        if err != nil { 
            fmt.Fprintf(os.Stderr, "Error while: %s")
            fmt.Fprintf(os.Stderr, err.Error())
            if (hint != ""){
                fmt.Fprintf(os.Stderr, "Hint: %s:", hint)
            }
            os.Exit(ERROR_EXIT_NUMBER)
        }
}

func pathExists(path string) (bool) {
        _, err := os.Stat(path)
        if err == nil { return true }
        if os.IsNotExist(err) { return false }
        panic(err)
}
func main() {

	container := os.Args[1]
	if !isint(container) {
		panic("argument must be an container, which is an integer")
	}

        // Think three times before removing this line.
        // without it, chroot could be executed in a thread that still is root.
        // That would be a privilege escalation
        runtime.LockOSThread() 

        callerUid := syscall.Getuid()
        err := syscall.Setreuid(0, 0);
        checkErr(err, ERR_SETREUID_CONTEXT, ERR_SETREUID_HINT)

        // the mountpoint to chroot into
        bootId, err := ioutil.ReadFile("/proc/sys/kernel/random/boot_id");
        checkErr(err, ERR_READ_BOOT_ID_CONTEXT, ERR_READ_BOOT_ID_HINT)

        plashId, err := ioutil.ReadFile("/var/lib/plash/id")
        checkErr(err, ERR_READ_PLASH_ID_CONTEXT, ERR_READ_PLASH_ID_HINT)

        finalMountpoint := fmt.Sprintf("/var/run/plash-run-suid-%s-%s-%d", bootId, plashId, container)

        //
        // populate mountpoint, if not done yet
        //
        if ! pathExists(finalMountpoint){
                
                mountpoint, err := ioutil.TempDir("/var/tmp", "plash-run-suid-mountpoint-")
                checkErr(err, ERR_TMP_DIR_CONTEXT, ERR_TMP_DIR_HINT)
                indexLink := fmt.Sprintf("/var/lib/plash/index/%d", container)
                topLayer, err := os.Readlink(indexLink);
                checkErr(err, fmt.Sprintf(ERR_READLINK_CONTEXT, indexLink), ERR_READLINK_HINT)

                // generate the overlay options for mount
                var buffer bytes.Buffer
                buffer.WriteString("lowerdir=")
                first := true
                for {
                        cont := filepath.Base(topLayer)
                        topLayer = filepath.Dir(topLayer)
                        if !isint(cont) {
                                break
                        }
                        if !first {
                                buffer.WriteString(":")
                        }
                        first = false
                        buffer.WriteString("/var/lib/plash/index/")
                        buffer.WriteString(cont)
                        buffer.WriteString("/_data/root")
                }
                buffer.WriteString(",nosuid")
                overlayOpts := buffer.String()

                // mount directories
                call("mount", "-t", "overlay", "overlay", "-o", overlayOpts, mountpoint)
                call("mount", "-t", "proc", "-o", "rw,nosuid,nodev,noexec,relatime",
                        "/proc", path.Join(mountpoint, "/proc"))
                call("mount", "-t", "none", "-o", "defaults,bind",
                        "/home", path.Join(mountpoint, "/home"))
                call("mount", "-t", "none", "-o", "defaults,bind",
                        "/etc/resolv.conf", path.Join(mountpoint, "/etc/resolv.conf"))
                call("mount", "-t", "none", "-o", "defaults,bind",
                        "/tmp", path.Join(mountpoint, "tmp"))
                
                // makes mounpoint reusing possible
                err = os.Symlink(mountpoint, finalMountpoint)

                // its ok if the symlink was already generated by another process
                // we could cleanup the mounts at this point
                if ! os.IsExist(err){
                        checkErr(err, fmt.Sprintf(
                            ERR_SYMLINK_CONTEXT, mountpoint, finalMountpoint),
                        ERR_SYMLINK_HINT)
                }
        }

	pwd, err := os.Getwd()
        checkErr(err, ERR_GETWD_CONTEXT, ERR_GETWD_HINT)
	err = syscall.Chroot(finalMountpoint)
        checkErr(err,
           fmt.Sprintf(ERR_CHROOT_CONTEXT, finalMountpoint),
           ERR_CHROOT_HINT)
        err = syscall.Setreuid(callerUid, callerUid)
	err = os.Chdir(pwd)
	if err != nil {
		err = os.Chdir("/");
                checkErr(err, ERR_CHDIR_ROOT_CONTEXT, ERR_CHDIR_ROOT_HINT)
	}
	err = syscall.Exec("/usr/bin/env", os.Args[2:], os.Environ())
        checkErr(err, ERR_EXEC_CONTEXT, ERR_EXEC_HINT)
}
