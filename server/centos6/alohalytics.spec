%define	pkg_version 20160425.1430
%define checkout_date %(date -d "`echo %{pkg_version} | sed -e 's#\\.# #'`" +"%Y-%m-%d %H:%M")

Name:           alohalytics
Version:        %{pkg_version}
Release:        1
Summary:        alohalytics
Group:          Applications/Counters
License:        MIT
URL:            https://github.com/mapsme/Alohalytics/blob/master/server
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}
BuildRequires:  rpm-build
Source0:        %{name}-%{version}.tar.gz

%package server
Summary: alohalytics server
BuildRequires: fcgi-devel
BuildRequires: spawn-fcgi
BuildRequires: zlib-devel
BuildRequires: devtoolset-2-gcc-c++
Requires: spawn-fcgi, fcgi, fcgi-devel

%description
alohalytics main package

%description server
alohalytics server

%prep
%{__rm} -rf %{_builddir}/%{name}-%{version}
if [ -e %{S:0} ]; then
        %{__tar} xzf %{S:0}
        %{__chmod} -Rf a+rX,u+w,g-w,o-w %{_builddir}/%{name}-%{version}
else
        git clone https://github.com/mapsme/Alohalytics.git %{_builddir}/%{name}-%{version}
        cd %{_builddir}/%{name}-%{version}
        git checkout  `git rev-list -n 1 --before="%{version_date}" master`
        # pack source to save it in src rpm
        cd ..
        %{__tar} czf %{S:0} %{name}-%{version}
fi
%setup -T -D

%build
cd server
. /opt/rh/devtoolset-2/enable
make

%install
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}
%{__mkdir_p} %{buildroot}/usr/local/bin
%{__install} -p -d -m 0755 %{buildroot}/usr/local/bin
%{__install} -p -d -m 0755 %{buildroot}/etc/init.d
%{__install} -p -m 0644 %{_builddir}/%{name}-%{version}/server/build/fcgi_server %{buildroot}/usr/local/bin/%{name}-server
%{__install} -p -m 0644 %{_builddir}/%{name}-%{version}/server/centos6/alohalytics-server.init %{buildroot}/etc/init.d/%{name}-server

%clean
rm -rf %{buildroot}

%files server
%defattr(-,root,root,-)
%attr(755,root,root) /usr/local/bin/%{name}-server
%attr(755,root,root) /etc/init.d/%{name}-server

%changelog
* Mon Apr 25 2016 Alexander Zolotarev <me@alex.bio> - 20160425.1424
* Tue Apr 19 2016 Alexander Zolotarev <me@alex.bio> - 20160419.1646
* Fri Apr 15 2016 Aleksey Antropov <aleksey.antropov@corp.mail.ru> - 20160415.1031-1
- Initial
