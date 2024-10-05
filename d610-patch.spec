Name:           d610-patch 
Version:        0.1
Release:        1
Epoch:          1
Summary:        Makes it possible to set unsupported vidio resolutions with intel chipset.
Packager:	Casper Pedersen <casperghst42[at]fastmail.fm>

Group:          Applications/System
License:        Public Domain
URL:            http://www.el-streb.com
Source0:	http://download.el-streb.com/d610-patch.c
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  gcc
Requires:       xorg-x11

%description
Bios patch to make it possible to get 1400x1050 on Dell Latitude D610 with the Intel i915 chipset. The driver from Intel is not nessarry. Get the source from http://download.el-streb.com/d610-patch.c

%prep

%build
# Ugly hack to get around that we do not have a tar.gz
rm -rf $RPM_BUILD_DIR/%{name}
mkdir $RPM_BUILD_DIR/%{name}
cp %{SOURCE0} $RPM_BUILD_DIR/%{name}
cd $RPM_BUILD_DIR/%{name}
gcc -o %{name} %{SOURCE0}


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_sbindir}
mkdir -p $RPM_BUILD_ROOT%{_defaultdocdir}/%{name}

cp -ar $RPM_BUILD_DIR/%{name}/%{name} $RPM_BUILD_ROOT%{_sbindir}

cat << EOF > $RPM_BUILD_ROOT/%{_defaultdocdir}/%{name}/README_dell_d610.txt
To get 1400x1050 on a Dell Latitude D610, add the following to /etc/init.d/boot.local:
%{_sbindir}/%{name} 3c 1400 1050
EOF

%clean
rm -rf $RPM_BUILD_ROOT

%post
cat %{_defaultdocdir}/%{name}/README_dell_d610.txt

%preun

%postun

%files 
%defattr(-,root,root,-)
%{_sbindir}/*
%{_defaultdocdir}/%{name}/*

%changelog
* Thu Mar 31 2005 Casper Pedersen <casperghst42[at]fastmail.fm> - 1:0.1-1
- Initial RPM release.
