#!/usr/bin/env python3
from __future__ import print_function
"""\
@file viewer_manifest.py
@author Ryan Williams
@brief Description of all installer viewer files, and methods for packaging
       them into installers for all supported platforms.

$LicenseInfo:firstyear=2006&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2006-2014, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
"""
import errno
import json
import os
import os.path
import plistlib
import random
import re
import shutil
import stat
import subprocess
import sys
import tarfile
import time
import zipfile

viewer_dir = os.path.dirname(__file__)
# Add indra/lib/python to our path so we don't have to muck with PYTHONPATH.
# Put it FIRST because some of our build hosts have an ancient install of
# indra.util.llmanifest under their system Python!
sys.path.insert(0, os.path.join(viewer_dir, os.pardir, "lib", "python"))
from indra.util.llmanifest import LLManifest, main, proper_windows_path, path_ancestors, CHANNEL_VENDOR_BASE, RELEASE_CHANNEL, ManifestError
import llsd

class ViewerManifest(LLManifest):
    def is_packaging_viewer(self):
        # Some commands, files will only be included
        # if we are packaging the viewer on windows.
        # This manifest is also used to copy
        # files during the build (see copy_w_viewer_manifest
        # and copy_l_viewer_manifest targets)
        return 'package' in self.args['actions']

    def package_skin(self, xml, skin_dir):
        self.path(xml)
        self.path(skin_dir + "/*")
        # include the entire textures directory recursively
        with self.prefix(src_dst=skin_dir+"/textures"):
            self.path("*/*.tga")
            self.path("*/*.j2c")
            self.path("*/*.jpg")
            self.path("*/*.png")
            self.path("*.tga")
            self.path("*.j2c")
            self.path("*.jpg")
            self.path("*.png")
            self.path("*.xml")

    def construct(self):
        super(ViewerManifest, self).construct()
        self.path(src="../../scripts/messages/message_template.msg", dst="app_settings/message_template.msg")
        self.path(src="../../etc/message.xml", dst="app_settings/message.xml")

        if True: #self.is_packaging_viewer():
            with self.prefix(src_dst="app_settings"):
                self.exclude("logcontrol.xml")
                self.exclude("logcontrol-dev.xml")
                self.path("*.crt")
                self.path("*.ini")
                self.path("*.xml")
                self.path("*.db2")

                # include the entire shaders directory recursively
                self.path("shaders")

                # ... and the entire windlight directory
                self.path("windlight")

                # ... and the included spell checking dictionaries
                pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
                with self.prefix(src=pkgdir):
                    self.path("dictionaries")

                # include the extracted packages information (see BuildPackagesInfo.cmake)
                self.path(src=os.path.join(self.args['build'],"packages-info.txt"), dst="packages-info.txt")


            with self.prefix(src_dst="character"):
                self.path("*.llm")
                self.path("*.xml")
                self.path("*.tga")

            # Include our fonts (3p-viewer-fonts package)
            with self.prefix(src=os.path.join(pkgdir, "fonts"), dst="fonts"):
                self.path("DejaVuSans.ttf")
                self.path("DejaVuSans-Bold.ttf")
                self.path("DejaVuSans-Oblique.ttf")
                self.path("DejaVuSans-BoldOblique.ttf")
                self.path("DejaVuSansMono.ttf")
                self.path("TwemojiSVG.ttf")

            # Include our font licenses
            with self.prefix(src_dst="fonts"):
                self.path("*.txt")

            # skins
            with self.prefix(src_dst="skins"):
                self.path("paths.xml")
                self.path("default/xui/*/*.xml")
                # default folder: base XUI fallback assets (not a selectable skin)
                self.path("default/colors.xml")
                self.path("default/colors_base.xml")
                with self.prefix(src_dst="default/textures"):
                    self.path("*/*.tga")
                    self.path("*/*.j2c")
                    self.path("*/*.jpg")
                    self.path("*/*.png")
                    self.path("*.tga")
                    self.path("*.j2c")
                    self.path("*.jpg")
                    self.path("*.png")
                    self.path("*.xml")
                self.package_skin("Cyber.xml", "cyber")

                # Local HTML files (e.g. loading screen)
                with self.prefix(src_dst="*/html"):
                    self.path("*.png")
                    self.path("*/*/*.html")
                    self.path("*/*/*.gif")

            # File in the newview/ directory
            self.path("gpu_table.txt")

            #build_data.json.  Standard with exception handling is fine.  If we can't open a new file for writing, we have worse problems
            #platform is computed above with other arg parsing
            build_data_dict = {"Type":"viewer","Version":'.'.join(self.args['version']),
                            "Channel Base": CHANNEL_VENDOR_BASE,
                            "Channel":self.channel_with_pkg_suffix(),
                            "Platform":self.build_data_json_platform,
                            "Address Size":self.address_size,
                            "Update Service":"https://app.alchemyviewer.org/update",
                            }
            build_data_dict = self.finish_build_data_dict(build_data_dict)
            with open(os.path.join(os.pardir,'build_data.json'), 'w') as build_data_handle:
                json.dump(build_data_dict,build_data_handle)

            #we likely no longer need the test, since we will throw an exception above, but belt and suspenders and we get the
            #return code for free.
            if not self.path2basename(os.pardir, "build_data.json"):
                print("No build_data.json file")

    def standalone(self):
        return self.args['standalone'] == "ON"

    def finish_build_data_dict(self, build_data_dict):
        return build_data_dict

    def grid(self):
        return self.args['grid']

    def viewer_branding_id(self):
        return self.args['branding_id']

    def channel(self):
        return self.args['channel']

    def channel_with_pkg_suffix(self):
        fullchannel=self.channel()
        channel_suffix = self.args.get('channel_suffix')
        if channel_suffix:
            fullchannel+=' '+channel_suffix
        return fullchannel

    def channel_variant(self):
        global CHANNEL_VENDOR_BASE
        return self.channel().replace(CHANNEL_VENDOR_BASE, "").strip()

    def channel_type(self): # returns 'release', 'beta', 'project', or 'test'
        channel_qualifier=self.channel_variant().lower()
        if channel_qualifier.startswith('release'):
            channel_type='release'
        elif channel_qualifier.startswith('beta'):
            channel_type='beta'
        elif channel_qualifier.startswith('alpha'):
            channel_type='alpha'
        elif channel_qualifier.startswith('project'):
            channel_type='project'
        else:
            channel_type='test'
        return channel_type

    def channel_variant_app_suffix(self):
        # get any part of the channel name after the CHANNEL_VENDOR_BASE
        suffix=self.channel_variant()
        # by ancient convention, we don't use Release in the app name
        if self.channel_type() == 'release':
            suffix=suffix.replace('Release', '').strip()
        # for the base release viewer, suffix will now be null - for any other, append what remains
        if suffix:
            suffix = "_".join([''] + suffix.split())
        # the additional_packages mechanism adds more to the installer name (but not to the app name itself)
        # ''.split() produces empty list, so suffix only changes if
        # channel_suffix is non-empty
        suffix = "_".join([suffix] + self.args.get('channel_suffix', '').split())
        return suffix

    def installer_base_name(self):
        global CHANNEL_VENDOR_BASE
        # a standard map of strings for replacing in the templates
        substitution_strings = {
            'channel_vendor_base' : '_'.join(CHANNEL_VENDOR_BASE.split()),
            'channel_variant_underscores':self.channel_variant_app_suffix(),
            'version_underscores' : '_'.join(self.args['version']),
            'arch':self.args['arch']
            }
        return "%(channel_vendor_base)s%(channel_variant_underscores)s_%(version_underscores)s_%(arch)s" % substitution_strings

    def app_name(self):
        global CHANNEL_VENDOR_BASE
        channel_type=self.channel_type()
        if channel_type == 'release':
            app_suffix=''
        else:
            app_suffix=self.channel_variant()
        return CHANNEL_VENDOR_BASE + ' ' + app_suffix

    def app_name_oneword(self):
        return ''.join(self.app_name().split())

    def icon_path(self):
        return "icons/" + ("default", "alpha")[self.channel_type() == "alpha"]

    def extract_names(self,src):
        try:
            contrib_file = open(src,'r')
        except IOError:
            print("Failed to open '%s'" % src)
            raise
        lines = contrib_file.readlines()
        contrib_file.close()

        # All lines up to and including the first blank line are the file header; skip them
        lines.reverse() # so that pop will pull from first to last line
        while not re.match(r"\s*$", lines.pop()) :
            pass # do nothing

        # A line that starts with a non-whitespace character is a name; all others describe contributions, so collect the names
        names = []
        for line in lines :
            if re.match(r"\S", line) :
                names.append(line.rstrip())
        # It's not fair to always put the same people at the head of the list
        random.shuffle(names)
        return ', '.join(names)

    def relsymlinkf(self, src, dst=None, catch=True):
        """
        relsymlinkf() is just like symlinkf(), but instead of requiring the
        caller to pass 'src' as a relative pathname, this method expects 'src'
        to be absolute, and creates a symlink whose target is the relative
        path from 'src' to dirname(dst).
        """
        dstdir, dst = self._symlinkf_prep_dst(src, dst)

        # Determine the relative path starting from the directory containing
        # dst to the intended src.
        src = self.relpath(src, dstdir)

        self._symlinkf(src, dst, catch)
        return dst

    def symlinkf(self, src, dst=None, catch=True):
        """
        Like ln -sf, but uses os.symlink() instead of running ln. This creates
        a symlink at 'dst' that points to 'src' -- see:
        https://docs.python.org/2/library/os.html#os.symlink

        If you omit 'dst', this creates a symlink with basename(src) at
        get_dst_prefix() -- in other words: put a symlink to this pathname
        here at the current dst prefix.

        'src' must specifically be a *relative* symlink. It makes no sense to
        create an absolute symlink pointing to some path on the build machine!

        Also:
        - We prepend 'dst' with the current get_dst_prefix(), so it has similar
          meaning to associated self.path() calls.
        - We ensure that the containing directory os.path.dirname(dst) exists
          before attempting the symlink.

        If you pass catch=False, exceptions will be propagated instead of
        caught.
        """
        dstdir, dst = self._symlinkf_prep_dst(src, dst)
        self._symlinkf(src, dst, catch)
        return dst

    def _symlinkf_prep_dst(self, src, dst):
        # helper for relsymlinkf() and symlinkf()
        if dst is None:
            dst = os.path.basename(src)
        dst = os.path.join(self.get_dst_prefix(), dst)
        # Seems silly to prepend get_dst_prefix() to dst only to call
        # os.path.dirname() on it again, but this works even when the passed
        # 'dst' is itself a pathname.
        dstdir = os.path.dirname(dst)
        self.cmakedirs(dstdir)
        return (dstdir, dst)

    def _symlinkf(self, src, dst, catch):
        # helper for relsymlinkf() and symlinkf()
        # the passed src must be relative
        if os.path.isabs(src):
            raise ManifestError("Do not symlinkf(absolute %r, asis=True)" % src)

        # The outer catch is the one that reports failure even after attempted
        # recovery.
        try:
            # At the inner layer, recovery may be possible.
            try:
                os.symlink(src, dst)
            except OSError as err:
                if err.errno != errno.EEXIST:
                    raise
                # We could just blithely attempt to remove and recreate the target
                # file, but that strategy doesn't work so well if we don't have
                # permissions to remove it. Check to see if it's already the
                # symlink we want, which is the usual reason for EEXIST.
                elif os.path.islink(dst):
                    if os.readlink(dst) == src:
                        # the requested link already exists
                        pass
                    else:
                        # dst is the wrong symlink; attempt to remove and recreate it
                        os.remove(dst)
                        os.symlink(src, dst)
                elif os.path.isdir(dst):
                    print("Requested symlink (%s) exists but is a directory; replacing" % dst)
                    shutil.rmtree(dst)
                    os.symlink(src, dst)
                elif os.path.exists(dst):
                    print("Requested symlink (%s) exists but is a file; replacing" % dst)
                    os.remove(dst)
                    os.symlink(src, dst)
                else:
                    # out of ideas
                    raise
        except Exception as err:
            # report
            print("Can't symlink %r -> %r: %s: %s" %  (dst, src, err.__class__.__name__, err))
            # if caller asked us not to catch, re-raise this exception
            if not catch:
                raise

    def relpath(self, path, base=None, symlink=False):
        """
        Return the relative path from 'base' to the passed 'path'. If base is
        omitted, self.get_dst_prefix() is assumed. In other words: make a
        same-name symlink to this path right here in the current dest prefix.

        Normally we resolve symlinks. To retain symlinks, pass symlink=True.
        """
        if base is None:
            base = self.get_dst_prefix()

        # Since we use os.path.relpath() for this, which is purely textual, we
        # must ensure that both pathnames are absolute.
        if symlink:
            # symlink=True means: we know path is (or indirects through) a
            # symlink, don't resolve, we want to use the symlink.
            abspath = os.path.abspath
        else:
            # symlink=False means to resolve any symlinks we may find
            abspath = os.path.realpath

        return os.path.relpath(abspath(path), abspath(base))


class WindowsManifest(ViewerManifest):
    # We want the platform, per se, for every Windows build to be 'win'. The
    # VMP will concatenate that with the address_size.
    build_data_json_platform = 'win'

    def final_exe(self):
        # "AllynViewer.exe" for the release channel, "AllynViewerBeta.exe" etc. otherwise.
        return self.app_name_oneword()+".exe"

    def finish_build_data_dict(self, build_data_dict):
        #MAINT-7294: Windows exe names depend on channel name, so write that in also
        build_data_dict['Executable'] = self.final_exe()
        build_data_dict['AppName']    = self.app_name()
        return build_data_dict

    def test_msvcrt_and_copy_action(self, src, dst):
        # This is used to test a dll manifest.
        # It is used as a temporary override during the construct method
        from test_win32_manifest import test_assembly_binding
        # TODO: This is redundant with LLManifest.copy_action(). Why aren't we
        # calling copy_action() in conjunction with test_assembly_binding()?
        if src and (os.path.exists(src) or os.path.islink(src)):
            # ensure that destination path exists
            self.cmakedirs(os.path.dirname(dst))
            self.created_paths.append(dst)
            if not os.path.isdir(src):
                if(self.args['configuration'].lower() == 'debug'):
                    test_assembly_binding(src, "Microsoft.VC80.DebugCRT", "8.0.50727.4053")
                else:
                    test_assembly_binding(src, "Microsoft.VC80.CRT", "8.0.50727.4053")
                self.ccopy(src,dst)
            else:
                raise Exception("Directories are not supported by test_CRT_and_copy_action()")
        else:
            print("Doesn't exist:", src)

    def test_for_no_msvcrt_manifest_and_copy_action(self, src, dst):
        # This is used to test that no manifest for the msvcrt exists.
        # It is used as a temporary override during the construct method
        from test_win32_manifest import test_assembly_binding
        from test_win32_manifest import NoManifestException, NoMatchingAssemblyException
        # TODO: This is redundant with LLManifest.copy_action(). Why aren't we
        # calling copy_action() in conjunction with test_assembly_binding()?
        if src and (os.path.exists(src) or os.path.islink(src)):
            # ensure that destination path exists
            self.cmakedirs(os.path.dirname(dst))
            self.created_paths.append(dst)
            if not os.path.isdir(src):
                try:
                    if(self.args['configuration'].lower() == 'debug'):
                        test_assembly_binding(src, "Microsoft.VC80.DebugCRT", "")
                    else:
                        test_assembly_binding(src, "Microsoft.VC80.CRT", "")
                    raise Exception("Unknown condition")
                except NoManifestException as err:
                    pass
                except NoMatchingAssemblyException as err:
                    pass

                self.ccopy(src,dst)
            else:
                raise Exception("Directories are not supported by test_CRT_and_copy_action()")
        else:
            print("Doesn't exist:", src)

    def construct(self):
        super(WindowsManifest, self).construct()

        if self.args['configuration'].lower() == '.':
            config = 'debug' if self.args['buildtype'].lower() == 'debug' else 'release'
        else:
            config = 'debug' if self.args['configuration'].lower() == 'debug' else 'release'
        pkgdir = os.path.join(self.args['build'], os.pardir, 'packages')
        relpkgdir = os.path.join(pkgdir, "lib", "release")
        debpkgdir = os.path.join(pkgdir, "lib", "debug")
        pkgbindir = os.path.join(pkgdir, "bin", config)

        if True: #self.is_packaging_viewer():
            # Find Allyn-bin.exe in the 'configuration' dir, then rename it to the result of final_exe.
            self.path(src=os.path.join(self.args['dest'], ('%s-bin.exe' % self.viewer_branding_id())), dst=self.final_exe())

        # Plugin host application
        self.path2basename(os.path.join(os.pardir,
                                        'llplugin', 'slplugin', config),
                           "SLplugin.exe")

        # Get shared libs from the shared libs staging directory
        with self.prefix(src=os.path.join(self.args['build'], os.pardir,
                                          'sharedlibs', config)):

            # Get llcommon and deps. If missing assume static linkage and continue.
            if self.path('llcommon.dll') == 0:
                print("Skipping llcommon.dll (assuming llcommon was linked statically)")

            self.path('libapr-1.dll')
            self.path('libaprutil-1.dll')
            self.path('libapriconv-1.dll')

            # Mesh 3rd party libs needed for auto LOD and collada reading
            if self.path("glod.dll") == 0:
                print("Skipping GLOD library (assumming linked statically)")

            # Get OpenAL dlls, continue if missing
            if self.path("alut.dll") == 0 or self.path("OpenAL32.dll") == 0:
                print("Skipping OpenAL audio library (assuming other audio engine)")

            # Vivox runtimes
            self.path("llwebrtc.dll")
            #self.path("libsndfile-1.dll")
            
            # Security
            self.path("libcrypto-1_1-x64.dll")
            self.path("libssl-1_1-x64.dll")

            # Hunspell
            self.path("libhunspell.dll")

        # For crashpad
        with self.prefix(src=pkgbindir):
            self.path("crashpad_handler.exe")
            if not self.is_packaging_viewer():
                self.path("crashpad_handler.pdb")

        self.path(src="licenses-windows.txt", dst="licenses.txt")
        self.path("featuretable.txt")

        # Plugins
        with self.prefix(dst="llplugin"):
            with self.prefix(src=os.path.join(self.args['build'], os.pardir, 'plugins')):

                # Plugins - FilePicker
                with self.prefix(src=os.path.join('filepicker', config)):
                    self.path("basic_plugin_filepicker.dll")

                # Media plugins - LibVLC
                with self.prefix(src=os.path.join('libvlc', config)):
                    self.path("media_plugin_libvlc.dll")

                # Media plugins - CEF
                with self.prefix(src=os.path.join('cef', config)):
                    self.path("media_plugin_cef.dll")

            # CEF runtime files - debug
            # CEF runtime files - not debug (release, relwithdebinfo etc.)
            # CEF 139+: pak names are chrome_*/resources.pak; GPU needs Vulkan/DXC runtime.
            with self.prefix(src=pkgbindir):
                self.path("chrome_elf.dll")
                self.path("d3dcompiler_47.dll")
                self.path("dxcompiler.dll")
                self.path("dxil.dll")
                self.path("libcef.dll")
                self.path("libEGL.dll")
                self.path("libGLESv2.dll")
                self.path("v8_context_snapshot.bin")
                self.path("vk_swiftshader.dll")
                self.path("vk_swiftshader_icd.json")
                self.path("vulkan-1.dll")
                self.path("dullahan_host.exe")

            # CEF files common to all configurations
            with self.prefix(src=os.path.join(pkgdir, 'resources')):
                self.path("chrome_100_percent.pak")
                self.path("chrome_200_percent.pak")
                self.path("resources.pak")
                self.path("icudtl.dat")

            cef_locale_paks = (
                "en-US.pak",
                "es.pak",
                "pt-BR.pak",
                "fr.pak",
                "de.pak",
                "it.pak",
                "tr.pak",
                "ru.pak",
                "ja.pak",
            )
            with self.prefix(src=os.path.join(pkgdir, 'resources', 'locales'), dst='locales'):
                for pak in cef_locale_paks:
                    self.path(pak)
                dest_locales = self.get_dst_prefix()
                if os.path.isdir(dest_locales):
                    wanted = set(cef_locale_paks)
                    for name in os.listdir(dest_locales):
                        if name.endswith(".pak") and name not in wanted:
                            try:
                                os.remove(os.path.join(dest_locales, name))
                            except OSError:
                                pass

            with self.prefix(src=pkgbindir):
                self.path("libvlc.dll")
                self.path("libvlccore.dll")
                self.path("plugins/")


        if not self.is_packaging_viewer():
            self.package_file = "copied_deps"

    def nsi_file_commands(self, install=True):
        def wpath(path):
            if path.endswith('/') or path.endswith(os.path.sep):
                path = path[:-1]
            path = path.replace('/', '\\')
            return path

        result = ""
        dest_files = [pair[1] for pair in self.file_list if pair[0] and os.path.isfile(pair[1])]
        # sort deepest hierarchy first
        dest_files.sort(key=lambda a: (-a.count(os.path.sep), a))
        out_path = None
        for pkg_file in dest_files:
            rel_file = os.path.normpath(pkg_file.replace(self.get_dst_prefix()+os.path.sep,''))
            installed_dir = wpath(os.path.join('$INSTDIR', os.path.dirname(rel_file)))
            pkg_file = wpath(os.path.normpath(pkg_file))
            if installed_dir != out_path:
                if install:
                    out_path = installed_dir
                    result += 'SetOutPath ' + out_path + '\n'
            if install:
                result += 'File "' + pkg_file + '"\n'
            else:
                result += 'Delete "' + wpath(os.path.join('$INSTDIR', rel_file)) + '"\n'

        # at the end of a delete, just rmdir all the directories
        if not install:
            deleted_file_dirs = [os.path.dirname(pair[1].replace(self.get_dst_prefix()+os.path.sep,'')) for pair in self.file_list]
            # find all ancestors so that we don't skip any dirs that happened to have no non-dir children
            deleted_dirs = []
            for d in deleted_file_dirs:
                deleted_dirs.extend(path_ancestors(d))
            # sort deepest hierarchy first
            deleted_dirs.sort(key=lambda a: (-a.count(os.path.sep), a))
            prev = None
            for d in deleted_dirs:
                if d != prev:   # skip duplicates
                    result += 'RMDir ' + wpath(os.path.join('$INSTDIR', os.path.normpath(d))) + '\n'
                prev = d

        return result
	
    def sign_command(self, *argv):
        return [
            "signtool.exe", "sign", "/v",
            "/n", self.args['signature'],
            "/p", os.environ['VIEWER_SIGNING_PWD'],
            "/d","%s" % self.channel(),
            "/t","http://timestamp.comodoca.com/authenticode"
        ] + list(argv)
	
    def sign(self, *argv):
        subprocess.check_call(self.sign_command(*argv))

    def zip_file_name(self):
        return self.installer_base_name() + '.zip'

    def package_zip(self):
        """Portable ZIP containing exactly the files the NSIS installer ships,
        under a single top-level folder. Built before the installer so that a
        release can be published even on machines without NSIS."""
        zip_path = self.dst_path_of(self.zip_file_name())
        top = self.app_name_oneword()
        dst_prefix = self.get_dst_prefix()
        files = sorted(set(pair[1] for pair in self.file_list
                           if pair[0] and os.path.isfile(pair[1])))
        print("Creating portable ZIP %s (%d files)" % (zip_path, len(files)))
        with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED, allowZip64=True) as zf:
            for f in files:
                rel = os.path.relpath(f, dst_prefix)
                zf.write(f, '/'.join([top] + rel.split(os.path.sep)))
        self.created_path(zip_path)
        return zip_path

    def package_finish(self):
        if 'signature' in self.args and 'VIEWER_SIGNING_PWD' in os.environ:
            try:
                self.sign(self.args['dest']+"\\"+self.final_exe())
                self.sign(self.args['dest']+"\\SLPlugin.exe")
                # SLVoice removed (WebRTC)
            except:
                print("Couldn't sign binaries. Tried to sign %s" % self.args['dest'] + "\\" + self.final_exe())
		
        # a standard map of strings for replacing in the templates
        substitution_strings = {
            'version' : '.'.join(self.args['version']),
            'version_short' : '.'.join(self.args['version'][:-1]),
            'version_dashes' : '-'.join(self.args['version']),
            'version_registry' : '%s(%s)' %
            ('.'.join(self.args['version']), self.address_size),
            'final_exe' : self.final_exe(),
            'flags':'',
            'app_name':self.app_name(),
            'app_name_oneword':self.app_name_oneword()
            }

        installer_file = self.installer_base_name() + '_Setup.exe'
        substitution_strings['installer_file'] = installer_file

        # Packaging the installer takes forever, dodge it if we can.
        installer_path = os.path.join(self.args['dest'], installer_file);
        if os.path.isfile(installer_path):
            binary_mod = os.path.getmtime(os.path.join(self.args['dest'], self.final_exe()))
            installer_mod = os.path.getmtime(installer_path)
            if binary_mod <= installer_mod:
                print("Binary is unchanged since last package, touch the binary or delete installer to trigger repackage.")
                exit();

        # Portable ZIP first: it does not depend on NSIS.
        zip_name = self.zip_file_name()
        self.package_zip()

        version_vars = """
        !define INSTEXE  "%(final_exe)s"
        !define VERSION "%(version_short)s"
        !define VERSION_LONG "%(version)s"
        !define VERSION_DASHES "%(version_dashes)s"
        """ % substitution_strings

        if self.channel_type() == 'release':
            substitution_strings['caption'] = CHANNEL_VENDOR_BASE
        else:
            substitution_strings['caption'] = self.app_name() + ' ${VERSION}'

        inst_vars_template = """
            !define INSTEXE  "%(final_exe)s"
            !define INSTOUTFILE "%(installer_file)s"
            !define APPNAME   "%(app_name)s"
            !define APPNAMEONEWORD   "%(app_name_oneword)s"
            !define URLNAME   "secondlife"
            !define CAPTIONSTR "%(caption)s"
            !define VENDORSTR "Allyn Viewer Project"
            !define VERSION "%(version_short)s"
            !define VERSION_LONG "%(version)s"
            !define VERSION_DASHES "%(version_dashes)s"
            """

        tempfile = "%s_setup_tmp.nsi" % self.viewer_branding_id()
        # the following replaces strings in the nsi template
        # it also does python-style % substitution
        self.replace_in("installers/windows/installer_template.nsi", tempfile, {
                "%%VERSION%%":version_vars,
                "%%SOURCE%%":self.get_src_prefix(),
                "%%INST_VARS%%":inst_vars_template % substitution_strings,
                "%%INSTALL_FILES%%":self.nsi_file_commands(True),
                "%%DELETE_FILES%%":self.nsi_file_commands(False),
                "%%WIN64_BIN_BUILD%%":"!define WIN64_BIN_BUILD 1",})

        # We use the Unicode version of NSIS, available from
        # http://www.scratchpaper.com/
        try:
            import winreg as reg
            NSIS_path = reg.QueryValue(reg.HKEY_LOCAL_MACHINE, r"SOFTWARE\NSIS") + '\\makensis.exe'
            self.run_command([proper_windows_path(NSIS_path), self.dst_path_of(tempfile)])
        except Exception:
            try:
                NSIS_path = os.environ.get('ProgramFiles', '') + '\\NSIS\\makensis.exe'
                self.run_command([proper_windows_path(NSIS_path), self.dst_path_of(tempfile)])
            except Exception:
                try:
                    NSIS_path = os.environ.get('ProgramFiles(x86)', '') + '\\NSIS\\makensis.exe'
                    self.run_command([proper_windows_path(NSIS_path),self.dst_path_of(tempfile)])
                except Exception as e:
                    print("WARNING: NSIS installer packaging failed (%s)." % e)
                    print("         Install NSIS (build.bat tools) to get the _Setup.exe; the portable ZIP %s was created." % zip_name)
                    try:
                        self.remove(self.dst_path_of(tempfile))
                    except Exception:
                        pass
                    self.package_file = zip_name
                    return
        self.remove(self.dst_path_of(tempfile))
        if 'signature' in self.args and 'VIEWER_SIGNING_PWD' in os.environ:
            try:
                self.sign(self.args['dest'] + "\\" + substitution_strings['installer_file'])
            except Exception: 
                print("Couldn't sign windows installer. Tried to sign %s" % self.args['dest'] + "\\" + substitution_strings['installer_file'])

        self.created_path(self.dst_path_of(installer_file))
        self.package_file = installer_file


class Windows_x86_64_Manifest(WindowsManifest):
    address_size = 64




################################################################

if __name__ == "__main__":
    extra_arguments = [
        dict(name='bugsplat', description="""BugSplat database to which to post crashes,
             if BugSplat crash reporting is desired""", default=''),
        ]
    main(extra=extra_arguments)
