Name:       %{Name}

%{!?qtc_qmake:%define qtc_qmake %qmake}
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}
Summary:    %{Name}
Version:    0.1
Release:    1
Group:      Qt/Qt
License:    LICENSE
URL:        http://example.org/
Source0:    %{name}-%{version}.tar.bz2
Provides:   %{Name}

%description
Short description of %{Name}


@if !%{StaticSelected}
%package devel
Summary:    %{Name} development headers
Group:      Qt/Qt
Requires:   %{name} = %{version}-%{release}

%description devel
Development headers for %{Name}


@endif
%prep
%setup -q -n %{name}-%{version}

%build

%qtc_qmake5 

%qtc_make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%qmake5_install

@if !%{StaticSelected}
%files
%defattr(-,root,root,-)
%{_libdir}/lib%{Name}.so.*

%files devel
%defattr(-,root,root,-)
%{_libdir}/lib%{Name}.so
%{_libdir}/pkgconfig/%{Name}.pc
%{_includedir}/%{Name}/*
@endif
@if %{StaticSelected}
%files
%defattr(-,root,root,-)
%{_libdir}/lib%{Name}.a
%{_includedir}/%{Name}/*
@endif
