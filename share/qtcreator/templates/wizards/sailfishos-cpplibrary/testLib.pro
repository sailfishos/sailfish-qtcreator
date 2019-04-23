TARGET = %{Name}

TEMPLATE = lib
%{JS: %{StaticSelected} ? "\nCONFIG += staticlib\n" : ''}
DEFINES += %{UpperClassName}_LIBRARY

SOURCES += \\
        src/%{SrcFile}

HEADERS += \\
        src/%{HdrFile} \\
        %{JS: %{StaticSelected} ? '' : ''.concat('src/%{ClassName}'.toLowerCase(), '_global.h\n')}

headers.files = $${HEADERS}
headers.path = /usr/include/$${TARGET}

library.path = /usr/lib
@if !%{StaticSelected}
library.files = $${OUT_PWD}/lib$${TARGET}.so*

PKGCONFIGFILES.path = /usr/lib/pkgconfig
PKGCONFIGFILES.files = $${TARGET}.pc

INSTALLS += headers library PKGCONFIGFILES
@endif
@if %{StaticSelected}
library.files = $${OUT_PWD}/lib$${TARGET}.a

INSTALLS += headers library
@endif

OTHER_FILES += rpm/$${TARGET}.spec
