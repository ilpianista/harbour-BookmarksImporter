Name:       harbour-bookmarksimporter

Summary:    Import browser bookmarks from an HTML file into the Sailfish browser.
Version:    0.1.0
Release:    1
License:    MIT
URL:        https://github.com/ilpianista/harbour-BookmarksImporter
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5 >= 0.10.9
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  desktop-file-utils

%description
Import bookmarks from a Netscape-format HTML file (exported by desktop
browsers such as Firefox) into the Sailfish browser's bookmark list.

%if 0%{?_chum}
Title: Bookmarks Importer
Type: desktop-application
DeveloperName: Andrea Scarpino
Categories:
 - Network
Custom:
  Repo: https://github.com/ilpianista/harbour-bookmarksimporter
Icon: https://raw.githubusercontent.com/ilpianista/harbour-bookmarksimporter/master/icons/harbour-bookmarksimporter.svg
Screenshots:
 - https://raw.githubusercontent.com/ilpianista/harbour-SailHN/master/screenshots/screenshot_1.png
Links:
  Homepage: https://github.com/ilpianista/harbour-bookmarksimporter
  Bugtracker: https://github.com/ilpianista/harbour-bookmarksimporter/issues
  Donation: https://liberapay.com/ilpianista
%endif


%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
%make_build

%install
%qmake5_install

desktop-file-install --delete-original \
    --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
