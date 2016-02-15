Name:       nemo-qml-plugin-messages-internal-qt5

Summary:    QML plugin for internal messages functionality
Version:    0.1.19
Release:    1
Group:      System/Libraries
License:    BSD
URL:        https://github.com/nemomobile/nemo-qml-plugin-messages
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Contacts)
BuildRequires:  pkgconfig(Qt5Test)
BuildRequires:  pkgconfig(TelepathyQt5)
BuildRequires:  pkgconfig(commhistory-qt5)
BuildRequires:  pkgconfig(qtcontacts-sqlite-qt5-extensions)

%description
QML plugin for internal messages functionality in Nemo

%package tests
Summary:    QML plugin for internal messages functionality tests
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}

%description tests
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build

%qmake5 

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%qmake_install

%files
%defattr(-,root,root,-)
%{_libdir}/qt5/qml/org/nemomobile/messages/internal/*

%files tests
%defattr(-,root,root,-)
/opt/tests/nemo-qml-plugin-messages/*
