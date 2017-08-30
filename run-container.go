package main

import "fmt"
import "os"
// import "encoding/json"
// import "encoding/hex"
import "os/exec"
import "os/user"
// import "strings"
// import "bytes"
import "io/ioutil"
import "syscall"
import "runtime"
// import "crypto/sha256"

//TODO:
// use as less syscalls as possible and rely more on golang libraries (apparently better guarantees there)
// parallelize the mount calls?
// golang is too high-level - the scheduler in the background magic sucks for this purpose - mayber write this in C if i want to learn it good enough.

// func usage(){
// 	fmt.Println('run-container --container cont command arg1 arg2... | squashfs_file_or_directory arg1 arg2...')
// 	os.Exit(1)
// }

func run(name string, args... string){
	c := exec.Command(name, args...)
	c.Env = make([]string, 0)
	output, err := c.CombinedOutput()
	if err != nil {
		fmt.Println(string(output)) // print to stderr is better
		panic(err)
	}
}


func check(err error){
	if err != nil {
		panic(err)
	}
}



func main() {
	runtime.LockOSThread() // golang has some scheduler or so that runs this in different os threads, but setuid affects only the current thread, soo we need this call

	// squashFile := filepath.Abs(cmd.Args[1])
	squashFile := os.Args[1]
	squashArgs := os.Args[1:]

	// cheapo file hashing
	var stat syscall.Stat_t
	err := syscall.Stat(squashFile, &stat)
	check(err)
	// m, err := json.Marshal(squashFile)
	// check(err)
	// h := sha256.New()
	// h.Write(m)
	// hash := hex.EncodeToString((h.Sum(nil)))[:16]
	inode := fmt.Sprintf("%v", stat.Ino)
	mountpoint := os.TempDir() + "/" + inode + ".mount"
	// mountpointExists := false
	var mountpointExists bool
	// err = os.Mkdir(mountpoint, 0755) // fix permissions later

	// something more robust
	if _, err := os.Stat(mountpoint); os.IsNotExist(err) {
		mountpointExists = false
	} else {
	
		mountpointExists = true
	}

	// if err != nil {
	// 	if ! os.IsExist(err) {
	// 		panic(err)
	// 	} else {
	// 		mountpointExists = true
	// 	}
	// } else {
	// 	mountpointExists = false
	// }
	// panic(fmt.Sprintf("%v", mountpointExists))
	originalUid := syscall.Getuid()
	err = syscall.Setreuid(0, 0)
	check(err)
	
	suffix := fmt.Sprintf("%v.", syscall.Getpid()) // code repitiion

	// if _, err := os.Stat(mountpoint); err != nil {
	//     if os.IsNotExist(err) {
	// 	// file does not exist
	//     } else {
	// 	// other error
	//     }
	// }


	if ! mountpointExists {


		tempMountpoint, err := ioutil.TempDir("/tmp", "mysuffix")
		check(err)

		run("/bin/mount", squashFile, tempMountpoint) // nosuid?



		// if !strings.HasPrefix(mountpoint, "/var/lib/plash/mnt/") {
			// panic("can't mount here")
		// }
		if originalUid != 0 {

			//
			// add entry to /etc/passwd for this user
			//
			// TODO: also update groups and shadow ?
			rawGuestPasswd, err := ioutil.ReadFile(tempMountpoint + "/etc/passwd")
			guestPasswd := string(rawGuestPasswd)
			check(err)
			newGuestPasswd, err := ioutil.TempFile("/var/lib/plash/tmp", suffix)
			check(err)
			user, err := user.LookupId(fmt.Sprintf("%v", originalUid))
			check(err)
			userHome := user.HomeDir
			userName := user.Username // the docu states that it can be empty or basically whatever: validate!
			addLine := fmt.Sprintf("%v:x:%v:%v:,,,:%v:/bin/sh\n", userName, originalUid, originalUid, userHome)
			_, err = newGuestPasswd.WriteString(guestPasswd + addLine)
			check(err)
			newGuestPasswd.Close()
			err = os.Chmod(newGuestPasswd.Name(), 0644) // the rights needed for the root after the chroot.
			check(err)
			run("/bin/mount", "--bind", "-o", "ro", newGuestPasswd.Name(), tempMountpoint + "/etc/passwd")
		}

		//
		// mount /dev and so on for the container
		//
		run("/bin/mount", "-t", "proc", "proc", tempMountpoint + "/proc")
		run("/bin/mount", "--bind", "-o", "ro", "/etc/resolv.conf", tempMountpoint + "/etc/resolv.conf")
		// run("mount", "--bind", "-o", "ro", newGuestShadow.Name(), mountpoint + "/etc/shadow")
		// run("mount", "--bind", "-o", "ro", "/etc/group", mountpoint + "/etc/group")
		mounts := [5]string{"/sys", "/dev", "/dev/pts", "/tmp", "/home"} // home should be noexec
		for _, mount := range mounts {
			// run("/bin/mount", "--bind", mount, mountpoint + mount)
			// Mostly use the mount binary for now but switch to the system call if i can set all mounts into stone
			err := syscall.Mount(mount, tempMountpoint + mount, "bind", syscall.MS_MGC_VAL|syscall.MS_BIND, "")
			check(err)
		}

		// err = os.Mkdir(mountpoint, 0755) // fix permissions later // "short" race codition here
		// check(err)
		// err = os.Link(tempMountpoint, mountpoint)
		// err = os.Rename(tempMountpoint, mountpoint)
		err = os.Symlink(tempMountpoint, mountpoint)
		check(err) // its ok if it already exists
		// fmt.Println(tempMountpoint)
		// run("/bin/mount", "--move", tempMountpoint, mountpoint)
	}

	pwd, err := os.Getwd()
	check(err)
	// panic(mountpoint)
	// fmt.Println(mountpoint)
	err = syscall.Chroot(mountpoint)
	check(err)
	err = os.Chdir("/")
	check(err)

	err = syscall.Setreuid(originalUid, originalUid)
	check(err)
	// some threads in this golang process are still root, i need to check if exec vanishes that

	// don't handle error, we are fine staying at /
	os.Chdir(pwd)


	// err = syscall.Exec("/bin/sh", squashArgs, os.Environ())
	err = syscall.Exec("/entrypoint", squashArgs, os.Environ())
	if err != nil {
		fmt.Fprintln(os.Stderr, "Could not exec inside the chroot:")
		fmt.Fprintln(os.Stderr, err)
		os.Exit(127)
	}
}

	// check input!!

	//
	// build up the lowerdir for the overlay filesystem
	//
	// layersList := strings.Split(os.Args[1], ":")
	// var buffer bytes.Buffer
	// lenLayersList := len(layersList)
	// for i := 0; i < lenLayersList; i++ {
	// 	layers := layersList[:i+1]
	// 	buffer.WriteString("/var/lib/plash/builds/")
	// 	for i2, element := range layers {
	// 		buffer.WriteString(element)
	// 		if i != i2 {
	// 			buffer.WriteString("/children/")
	// 		} else {
	// 			buffer.WriteString("/payload")
	// 		}
	// 	}
	// 	if (i != lenLayersList-1){
	// 		buffer.WriteString(":")
	// 	}
	// }
	// payloads := strings.Split(buffer.String(), ":") // use a more elegant way
	// buffer.WriteString(":/var/lib/plash/topdir")
	// lowerDirs := buffer.String()

	// h := sha256.New()
	// for _, payload := range payloads {
	// 	fi, err := os.Stat(payload)
	// 	check(err)
	// 	modtime := fi.ModTime().Unix()
	// 	h.Write([]byte(fmt.Sprintf("%v=%v;", payload, modtime)))
	// }
	// mountCacheKey := hex.EncodeToString((h.Sum(nil)))[:16]
	// if _, err := os.Stat("/path/to/whatever"); os.IsNotExist(err) {
	// 	// // 	// create mountpoint for the container
	// 	//
	// 	suffix := fmt.Sprintf("%v.", syscall.Getpid())
	// 	mountpoint, err := ioutil.TempDir("/var/lib/plash/mnt", suffix)
	// 	check(err)
	// 	err = os.Chmod(mountpoint, 0755) // the rights needed for the root after the chroot.
	// 	check(err)

	// 	// root stuff to comment better
	// 	originalUid := syscall.Getuid()
	// 	err = syscall.Setreuid(0, 0)
	// 	check(err)

	// 	oOpts := fmt.Sprintf("nosuid,lowerdir=%s", lowerDirs)
	// 	run("/bin/mount", "-t", "overlay", "overlay", "-o", oOpts, mountpoint)
	// } else {
	// 	panic("TODO: get mountpoint from link")
	// }

	// execInPreapredChroot(mountpoint, os.Args[2], os.Args[2:], originalUid)
// }
