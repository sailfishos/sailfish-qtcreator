#!/usr/bin/env python
#############################################################################
##
## Copyright (C) 2020 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the release tools of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

# import the print function which is used in python 3.x
from __future__ import print_function

import argparse
import collections
import glob
import os

import common

def existing_path(path):
    return path if os.path.exists(path) else None

def default_python3():
    path_system = os.path.join('/usr', 'bin') if not common.is_windows_platform() else None
    path = os.environ.get('PYTHON3_PATH') or path_system
    postfix = '.exe' if common.is_windows_platform() else ''
    return (path if not path
            else (existing_path(os.path.join(path, 'python3' + postfix)) or
                  existing_path(os.path.join(path, 'python' + postfix))))

def get_arguments():
    parser = argparse.ArgumentParser(description='Build Qt Creator for packaging')
    parser.add_argument('--src', help='path to sources', required=True)
    parser.add_argument('--build', help='path that should be used for building', required=True)
    parser.add_argument('--qt-path', help='Path to Qt')

    parser.add_argument('--build-type', help='Build type to pass to CMake (defaults to RelWithDebInfo)',
                        default='RelWithDebInfo')

    # clang codemodel
    parser.add_argument('--llvm-path', help='Path to LLVM installation for Clang code model',
                        default=os.environ.get('LLVM_INSTALL_DIR'))

    # perfparser
    parser.add_argument('--elfutils-path',
                        help='Path to elfutils installation for use by perfprofiler (Windows, Linux)')

    # signing
    parser.add_argument('--keychain-unlock-script',
                        help='Path to script for unlocking the keychain used for signing (macOS)')

    # cdbextension
    parser.add_argument('--python-path',
                        help='Path to python libraries for use by cdbextension (Windows)')

    parser.add_argument('--app-target', help='File name of the executable / app bundle',
                        default=('Qt Creator.app' if common.is_mac_platform()
                                 else 'qtcreator'))
    parser.add_argument('--python3', help='File path to python3 executable for generating translations',
                        default=default_python3())

    parser.add_argument('--no-qtcreator',
                        help='Skip Qt Creator build (only build separate tools)',
                        action='store_true', default=False)
    parser.add_argument('--no-cdb',
                        help='Skip cdbextension and the python dependency packaging step (Windows)',
                        action='store_true', default=(not common.is_windows_platform()))
    parser.add_argument('--no-docs', help='Skip documentation generation',
                        action='store_true', default=False)
    parser.add_argument('--no-build-date', help='Does not show build date in about dialog, for reproducible builds',
                        action='store_true', default=False)
    parser.add_argument('--no-dmg', help='Skip disk image creation (macOS)',
                        action='store_true', default=False)
    parser.add_argument('--no-zip', help='Skip creation of 7zip files for install and developer package',
                        action='store_true', default=False)
    parser.add_argument('--with-tests', help='Enable building of tests',
                        action='store_true', default=False)
    parser.add_argument('--with-pch', help='Enable building with PCH',
                        action='store_true', default=False)
    parser.add_argument('--add-path', help='Prepends a CMAKE_PREFIX_PATH to the build',
                        action='append', dest='prefix_paths', default=[])
    parser.add_argument('--add-module-path', help='Prepends a CMAKE_MODULE_PATH to the build',
                        action='append', dest='module_paths', default=[])
    parser.add_argument('--add-make-arg', help='Passes the argument to the make tool.',
                        action='append', dest='make_args', default=[])
    parser.add_argument('--add-config', help=('Adds the argument to the CMake configuration call. '
                                              'Use "--add-config=-DSOMEVAR=SOMEVALUE" if the argument begins with a dash.'),
                        action='append', dest='config_args', default=[])
    parser.add_argument('--zip-infix', help='Adds an infix to generated zip files, use e.g. for a build number.',
                        default='')
    parser.add_argument('--zip-threads', help='Sets number of threads to use for 7z. Use "+" for turning threads on '
                        'without a specific number of threads. This is directly passed to the "-mmt" option of 7z.',
                        default='2')
    args = parser.parse_args()
    args.with_debug_info = args.build_type == 'RelWithDebInfo'

    if not args.qt_path and not args.no_qtcreator:
        parser.error("argument --qt-path is required if --no-qtcreator is not given")
    return args

def common_cmake_arguments(args):
    separate_debug_info_option = 'ON' if args.with_debug_info else 'OFF'
    cmake_args = ['-DCMAKE_BUILD_TYPE=' + args.build_type,
                  '-DQTC_SEPARATE_DEBUG_INFO=' + separate_debug_info_option,
                  '-G', 'Ninja']

    if args.python3:
        cmake_args += ['-DPYTHON_EXECUTABLE=' + args.python3]
        cmake_args += ['-DPython3_EXECUTABLE=' + args.python3]

    if args.module_paths:
        module_paths = [common.to_posix_path(os.path.abspath(fp)) for fp in args.module_paths]
        cmake_args += ['-DCMAKE_MODULE_PATH=' + ';'.join(module_paths)]

    # force MSVC on Windows, because it looks for GCC in the PATH first,
    # even if MSVC is first mentioned in the PATH...
    # TODO would be nicer if we only did this if cl.exe is indeed first in the PATH
    if common.is_windows_platform():
        if not os.environ.get('CC') and not os.environ.get('CXX'):
            cmake_args += ['-DCMAKE_C_COMPILER=cl',
                           '-DCMAKE_CXX_COMPILER=cl']
        if args.python_path:
            python_library = glob.glob(os.path.join(args.python_path, 'libs', 'python??.lib'))
            if python_library:
                cmake_args += ['-DPYTHON_LIBRARY=' + python_library[0],
                               '-DPYTHON_INCLUDE_DIR=' + os.path.join(args.python_path, 'include')]

    pch_option = 'ON' if args.with_pch else 'OFF'
    cmake_args += ['-DBUILD_WITH_PCH=' + pch_option]

    return cmake_args

def build_qtcreator(args, paths):
    if args.no_qtcreator:
        return
    if not os.path.exists(paths.build):
        os.makedirs(paths.build)
    prefix_paths = [os.path.abspath(fp) for fp in args.prefix_paths] + [paths.qt]
    if paths.llvm:
        prefix_paths += [paths.llvm]
    if paths.elfutils:
        prefix_paths += [paths.elfutils]
    prefix_paths = [common.to_posix_path(fp) for fp in prefix_paths]
    with_docs_str = 'OFF' if args.no_docs else 'ON'
    build_date_option = 'OFF' if args.no_build_date else 'ON'
    test_option = 'ON' if args.with_tests else 'OFF'
    cmake_args = ['cmake',
                  '-DCMAKE_PREFIX_PATH=' + ';'.join(prefix_paths),
                  '-DSHOW_BUILD_DATE=' + build_date_option,
                  '-DWITH_DOCS=' + with_docs_str,
                  '-DBUILD_DEVELOPER_DOCS=' + with_docs_str,
                  '-DBUILD_EXECUTABLE_SDKTOOL=OFF',
                  '-DCMAKE_INSTALL_PREFIX=' + common.to_posix_path(paths.install),
                  '-DWITH_TESTS=' + test_option]
    cmake_args += common_cmake_arguments(args)

    if common.is_windows_platform():
        cmake_args += ['-DBUILD_EXECUTABLE_WIN32INTERRUPT=OFF',
                       '-DBUILD_EXECUTABLE_WIN64INTERRUPT=OFF',
                       '-DBUILD_LIBRARY_QTCREATORCDBEXT=OFF']

    ide_revision = common.get_commit_SHA(paths.src)
    if ide_revision:
        cmake_args += ['-DIDE_REVISION=ON',
                       '-DIDE_REVISION_STR=' + ide_revision,
                       '-DIDE_REVISION_URL=https://code.qt.io/cgit/qt-creator/qt-creator.git/log/?id=' + ide_revision]

    cmake_args += args.config_args

    common.check_print_call(cmake_args + [paths.src], paths.build)
    build_args = ['cmake', '--build', '.']
    if args.make_args:
        build_args += ['--'] + args.make_args
    common.check_print_call(build_args, paths.build)
    if not args.no_docs:
        common.check_print_call(['cmake', '--build', '.', '--target', 'docs'], paths.build)

    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install, '--strip'],
                            paths.build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                             '--component', 'Dependencies'],
                            paths.build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.dev_install,
                             '--component', 'Devel'],
                            paths.build)
    if args.with_debug_info:
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.debug_install,
                                 '--component', 'DebugInfo'],
                                 paths.build)
    if not args.no_docs:
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                                 '--component', 'qch_docs'],
                                paths.build)
        common.check_print_call(['cmake', '--install', '.', '--prefix', paths.install,
                                 '--component', 'html_docs'],
                                paths.build)

def build_wininterrupt(args, paths):
    if not common.is_windows_platform():
        return
    if not os.path.exists(paths.wininterrupt_build):
        os.makedirs(paths.wininterrupt_build)
    prefix_paths = [common.to_posix_path(os.path.abspath(fp)) for fp in args.prefix_paths]
    cmake_args = ['-DCMAKE_PREFIX_PATH=' + ';'.join(prefix_paths),
                  '-DCMAKE_INSTALL_PREFIX=' + common.to_posix_path(paths.wininterrupt_install)]
    cmake_args += common_cmake_arguments(args)
    common.check_print_call(['cmake'] + cmake_args + [os.path.join(paths.src, 'src', 'tools', 'wininterrupt')],
                            paths.wininterrupt_build)
    common.check_print_call(['cmake', '--build', '.'], paths.wininterrupt_build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.wininterrupt_install,
                             '--component', 'wininterrupt'],
                            paths.wininterrupt_build)

def build_qtcreatorcdbext(args, paths):
    if args.no_cdb:
        return
    if not os.path.exists(paths.qtcreatorcdbext_build):
        os.makedirs(paths.qtcreatorcdbext_build)
    prefix_paths = [common.to_posix_path(os.path.abspath(fp)) for fp in args.prefix_paths]
    cmake_args = ['-DCMAKE_PREFIX_PATH=' + ';'.join(prefix_paths),
                  '-DCMAKE_INSTALL_PREFIX=' + common.to_posix_path(paths.qtcreatorcdbext_install)]
    cmake_args += common_cmake_arguments(args)
    common.check_print_call(['cmake'] + cmake_args + [os.path.join(paths.src, 'src', 'libs', 'qtcreatorcdbext')],
                            paths.qtcreatorcdbext_build)
    common.check_print_call(['cmake', '--build', '.'], paths.qtcreatorcdbext_build)
    common.check_print_call(['cmake', '--install', '.', '--prefix', paths.qtcreatorcdbext_install,
                             '--component', 'qtcreatorcdbext'],
                            paths.qtcreatorcdbext_build)

def package_qtcreator(args, paths):
    if not args.no_zip:
        if not args.no_qtcreator:
            common.check_print_call(['7z', 'a', '-mmt' + args.zip_threads,
                                     os.path.join(paths.result, 'qtcreator' + args.zip_infix + '.7z'),
                                     '*'],
                                    paths.install)
            common.check_print_call(['7z', 'a', '-mmt' + args.zip_threads,
                                     os.path.join(paths.result, 'qtcreator' + args.zip_infix + '_dev.7z'),
                                     '*'],
                                    paths.dev_install)
            if args.with_debug_info:
                common.check_print_call(['7z', 'a', '-mmt' + args.zip_threads,
                                         os.path.join(paths.result, 'qtcreator' + args.zip_infix + '-debug.7z'),
                                         '*'],
                                        paths.debug_install)
        if common.is_windows_platform():
            common.check_print_call(['7z', 'a', '-mmt' + args.zip_threads,
                                     os.path.join(paths.result, 'wininterrupt' + args.zip_infix + '.7z'),
                                     '*'],
                                    paths.wininterrupt_install)
            if not args.no_cdb:
                common.check_print_call(['7z', 'a', '-mmt' + args.zip_threads,
                                         os.path.join(paths.result, 'qtcreatorcdbext' + args.zip_infix + '.7z'),
                                         '*'],
                                        paths.qtcreatorcdbext_install)

    if common.is_mac_platform() and not args.no_dmg and not args.no_qtcreator:
        if args.keychain_unlock_script:
            common.check_print_call([args.keychain_unlock_script], paths.install)
        common.check_print_call(['python', '-u',
                                 os.path.join(paths.src, 'scripts', 'makedmg.py'),
                                 'qt-creator' + args.zip_infix + '.dmg',
                                 'Qt Creator',
                                 paths.src,
                                 paths.install],
                                paths.result)

def get_paths(args):
    Paths = collections.namedtuple('Paths',
                                   ['qt', 'src', 'build', 'wininterrupt_build', 'qtcreatorcdbext_build',
                                    'install', 'dev_install', 'debug_install',
                                    'wininterrupt_install', 'qtcreatorcdbext_install', 'result',
                                    'elfutils', 'llvm'])
    build_path = os.path.abspath(args.build)
    install_path = os.path.join(build_path, 'install')
    qt_path = os.path.abspath(args.qt_path) if args.qt_path else None
    return Paths(qt=qt_path,
                 src=os.path.abspath(args.src),
                 build=os.path.join(build_path, 'build'),
                 wininterrupt_build=os.path.join(build_path, 'build-wininterrupt'),
                 qtcreatorcdbext_build=os.path.join(build_path, 'build-qtcreatorcdbext'),
                 install=os.path.join(install_path, 'qt-creator'),
                 dev_install=os.path.join(install_path, 'qt-creator-dev'),
                 debug_install=os.path.join(install_path, 'qt-creator-debug'),
                 wininterrupt_install=os.path.join(install_path, 'wininterrupt'),
                 qtcreatorcdbext_install=os.path.join(install_path, 'qtcreatorcdbext'),
                 result=build_path,
                 elfutils=os.path.abspath(args.elfutils_path) if args.elfutils_path else None,
                 llvm=os.path.abspath(args.llvm_path) if args.llvm_path else None)

def main():
    args = get_arguments()
    paths = get_paths(args)

    build_qtcreator(args, paths)
    build_wininterrupt(args, paths)
    build_qtcreatorcdbext(args, paths)
    package_qtcreator(args, paths)

if __name__ == '__main__':
    main()
