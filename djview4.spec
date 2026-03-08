%define release 1
%define version 4.16

Summary: DjVu viewer
Name: djview4
Version: %{version}
Release: %{release}
License: GPL
Group: Applications/Publishing
Source: djview-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
URL: https://github.com/volnsav/DjView4Experimental

BuildRequires: cmake >= 3.21
BuildRequires: gcc-c++
BuildRequires: qt6-qtbase-devel
BuildRequires: qt6-linguist
BuildRequires: djvulibre-devel >= 3.5.28


%description
DjView4 is a standalone viewer for DjVu documents,
based on the DjVuLibre library and the Qt6 toolkit.


%prep
%setup -q

%build
%cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%{_prefix}
%cmake_build


%install
%cmake_install


%clean
rm -rf %{buildroot}

%files
%doc README.md COPYRIGHT COPYING NEWS
%{_bindir}/djview
%{_datadir}/djvu/
%{_datadir}/applications/
%{_datadir}/icons/
%{_mandir}/man1/

%changelog
* Sat Mar 08 2026 Volodymyr <volnsav@github> 4.16-1
- Version 4.16: export formats reorganization, TIFF export fixes, AVIF deploy via CMake
- Unified DPI presets and export dialog behavior across PDF/TIFF/Image export

* Sat Mar 08 2026 Volodymyr <volnsav@github> 4.15-1
- Version 4.15: PDF export with JBIG2 compression, text layer, dual-mask fills
- Settings: DPI presets, BG downscale, force grayscale, JBIG2 threshold

* Fri Mar 06 2026 Volodymyr <volnsav@github> 4.14-1
- Version 4.14: tabbed document mode, multi-tab viewing, document window logic fixes

* Fri Mar 06 2026 Volodymyr <volnsav@github> 4.13-1
- Version 4.13: Qt6 migration, CMake build system, dark-theme rendering fix
- CMake-only build system; drop autotools/qmake legacy
* Mon Jun 09 2025 Volodymyr <volnsav@github> 4.12-1
- Migrated build system from autotools/qmake to CMake 3.21+
- Dropped NPAPI browser plugin (npdjvu, nsdejavu)
- Now requires Qt6, DjVuLibre >= 3.5.28
* Thu Jan 22 2015 Leon Bottou <leonb@users.sourceforge.net> 4.9-2
- no longer rely on xdg portland tools for appfiles and icons
* Sat Jan 27 2007 Leon Bottou <leonb@users.sourceforge.net> 4.0-1
- initial release.

