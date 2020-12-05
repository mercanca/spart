%global slurm_version  %(rpm -q slurm-devel --qf "%{VERSION}" 2>/dev/null)

# run "spectool -g -R spart.spec" to automatically download source files
# spectool is part of the rpmdevtools package
Name:       spart
Version:    1.4.3
Release:    %{slurm_version}.1%{?dist}
Summary:    A tool to display user-oriented Slurm partition information.

Group:      System Environment/Base
License:    GPLv2+
URL:        https://github.com/mercanca/%{name}
Source:     https://github.com/mercanca/%{name}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  slurm-devel
Requires:   slurm

%description
%{name} is a tool to display user-oriented Slurm partition information.

%prep
%autosetup -n %{name}-%{version}

%build
gcc -lslurm %{name}.c -o %{name}

%install
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_mandir}/man1
install -m 0755 ./%{name} %{buildroot}%{_bindir}/%{name}
install -m 0644 ./%{name}.1.gz %{buildroot}%{_mandir}/man1/%{name}.1.gz

%files
%defattr(755,root,root)
%{_bindir}/%{name}
%{_mandir}/man1/%{name}.1.gz


%changelog
* Thu Dec 03 2020 Kilian Cavalotti <kilian@stanford.edu>
- add Slurm version in package relase
- fix man paths
- fix slurm-devel build dependency
* Thu May 02 2019 Kilian Cavalotti <kilian@stanford.edu>
- initial package
