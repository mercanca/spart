# run "spectool -g -R spart.spec" to automatically download source files
# spectool is part of the rpmdevtools package
Name:       spart
Version:    1.3.0
Release:    1%{?dist}
Summary:    A tool to display user-oriented Slurm partition information.

Group:      System Environment/Base
License:    GPLv2+
URL:        https://github.com/mercanca/%{name}
Source:     https://github.com/mercanca/%{name}/archive/v%{version}/%{name}-%{version}.tar.gz


BuildRequires:  slurm
Requires:   slurm

%description
%{name} is a tool to display user-oriented Slurm partition information.

%prep
%autosetup -n %{name}-%{version}

%build
gcc -lslurm %{name}.c -o %{name}

%install
mkdir -p %{buildroot}%{_bindir}
install -m 0755 ./%{name} %{buildroot}%{_bindir}/%{name}

%files
%defattr(755,root,root)
%{_bindir}/%{name}


%changelog
* Thu May 02 2019 Kilian Cavalotti <kilian@stanford.edu>
- initial package
