Name: libsiilihai
Version: 1.3.0
Release: 1
Summary: Siilihai web forum reader library
License: GPL
Group: System/Libraries
Url: http://siilihai.com/
Source: %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: qt-devel
BuildRequires: pkgconfig(QtCore) >= 4.6.0
BuildRequires: pkgconfig(QtNetwork)
BuildRequires: pkgconfig(QtXml)

%description
Siilihai web forum reader library

%package devel
Summary:    Siilihai web forum reader development files
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains the files necessary to develop applications
that use libsiilihai library.

%prep
%setup -q

%build
%qmake
make %{?jobs:-j%jobs}

%install
%{__rm} -rf %{buildroot}
%qmake_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libsiilihai.so.*

%files devel
%defattr(-,root,root,-)
%{_libdir}/libsiilihai.so
%{_libdir}/libsiilihai.prl
%{_includedir}/siilihai

