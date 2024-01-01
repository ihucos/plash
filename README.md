[![Build Status](https://travis-ci.org/ihucos/plash.svg?branch=master)](https://travis-ci.org/ihucos/plash) 

# Plash

Build and run layered root filesystems.


## Install
```
cd $(mktemp -d)
git clone git@github.com:ihucos/plash.git .
make
sudo cp dist/plash /usr/local/bin
```

## Uninstall
```
sudo rm /usr/local/bin/plash
```

## Requirements
  - `make` and `cc`
  - Linux Kernel >= 5.11
  - Optional `newuidmap` and `newgidmap` for setuid/setgid support with non-root users (needed by e.G. `apt`)

## Caveats

- Plash processes have the same operating system access rights than the process
  that started it. There is no security relevant isolation feature. Exactly as
  with running programs "normally", don't run programs you do not trust with
  plash and try to avoid using plash with the root user.

- Plash only runs inside Docker containers started with the `--privileged`
  flag, see GitHub issue #51 for details. 
  
## Plash vs Other Container Engines

Plash containers are not necessarily true containers because they do not fully isolate themselves from the host system and do not have additional security measures set in place. Instead, they are more like a combination of processes and containers, and can be treated like normal processes (e.g., they can be killed). Plash containers also have access to the home directory of the user who started them. To better understand this concept, refer to the provided diagram. 

```
Threads < Processes < Plash < Containers < Virtualisation < Physical computer
```

In general, the more to the left something is on the spectrum, the less flexible it is, but the more integrated it is with the system, allowing it to share existing resources. Plash containers are more constrained than traditional containers, but in exchange, they have access to resources that would typically only be available to processes.

### Resources plash containers share with it's caller
- Network access, including access to the hosts localhost
- The user's home directory (/home is mapped)
- The /tmp directory (during runtime, not during building)
- The Linux kernel (as with traditional containers)

### Resources unique to a plash containers
- The mount namespace
- The root folder, allowing running a different linux distribution

### Let's look at an example

I want to edit an image at my Desktop with the gimp image editor.

```
$ plash build --from lxc alpine:edge "apk add gimp"
112
$ plash run 112 gimp
```

Gimp automatically has access to my X-Server and pop ups on my screen. It also has access to my home folder and all my files. But it does run on an alpine distribution and pretty much only in that regard is independent from its host operating system.

### Conclusion

Plash containers are just a normal Linux process that happen to run on a different root filesystem. This means that they have their own set of benefits and drawbacks and may be more or less suitable for a particular use case.

## Basic concepts


### Plash image
A plash image refers to the file system of an operating system that was created usually by `plash build`. Plash images have a numeric id that can be passed to `plash run` in order to run an image. A image that is running may be referred as container. You could import a docker image into plash by calling `plash import-docker mydockerimage`.

### Plash container
A plash containers is a Linux processes that was started with the help of
plash. Typically you start a plash container with the `plash run` subcommand.
E. G. `plash run 23 cowsay hi`. Since plash containers are just Linux processes
you can list them with `ps` or `top` and kill them with `kill`.  

### Plash macro
A plash macro may also be referred as a build command. Macros are instructions
used to build images. One example is the `apt` macro which installs any given
package or packages with the `apt` package manager. A more complete example for
the usage of the `apt` macro could be: `plash build --from ubuntu:focal --apt
nmap`. Internally a macro does nothing more than to emit shell code that is
executed when an image is build. Use `plash --help-macros` to list all macros.

### Plash build file
A plash build file is a file containing a set of macros. Building an image from
a build file can be achieved with following command `plash build --eval-file
./my-plash-build-file`.  Interestingely `eval-file` which is used to "run"
build files is itself a macro. As macros emit shell code so do plash build
files.

### Plash executable
Take a build file, have `#!/usr/bin/env plash` as its first line, mark it
as executable and specify it's entrypoint executable with the `entrypoint` macro.
That is a plash executable. Now you you can build and run containerized
software without knowing that containers or plash exist.




## Use Case: Containerzied Project Environment

When developing software together with other developers, for example in the
context of a company, it might make sense to standartize how the developed
software is run for development. Tooling needed for development might also be
containerized. The advantages are faster onboarding of new developers and
better reproducability of the software along the differnet development
computers.


Let's take a look at the following software.

```
# app.py
from flask import Flask
app = Flask(__name__)
@app.route('/')
def index():
    return 'Web App with Python Flask'
app.run(host='127.0.0.0.1', port=8080)
```

We could instruct developers do download `python3` and install `flask`, which might
be specified at the `requirements.txt` file. Or better let's use a container to
run that software without involving any manual setup steps.

We could write a plash executable and ask users to run that. Save that file to
`./runapp` and mark it as executable

```
#!/usr/bin/env plash
FROM lxc alpine:edge
apk update
apk add py3-pip
LAYER
HASH ./requirements.txt
pip3 install -r ./requirements.txt

SCRIPT /entrypoint
exec python3 app.py
```


Now developers can run the application simply by executing `./runapp`. When the
`requirements.txt` file changes, all steps after the `--hash-path` build
command are rerun so that modification in the requirements file can take
effect.

We can also take this one step forward and containerize development tools.
Create a directory called `devtools`, then add a file called `yapf` to it.

```
#!/usr/bin/env plash
FROM alpine:edge
apk update
apk add py3-pip
pip3 install yapf==0.32.0
ln -s /usr/local/bin/yapf /entrypoint
```

Add the `devtools` directory to your `PATH` environment variable. Now every time
you type in `yapf` into your terminal, this containerized version will be used.
One advantage is that every developer will have the same `yapf` version.



## Development Guidelines

- Keep the script character.
- Don't fall in love with the code, embrace its absence.
- All dependencies will get unmaintained at some point.
- Use honest thin wrappers, documented leaky abstractions are better then difficult promises.
- Don't be a monolith but don't try too hard not to be one.
- Don't complain or warn via stderr, do it or don't do it.
- Only be as smart as necessary and keep it simple and stupid (KISS).
- Still be able to run this in five years without any maintenance work.
- No baggage, no worries.
- Define well what this project is and especially what it is not.
- Say no to features, say yes to solved use cases.
- Postpone compromises.
- Ditch everything that turns out too fiddly.
- Be as vanilla as you can be
- Be humble, don't oversell your abstraction layer.
- Sometimes the dirty solution is cleaner than the proper one.
- Don't differentiate root from non-root users (this is a TODO)
- Crude is better than complex.
- Only eat your own dog food if you are hungry.
- Work towards a timeless, finished product that will require no maintenance.
- Don't write C just because it looks cool, use the right tool for the right job.
- Cognitive load for endusers does matter after all
- The right guidelines for the right situation.


## User Interface Guidelines
- Interface follows code
- Code supplements documentation
- Don't make difficult things seem easy
- Don't be too verbose, usually only information about success or failure matter
- Avoid too many features slowly getting in
- The UI is not a marketing instrument
- Just a prise of pragmatism
- Users don't know what they want
- user errors are the user's fault
- Rude is better than sorry
- Technical descriptions do not get outdated, reasoning and interpretations do.

## FAQ

### Can I contribute?
Please! Write me an mail mail@irae.me, open an issue, do a pull request or ask
me out for a friendly chat about plash in Berlin.

### Who are you?
A Django/Python software-developer. Since this is an open source project I hope
this software grows organically and collaboratively.

### Why write a containerization software?
Technical idealism. I wanted a better technical solution for a problem. In my
personal opinion Docker is revolutionary but has some shortcomings: awkward
interface, reinvention of established software or interfaces, bundling, vendor
lock in and overengineering. In a way it kills it's idea by trying too hard to
build a huge company on top of it. Plash thrives not to be more than a useful
tool with one task: Building and running containerized processes. Ultimately I
wanted something I can if necessary maintain by myself.

### Are there plans to commercialise this?
No, there isn't. At the same time I don't want to risk disappointing anyone and
am not making any absolute guarantees.

### What is the Licence?
plash is licensed under the MIT Licence.

### How does plash compare to Docker?
Docker is a bloated SUV you have to bring to the car workshop every week, for
random alterations, features and new advertising stickers. Plash is a nice
fixed gear bike, but the welds are still hot and nobody checked the bolts yet.

### Can I run this in production?
No guarantees.

### Is plash secure?
Plash does not use any daemons or have its own setuid helper binaries. Note
that plash does not try to isolate containers (which are just normal
processes). That means that running a program inside plash is not a security
feature. Running any container software introduces more entities to trust, that
is the root file system image with its additional linux distribution and its
own package manager. Using a program from alpine edge could be considered less
secure than a package from debian stable or vice versa. Also note that keeping
containers updated is more difficult than keeping "normal" system software
updated. Furthermore note that programs could be not used to run inside
semi-isolated containers and behave oddly. Plash uses unmodified lxc images.
Using plash as root should be avoided and should not be necessary for most use
cases.  Until now plash was written by one person and of course I could be
wrong about something. But generally speaking it really should be good enough.
