Name:           vimb
Version:        3.6.0
Release:        1
Summary:        The vim-like browser
License:        GPL-3.0-or-later
Group:          Productivity/Networking/Web/Browsers
URL:            https://fanglingsu.github.io/vimb/
Source:         https://github.com/fanglingsu/vimb/archive/%{version}.tar.gz
BuildRequires:  gtk3-devel, gcc
BuildRequires:  webkit2gtk3-devel >= 2.20

%global debug_package %{nil}

%description
vimb is a WebKit-based web browser that behaves like the vimperator
plugin for Firefox, and has usage paradigms from the editor vim.

%prep
%setup -q

%build
make %{?_smp_mflags} PREFIX=%{_prefix}

%install
%make_install PREFIX=%{_prefix}

%files
%{_bindir}/vimb
%dir %{_prefix}/lib/vimb
%{_prefix}/lib/vimb/webext_main.so
%{_datadir}/applications/vimb.desktop
%{_mandir}/man1/vimb.1.gz

%changelog
* Sat Oct 17 2020 Elagost <me@elagost.com>
- Created spec file
