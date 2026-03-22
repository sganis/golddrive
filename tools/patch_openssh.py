# tools/patch_openssh.py
# patch openssh-portable v10 windows to build with vendor openssl 3.6 and zlib
# openssl is built without no-deprecated so deprecated APIs are available.
# patches needed:
# - paths.targets: use generic SDK version
# - vcxproj: disable spectre mitigation
# - config.h.vs: fix EVP_CIPHER_CTX_get_iv/set_iv (libressl-only, not in openssl)
# - Directory.Build.props: inject vendor include/lib paths
import os
import glob

DIR = os.path.dirname(os.path.realpath(__file__))
VENDOR = os.path.normpath(os.path.join(DIR, '..', 'vendor'))

# paths.targets: use generic SDK version so msbuild picks whatever is installed
path = r'contrib\win32\openssh\paths.targets'
with open(path) as r:
    data = r.read()
with open(path, 'wt') as w:
    w.write(data.replace(
        '<WindowsSDKVersion>10.0.22621.0</WindowsSDKVersion>',
        '<WindowsSDKVersion>10.0</WindowsSDKVersion>'))

# patch all vcxproj: disable spectre mitigation
for path in glob.glob(r'contrib\win32\openssh\*.vcxproj'):
    with open(path) as r:
        data = r.read()
    if '<SpectreMitigation>Spectre</SpectreMitigation>' in data:
        with open(path, 'wt') as w:
            w.write(data.replace(
                '<SpectreMitigation>Spectre</SpectreMitigation>',
                '<SpectreMitigation>false</SpectreMitigation>'))

# config.h.vs: fix openssl 3.x compatibility
path = r'contrib\win32\openssh\config.h.vs'
with open(path) as r:
    data = r.read()
# EVP_CIPHER_CTX_get_iv/set_iv are libressl-only, not in openssl 3.x
# unset these so the compat shims in libressl-api-compat.c are used
data = data.replace(
    '#define HAVE_EVP_CIPHER_CTX_GET_IV 1',
    '/* #undef HAVE_EVP_CIPHER_CTX_GET_IV */\n'
    '#define HAVE_EVP_CIPHER_CTX_IV 1')
data = data.replace(
    '#define HAVE_EVP_CIPHER_CTX_SET_IV 1',
    '/* #undef HAVE_EVP_CIPHER_CTX_SET_IV */\n'
    '#define HAVE_EVP_CIPHER_CTX_IV_NOCONST 1')
# EVP_DigestSign/Verify exist in openssl 3.x, skip the compat shims
data += '\n#define HAVE_EVP_DIGESTSIGN 1\n'
data += '#define HAVE_EVP_DIGESTVERIFY 1\n'
with open(path, 'wt') as w:
    w.write(data)

# create Directory.Build.props to inject vendor openssl/zlib include and lib paths
props = fr'''<?xml version="1.0" encoding="utf-8"?>
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemDefinitionGroup>
    <ClCompile>
      <AdditionalIncludeDirectories>{VENDOR}\openssl\include;{VENDOR}\zlib\include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <PreprocessorDefinitions>OPENSSL_API_COMPAT=0x10100000L;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <DisableSpecificWarnings>4047;%(DisableSpecificWarnings)</DisableSpecificWarnings>
    </ClCompile>
    <Link>
      <AdditionalLibraryDirectories>{VENDOR}\openssl\lib\x64;{VENDOR}\zlib\lib\x64;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
      <AdditionalOptions>/ignore:4099 /ignore:4098 /ignore:4286 %(AdditionalOptions)</AdditionalOptions>
    </Link>
  </ItemDefinitionGroup>
</Project>
'''
with open(r'contrib\win32\openssh\Directory.Build.props', 'wt') as w:
    w.write(props)
