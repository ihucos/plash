package main

import "syscall"
import "os"

const PrimaryBinary = "/usr/local/bin/plash-run"
const SecondaryBinary = "/usr/bin/plash-run"

func main() {
	env := os.Environ()
	newEnv := make([]string, len(env)+1)
	newEnv[0] = "PLASH_RUN_SUID=1"
	for i := 1; i < len(env); i++ {
		newEnv[i] = "_" + env[i] // "escape" environment
	}
	args := os.Args[:]
	args[0] = PrimaryBinary

	err := syscall.Exec(PrimaryBinary, args, newEnv)
	if err != nil && err.Error() == "no such file or directory" {
		args[0] = SecondaryBinary
		err = syscall.Exec(SecondaryBinary, args, newEnv)
	}
	panic(err)
}
