# Qt Creator

Qt Creator is a cross-platform, integrated development environment (IDE) for
application developers to create applications for multiple desktop, embedded,
and mobile device platforms.

The Qt Creator Manual is available at:

https://doc.qt.io/qtcreator/index.html

For an overview of the Qt Creator IDE, see:

https://doc.qt.io/qtcreator/creator-overview.html

## Supported Platforms

The standalone binary packages support the following platforms:

* Windows 7 (64-bit) or later
* (K)Ubuntu Linux 18.04 (64-bit) or later
* macOS 10.13 or later

## Contributing

For instructions on how to set up the Qt Creator repository to contribute
patches back to Qt Creator, please check:

https://wiki.qt.io/Setting_up_Gerrit

See the following page for information about our coding standard:

https://doc.qt.io/qtcreator-extending/coding-style.html

## Compiling Qt Creator

Prerequisites:

* Qt 5.14.0 or later
* Qt WebEngine module for QtWebEngine based help viewer
* On Windows:
    * MinGW with GCC 7 or Visual Studio 2017 or later
    * Python 3.5 or later (optional, needed for the python enabled debug helper)
    * Debugging Tools for Windows (optional, for MSVC debugging support with CDB)
* On Mac OS X: latest Xcode
* On Linux: GCC 7 or later
* LLVM/Clang 10 or later (optional, LLVM/Clang 11 is recommended.
  See [instructions](#getting-llvmclang-for-the-clang-code-model) on how to
  get LLVM.
  The ClangFormat, ClangPchManager and ClangRefactoring use the LLVM C++ API.
  Since the LLVM C++ API provides no compatibility guarantee,
  if later versions don't compile we don't support that version.)
* CMake
* Ninja (recommended)

The installed toolchains have to match the one Qt was compiled with.

### Linux and macOS

These instructions assume that Ninja is installed and in the `PATH`, Qt Creator
sources are located at `/path/to/qtcreator_sources`, Qt is installed in
`/path/to/Qt`, and LLVM is installed in `/path/to/llvm`.

Note that if you install Qt via the online installer, the path to Qt must
include the version number and compiler ABI. The path to the online installer
content is not enough.

See [instructions](#getting-llvmclang-for-the-clang-code-model) on how to
get LLVM.

    mkdir qtcreator_build
    cd qtcreator_build

    cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja "-DCMAKE_PREFIX_PATH=/path/to/Qt;/path/to/llvm" /path/to/qtcreator_sources
    cmake --build .

### Windows

These instructions assume that Ninja is installed and in the `PATH`, Qt Creator
sources are located at `\path\to\qtcreator_sources`, Qt is installed in
`\path\to\Qt`, and LLVM is installed in `\path\to\llvm`.

Note that if you install Qt via the online installer, the path to Qt must
include the version number and compiler ABI. The path to the online installer
content is not enough.

See [instructions](#getting-llvmclang-for-the-clang-code-model) on how to
get LLVM.

Decide which compiler to use: MinGW or Microsoft Visual Studio.

MinGW is available via the Qt online installer, for other options see
<https://wiki.qt.io/MinGW>. Run the commands below in a shell prompt that has
`<path_to_mingw>\bin` in the `PATH`.

For Microsoft Visual C++ you can use the "Build Tools for Visual Studio". Also
install the "Debugging Tools for Windows" from the Windows SDK installer. We
strongly recommend using the 64-bit version and 64-bit compilers on 64-bit
systems. Open the `x64 Native Tools Command Prompt for VS <version>` from the
start menu items that were created for Visual Studio, and run the commands
below in it.

    md qtcreator_build
    cd qtcreator_build

    cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja "-DCMAKE_PREFIX_PATH=/path/to/Qt;/path/to/llvm" \path\to\qtcreator_sources
    cmake --build .

Qt Creator can be registered as a post-mortem debugger. This can be done in the
options page or by running the tool qtcdebugger with administrative privileges
passing the command line options -register/unregister, respectively.
Alternatively, the required registry entries

    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AeDebug
    HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows NT\CurrentVersion\AeDebug

can be modified using the registry editor regedt32 to contain

    qtcreator_build\bin\qtcdebugger %ld %ld

When using a self-built version of Qt Creator as post-mortem debugger, it needs
to be able to find all dependent Qt-libraries and plugins when being launched
by the system. The easiest way to do this is to create a self-contained Qt
Creator by installing it and installing its dependencies. See "Options" below
for details.

Note that unlike on Unix, you cannot overwrite executables that are running.
Thus, if you want to work on Qt Creator using Qt Creator, you need a separate
installation of it. We recommend using a separate, release-built version of Qt
Creator to work on a debug-built version of Qt Creator.

### Options

If you do not have Ninja installed and in the `PATH`, remove `-G Ninja` from
the first `cmake` call. If you want to build in release mode, change the build
type to `-DCMAKE_BUILD_TYPE=Release`. You can also build with release
optimizations but debug information with `-DCMAKE_BUILD_TYPE=RelWithDebInfo`.

Installation is not needed. It is however possible, using

    cmake --install . --prefix /path/to/qtcreator_install

To create a self-contained Qt Creator installation, including all dependencies
like Qt and LLVM, additionally run

    cmake --install . --prefix /path/to/qtcreator_install --component Dependencies

## Getting LLVM/Clang for the Clang Code Model

The Clang Code Model depends on the LLVM/Clang libraries. The currently
supported LLVM/Clang version is 8.0.

### Prebuilt LLVM/Clang packages

Prebuilt packages of LLVM/Clang can be downloaded from
https://download.qt.io/development_releases/prebuilt/libclang/

This should be your preferred option because you will use the version that is
shipped together with Qt Creator (with backported/additional patches). In
addition, MinGW packages for Windows are faster due to profile-guided
optimization. If the prebuilt packages do not match your configuration, you
need to build LLVM/Clang manually.

If you use the MSVC compiler to build Qt Creator the suggested way is:
    1. Download both MSVC and MinGW packages of libclang.
    2. Use the MSVC version of libclang during the Qt Creator build.
    3. Prepend PATH variable used for the run time with the location of MinGW version of libclang.dll.
    4. Launch Qt Creator.

### Building LLVM/Clang manually

You need to install CMake in order to build LLVM/Clang.

Build LLVM/Clang by roughly following the instructions at
http://llvm.org/docs/GettingStarted.html#git-mirror:

   1. Clone LLVM/Clang and checkout a suitable branch

          git clone -b release_110-based --recursive https://code.qt.io/clang/llvm-project.git

   2. Build and install LLVM/Clang

          mkdir build
          cd build

      For Linux/macOS:

          cmake \
            -D CMAKE_BUILD_TYPE=Release \
            -D LLVM_ENABLE_RTTI=ON \
            -D LLVM_ENABLE_PROJECTS="clang;clang-tools-extra" \
            -D CMAKE_INSTALL_PREFIX=<installation location> \
            ../llvm-project/llvm
          cmake --build . --target install

      For Windows:

          cmake ^
            -G Ninja ^
            -D CMAKE_BUILD_TYPE=Release ^
            -D LLVM_ENABLE_RTTI=ON ^
            -D LLVM_ENABLE_PROJECTS="clang;clang-tools-extra" ^
            -D CMAKE_INSTALL_PREFIX=<installation location> ^
            ..\llvm-project\llvm
          cmake --build . --target install

### Clang-Format

The ClangFormat plugin depends on the additional patch

    https://code.qt.io/cgit/clang/llvm-project.git/commit/?h=release_100-based&id=9b992a0f7f160dd6c75f20a4dcfcf7c60a4894df

While the plugin builds without it, it might not be fully functional.

Note that the plugin is disabled by default.

## Third-party Components

Qt Creator includes the following third-party components,
we thank the authors who made this possible:

### YAML Parser yaml-cpp (MIT License)

  https://github.com/jbeder/yaml-cpp

  QtCreator/src/libs/3rdparty/yaml-cpp

  Copyright (c) 2008-2015 Jesse Beder.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

### KSyntaxHighlighting

  Syntax highlighting engine for Kate syntax definitions

  This is a stand-alone implementation of the Kate syntax highlighting
  engine. It's meant as a building block for text editors as well as
  for simple highlighted text rendering (e.g. as HTML), supporting both
  integration with a custom editor as well as a ready-to-use
  QSyntaxHighlighter sub-class.

  Distributed under the:

  MIT License

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  The source code of KSyntaxHighlighting can be found here:
      https://cgit.kde.org/syntax-highlighting.git
      QtCreator/src/libs/3rdparty/syntax-highlighting
      https://code.qt.io/cgit/qt-creator/qt-creator.git/tree/src/libs/3rdparty/syntax-highlighting

### Clazy

  https://github.com/KDE/clazy

  Copyright (C) 2015-2018 Clazy Team

  Distributed under GNU LIBRARY GENERAL PUBLIC LICENSE Version 2 (LGPL2).

  Integrated with patches from
  http://code.qt.io/cgit/clang/clang-tools-extra.git/.

### LLVM/Clang

  https://github.com/llvm/llvm-project.git

  Copyright (C) 2003-2019 LLVM Team

  Distributed under the University of Illinois/NCSA Open Source License (NCSA),
  see https://github.com/llvm/llvm-project/blob/master/llvm/LICENSE.TXT

  With backported/additional patches from https://code.qt.io/clang/llvm-project.git

### Reference implementation for std::experimental::optional

  https://github.com/akrzemi1/Optional

  QtCreator/src/libs/3rdparty/optional

  Copyright (C) 2011-2012 Andrzej Krzemienski

  Distributed under the Boost Software License, Version 1.0
  (see accompanying file LICENSE_1_0.txt or a copy at
  http://www.boost.org/LICENSE_1_0.txt)

  The idea and interface is based on Boost.Optional library
  authored by Fernando Luis Cacciola Carballal

### Implementation for std::variant

  https://github.com/mpark/variant

  QtCreator/src/libs/3rdparty/variant

  Copyright Michael Park, 2015-2017

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

### Implementation for std::span

  https://github.com/tcbrindle/span

  QtCreator/src/libs/3rdparty/span

  Copyright Tristan Brindle, 2018

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

### Open Source front-end for C++ (license MIT), enhanced for use in Qt Creator

  Roberto Raggi <roberto.raggi@gmail.com>

  QtCreator/src/shared/cplusplus

  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### Open Source tool for generating C++ code that classifies keywords (license MIT)

  Roberto Raggi <roberto.raggi@gmail.com>

  QtCreator/src/tools/3rdparty/cplusplus-keywordgen

  Copyright (c) 2007 Roberto Raggi <roberto.raggi@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
  the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### SQLite, in-process library that implements a SQL database engine

SQLite (https://www.sqlite.org) is in the Public Domain.

### ClassView and ImageViewer plugins

  Copyright (C) 2016 The Qt Company Ltd.

  All rights reserved.
  Copyright (C) 2016 Denis Mingulov.

  Contact: http://www.qt.io

  This file is part of Qt Creator.

  You may use this file under the terms of the BSD license as follows:

  "Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of The Qt Company Ltd and its Subsidiary(-ies) nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."

### Source Code Pro font

  Copyright 2010, 2012 Adobe Systems Incorporated (http://www.adobe.com/),
  with Reserved Font Name 'Source'. All Rights Reserved. Source is a
  trademark of Adobe Systems Incorporated in the United States
  and/or other countries.

  This Font Software is licensed under the SIL Open Font License, Version 1.1.

  The font and license files can be found in QtCreator/src/libs/3rdparty/fonts.

### JSON Library by Niels Lohmann

  Used by the Chrome Trace Format Visualizer plugin instead of QJson
  because of QJson's current hard limit of 128 Mb object size and
  trace files often being much larger.

  The sources can be found in `QtCreator/src/libs/3rdparty/json`.

  The class is licensed under the MIT License:

  Copyright © 2013-2019 Niels Lohmann

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the “Software”), to
  deal in the Software without restriction, including without limitation the
  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is furnished
  to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  The class contains the UTF-8 Decoder from Bjoern Hoehrmann which is
  licensed under the MIT License (see above). Copyright © 2008-2009 Björn
  Hoehrmann bjoern@hoehrmann.de

  The class contains a slightly modified version of the Grisu2 algorithm
  from Florian Loitsch which is licensed under the MIT License (see above).
  Copyright © 2009 Florian Loitsch

### litehtml

  The litehtml HTML/CSS rendering engine is used as a help viewer backend
  to display help files.

  The sources can be found in:
    * QtCreator/src/plugins/help/qlitehtml
    * https://github.com/litehtml

  Copyright (c) 2013, Yuri Kobets (tordex)

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
   * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   * Neither the name of the <organization> nor the
   names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### gumbo

  The litehtml HTML/CSS rendering engine uses the gumbo parser.

  Copyright 2010, 2011 Google

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

### gumbo/utf8.c

  The litehtml HTML/CSS rendering engine uses gumbo/utf8.c parser.

  Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do  so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

### SourceCodePro fonts

  Qt Creator ships with the following fonts licensed under OFL-1.1:

   * SourceCodePro-Regular.ttf
   * SourceCodePro-It.ttf
   * SourceCodePro-Bold.ttf

  SIL OPEN FONT LICENSE

  Version 1.1 - 26 February 2007

  PREAMBLE
  The goals of the Open Font License (OFL) are to stimulate worldwide
  development of collaborative font projects, to support the font creation
  efforts of academic and linguistic communities, and to provide a free and
  open framework in which fonts may be shared and improved in partnership
  with others.

  The OFL allows the licensed fonts to be used, studied, modified and
  redistributed freely as long as they are not sold by themselves. The
  fonts, including any derivative works, can be bundled, embedded,
  redistributed and/or sold with any software provided that any reserved
  names are not used by derivative works. The fonts and derivatives,
  however, cannot be released under any other type of license. The
  requirement for fonts to remain under this license does not apply
  to any document created using the fonts or their derivatives.

  DEFINITIONS
  "Font Software" refers to the set of files released by the Copyright
  Holder(s) under this license and clearly marked as such. This may
  include source files, build scripts and documentation.

  "Reserved Font Name" refers to any names specified as such after the
  copyright statement(s).

  "Original Version" refers to the collection of Font Software components as
  distributed by the Copyright Holder(s).

  "Modified Version" refers to any derivative made by adding to, deleting,
  or substituting - in part or in whole - any of the components of the
  Original Version, by changing formats or by porting the Font Software to a
  new environment.

  "Author" refers to any designer, engineer, programmer, technical
  writer or other person who contributed to the Font Software.

  PERMISSION & CONDITIONS
  Permission is hereby granted, free of charge, to any person obtaining
  a copy of the Font Software, to use, study, copy, merge, embed, modify,
  redistribute, and sell modified and unmodified copies of the Font
  Software, subject to the following conditions:

  1) Neither the Font Software nor any of its individual components,
  in Original or Modified Versions, may be sold by itself.

  2) Original or Modified Versions of the Font Software may be bundled,
  redistributed and/or sold with any software, provided that each copy
  contains the above copyright notice and this license. These can be
  included either as stand-alone text files, human-readable headers or
  in the appropriate machine-readable metadata fields within text or
  binary files as long as those fields can be easily viewed by the user.

  3) No Modified Version of the Font Software may use the Reserved Font
  Name(s) unless explicit written permission is granted by the corresponding
  Copyright Holder. This restriction only applies to the primary font name as
  presented to the users.

  4) The name(s) of the Copyright Holder(s) or the Author(s) of the Font
  Software shall not be used to promote, endorse or advertise any
  Modified Version, except to acknowledge the contribution(s) of the
  Copyright Holder(s) and the Author(s) or with their explicit written
  permission.

  5) The Font Software, modified or unmodified, in part or in whole,
  must be distributed entirely under this license, and must not be
  distributed under any other license. The requirement for fonts to
  remain under this license does not apply to any document created
  using the Font Software.

  TERMINATION
  This license becomes null and void if any of the above conditions are
  not met.

  DISCLAIMER
  THE FONT SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
  OF COPYRIGHT, PATENT, TRADEMARK, OR OTHER RIGHT. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  INCLUDING ANY GENERAL, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL
  DAMAGES, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF THE USE OR INABILITY TO USE THE FONT SOFTWARE OR FROM
  OTHER DEALINGS IN THE FONT SOFTWARE.

  ### Qbs

  Qt Creator installations deliver Qbs. Its licensing and third party
  attributions are listed in Qbs Manual at
  https://doc.qt.io/qbs/attributions.html

  ### conan.cmake

  CMake script used by Qt Creator's auto setup of package manager dependencies.

  The sources can be found in:
    * QtCreator/src/share/3rdparty/package-manager/conan.cmake
    * https://github.com/conan-io/cmake-conan

  The MIT License (MIT)

  Copyright (c) 2018 JFrog

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
