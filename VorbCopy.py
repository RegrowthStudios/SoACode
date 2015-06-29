#!/usr/bin/env python
"""
Usage:
    prog [options]
"""
import os
import shutil


def get_environ():
    """Checks or updates environment for VORB PATH"""
    environ = os.environ
    vorb_path = environ.get("VORB_PATH", None)
    if vorb_path is None:
        filepath = os.path.dirname(os.path.abspath(__file__))
        local_instance = os.path.join(filepath, 'Vorb')
        separate_repo = os.path.join(os.path.dirname(filepath), 'Vorb')
        if os.path.exists(local_instance):
            print("Vorb found in current directory")
            vorb_path = local_instance
        elif os.path.exists(separate_repo):
            vorb_path = separate_repo
            print("Vorb found in parent directory")
        else:
            raise Exception("Unable to find Vorb")
    else:
        print("Using VORB_PATH environment variable")
    environ['VORB_PATH'] = vorb_path
    return environ


def copy_vorb(vorb_path):
    cwd = os.path.abspath(os.getcwd())
    includes = os.path.join(cwd, 'deps', 'include', 'Vorb')
    print "Copying {}/include to {}/".format(vorb_path, includes)

    # Copy 32-bit libraries
    lib32 = "Win32" if os.name == 'nt' else "lib32"
    lib32path = os.path.join(cwd, 'deps', 'lib', lib32)
    lib32_src = "{}/deps/lib/{}/".format(vorb_path, lib32)
    lib32_dest = "{}/".format(lib32path)
    print "Copying {} to {}".format(vorb_path, lib32, lib32path)
    shutil.copytree(lib32_src, lib32_dest)

    # Copy 64-bit libraries
    lib64 = "x64" if os.name == 'nt' else "lib64"
    lib64path = os.path.join(cwd, 'deps', 'lib', lib64)
    lib64_src = "{}/deps/lib/{}/".format(vorb_path, lib64)
    lib64_dest = "{}/".format(lib64path)
    print "Copying {} to {}".format(vorb_path, lib64, lib64path)
    shutil.copytree(lib64_src, lib64_dest)

    # Copy 32-bit precompiled headers
    pch32 = "Win32" if os.name == 'nt' else "lib32"
    pch32path = os.path.join(cwd, 'deps', 'lib', pch32)
    pch32_release_src = "{}/bin/{}/Release/Vorb.lib".format(vorb_path, pch32)
    pch32_release_dest = "{}/Vorb.lib".format(pch32path)
    pch32_debug_src = "{}/bin/{}/Debug/Vorb-d.lib".format(vorb_path, pch32)
    pch32_debug_dest = "{}/Vorb-d.lib".format(pch32path)
    print "Copying {} to {}".format(pch32_release_src, pch32_release_dest)
    shutil.copy2(pch32_release_src, pch32_release_dest)
    print "Copying {} to {}".format(pch32_debug_src, pch32_debug_dest)
    shutil.copy2(pch32_debug_src, pch32_debug_dest)

    # Copy 64-bit precompiled headers
    pch64 = "x64" if os.name == 'nt' else "lib64"
    pch64path = os.path.join(cwd, 'deps', 'lib', pch64)
    pch64_release_src = "{}/bin/{}/Release/Vorb.lib".format(vorb_path, pch64)
    pch64_release_dest = "{}/Vorb.lib".format(pch64path)
    pch64_debug_src = "{}/bin/{}/Debug/Vorb-d.lib".format(vorb_path, pch64)
    pch64_debug_dest = "{}/Vorb-d.lib".format(pch64path)
    print "Copying {} to {}".format(pch64_release_src, pch64_release_dest)
    shutil.copy2(pch64_release_src, pch64_release_dest)
    print "Copying {} to {}".format(pch64_debug_src, pch64_debug_dest)
    shutil.copy2(pch64_debug_src, pch64_debug_dest)


def main(args):
    """Main copy function"""
    environ = get_environ()
    copy_vorb(environ.get('VORB_PATH'))

if __name__ == "__main__":
    from docopt import docopt

    args = docopt(__doc__)
    main(args)
