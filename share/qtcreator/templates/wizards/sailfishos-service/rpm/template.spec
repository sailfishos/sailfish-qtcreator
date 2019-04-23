Name:       %{ProjectName}

%{!?qtc_qmake:%define qtc_qmake %qmake}
%{!?qtc_qmake5:%define qtc_qmake5 %qmake5}
%{!?qtc_make:%define qtc_make make}
%{?qtc_builddir:%define _builddir %qtc_builddir}
Summary:    %{ShortDescription}
Version:    0.1
Release:    1
Group:      Qt/Qt
License:    LICENSE
URL:        http://example.org/
Source0:    %{name}-%{version}.tar.bz2
Requires(post): /bin/touch

%description
%{Description}


%prep
%setup -q -n %{name}-%{version}

%build
%qtc_qmake5 
%qtc_make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%qmake5_install

%post
systemctl-user daemon-reload
systemctl-user enable %{ServiceName}.service
systemctl-user restart %{ServiceName}.service

%preun
systemctl-user stop %{ServiceName}.service
systemctl-user disable %{ServiceName}.service

%postun
systemctl-user daemon-reload


%files
%defattr(-,root,root,-)
%{_bindir}
%{_libdir}/systemd/user/%{ServiceName}.service
@if '%{ServiceType}' === 'dbus'
%{_datadir}/dbus-1/services/%{NamePrefix}.%{CompanyName}.%{ServiceName}.service
@endif
