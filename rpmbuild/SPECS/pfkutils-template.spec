
%define target_root @@RPMBUILD@@/root
%define _usr %{target_root}/usr
%define _var %{target_root}/var
%define pkg_path %{target_root}/pkg_path
%define prefix %{pkg_path}-%{version}
%define source_date_epoch_from_changelog 0
%define __brp_mangle_shebangs true

# do not create /usr/lib/.build_id/ shit.
%define _build_id_links none

Name:     pfkutils
Version:  @@VERSION@@
Release:  @@BUILD_NUM@@
Summary:  utilities PFK finds useful
Group:    pfkutils
License:  Proprietary

# if this package requires external libs, and then
# proceeds to provide them, set this to prevent scanning
AutoReqProv:  no

%description
utilities PFK finds useful

%prep

%build

%install

cd $PFKUTILS
HOME=${ORIG_HOME} make DESTDIR=%{buildroot} install

%pre

echo pre secript

%post

echo post script

%preun

echo preun script

%postun

echo postun script

%files

@@ORIG_HOME@@

%changelog
