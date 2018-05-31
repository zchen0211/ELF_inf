import os
import sys
import shlex
from datetime import datetime
import subprocess
from subprocess import check_output, call
import re


def confirm(s):
    while True:
        answer = input("Is that ok to start %s?" % s)
        if answer.lower() == "y":
            return True
        elif answer.lower() == "n":
            return False


def sig():
    return datetime.now().strftime("%m%d%y_%H%M%S_%f")


def run_remote(content, remote_hostname, dry_run=False):
    script_name = "tmp_" + sig() + ".sh"
    with open(script_name, "w") as f:
        f.write(content)

    copy_command = "scp %s %s:/tmp/" % (script_name, remote_hostname)
    shell_command = 'ssh -n %s "nohup bash /tmp/%s & echo start"' % (
        remote_hostname, script_name)

    # Send to a remote devgpu
    if dry_run:
        print(copy_command)
        print(shell_command)
    else:
        check_output(copy_command.split())
        proc = subprocess.Popen(
            shlex.split(shell_command),
            stdout=subprocess.PIPE)
        proc.stdout.readline()
        proc.kill()

        os.remove(script_name)


def run_local(content, dry_run=False):
    script_name = "tmp_" + sig() + ".sh"
    with open(script_name, "w") as f:
        f.write(content)

    if dry_run:
        print("bash " + script_name)
    else:
        call(["bash", script_name])
        os.remove(script_name)


matcher = re.compile(r"\{%[a-z_]+\}")


def get_scripts(templates, replacement):
    scripts = []
    for name, t, prompt in templates:
        curr_t = str(t)
        for k, v in replacement.items():
            if isinstance(v, bool):
                curr_t = curr_t.replace("{%%%s}" % k, ("--" + k if v else ""))
            else:
                curr_t = curr_t.replace("{%%%s}" % k, str(v))

        # Check whether there is anything undefined.
        m = matcher.search(curr_t)
        if m:
            print("Error! The symbol is not defined: " + m.group(0))
            sys.exit(1)

        scripts.append((name, curr_t, prompt))
    return scripts
