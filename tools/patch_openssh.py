# patch openssh-portable v10 windows to build with our vendor libraries
# our openssl 3.6 is built with no-deprecated, but openssh uses deprecated
# APIs (EC_KEY, RSA, DSA). The symbols exist in libcrypto.lib but are hidden
# by headers. We define OPENSSL_API_COMPAT=0x10100000L to expose them.
# We also fix config.h.vs for functions removed in openssl 3.6:
# - BN_is_prime_ex (removed, use BN_check_prime)
# - EVP_CIPHER_CTX_get_iv/set_iv (removed, use compat shims)
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

# config.h.vs: fix feature flags for openssl 3.6 (no-deprecated build)
path = r'contrib\win32\openssh\config.h.vs'
with open(path) as r:
    data = r.read()
# BN_is_prime_ex doesn't exist in openssl 3.6 no-deprecated
data = data.replace(
    '#define HAVE_BN_IS_PRIME_EX 1',
    '/* #undef HAVE_BN_IS_PRIME_EX */')
# EVP_CIPHER_CTX_get_iv/set_iv were removed in openssl 3.0.8+
# unset these so the compat shims in libressl-api-compat.c are used
data = data.replace(
    '#define HAVE_EVP_CIPHER_CTX_GET_IV 1',
    '/* #undef HAVE_EVP_CIPHER_CTX_GET_IV */\n'
    '#define HAVE_EVP_CIPHER_CTX_GET_UPDATED_IV 1')
data = data.replace(
    '#define HAVE_EVP_CIPHER_CTX_SET_IV 1',
    '/* #undef HAVE_EVP_CIPHER_CTX_SET_IV */\n'
    '#define HAVE_EVP_CIPHER_CTX_IV_NOCONST 1')
with open(path, 'wt') as w:
    w.write(data)

# moduli.c: replace BN_is_prime_ex with BN_check_prime
path = 'moduli.c'
with open(path) as r:
    data = r.read()
data = data.replace('BN_is_prime_ex(q, 1, NULL, NULL)', 'BN_check_prime(q, NULL, NULL)')
data = data.replace('BN_is_prime_ex(p, trials, NULL, NULL)', 'BN_check_prime(p, NULL, NULL)')
data = data.replace('BN_is_prime_ex(q, trials - 1, NULL, NULL)', 'BN_check_prime(q, NULL, NULL)')
data = data.replace('BN_is_prime_ex failed', 'BN_check_prime failed')
with open(path, 'wt') as w:
    w.write(data)

# create Directory.Build.props to:
# - inject vendor openssl/zlib include and library paths
# - define OPENSSL_API_COMPAT=0x10100000L to expose deprecated APIs in headers
#   (the symbols are compiled into libcrypto.lib, just hidden by header guards)
props = fr'''<?xml version="1.0" encoding="utf-8"?>
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemDefinitionGroup>
    <ClCompile>
      <AdditionalIncludeDirectories>{VENDOR}\openssl\include;{VENDOR}\zlib\include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <PreprocessorDefinitions>OPENSSL_API_COMPAT=0x10100000L;%(PreprocessorDefinitions)</PreprocessorDefinitions>
    </ClCompile>
    <Link>
      <AdditionalLibraryDirectories>{VENDOR}\openssl\lib\x64;{VENDOR}\zlib\lib\x64;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
    </Link>
  </ItemDefinitionGroup>
</Project>
'''
with open(r'contrib\win32\openssh\Directory.Build.props', 'wt') as w:
    w.write(props)
