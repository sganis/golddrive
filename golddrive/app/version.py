"""Get win32 binary version information."""

import argparse
import ctypes
from ctypes.wintypes import BOOL, DWORD, LPCWSTR, LPVOID
import sys


LPCVOID = LPVOID
LPVOIDP = ctypes.POINTER(LPVOID)
LPDWORD = ctypes.POINTER(DWORD)
PUINT = ctypes.POINTER(ctypes.c_uint)

_version = ctypes.windll.version

# <https://msdn.microsoft.com/library/ms647005>
GetFileVersionInfoSize = ctypes.WINFUNCTYPE(DWORD, LPCWSTR, LPDWORD)(
    ("GetFileVersionInfoSizeW", _version))

# <https://msdn.microsoft.com/library/ms647003>
GetFileVersionInfo = ctypes.WINFUNCTYPE(BOOL, LPCWSTR, DWORD, DWORD, LPVOID)(
    ("GetFileVersionInfoW", _version))

# <https://msdn.microsoft.com/library/ms647464>
VerQueryValue = ctypes.WINFUNCTYPE(BOOL, LPCVOID, LPCWSTR, LPVOIDP, PUINT)(
    ("VerQueryValueW", _version))


# <https://msdn.microsoft.com/library/ms646997>
class VS_FIXEDFILEINFO(ctypes.Structure):
    """Non-textual version information for a file."""

    _fields_ = (
        ("dwSignature", DWORD),
        ("dwStrucVersion", DWORD),
        ("dwFileVersionMS", DWORD),
        ("dwFileVersionLS", DWORD),
        ("dwProductVersionMS", DWORD),
        ("dwProductVersionLS", DWORD),
        ("dwFileFlagsMask", DWORD),
        ("dwFileFlags", DWORD),
        ("dwFileOS", DWORD),
        ("dwFileType", DWORD),
        ("dwFileSubtype", DWORD),
        ("dwFileDateMS", DWORD),
        ("dwFileDateLS", DWORD),
    )

    def _unpack_version(self, field_base):
        ms = getattr(self, field_base + "MS")
        ls = getattr(self, field_base + "LS")
        return _hi(ms), _lo(ms), _hi(ls), _lo(ls)


def _hi(dword):
    return int(dword >> 16)


def _lo(dword):
    return int(dword & 0xFFFF)


class ResourceError(AttributeError):
    """Given resource was not found in the file."""


class VersionInfo(object):
    """Access to win32 VersionInfo resources."""

    string_names = (
        "Comments", "CompanyName", "FileDescription", "FileVersion",
        "InternalName", "LegalCopyright", "LegalTrademarks",
        "OriginalFilename", "PrivateBuild", "ProductName", "ProductVersion",
        "SpecialBuild",
    )

    def __init__(self, path):
        size = GetFileVersionInfoSize(path, None)
        if size == 0:
            raise ctypes.WinError()
        self._buffer = ctypes.create_string_buffer(size)
        if not GetFileVersionInfo(path, 0, size, self._buffer):
            raise ctypes.WinError()
        self._root = self._get_root()

    @property
    def file_version(self):
        return self._root._unpack_version("dwFileVersion")

    @property
    def product_version(self):
        return self._root._unpack_version("dwProductVersion")

    def _query_value(self, key):
        buf = self._buffer
        ptr = LPVOID()
        size = ctypes.c_uint()
        if not VerQueryValue(buf, key, ctypes.byref(ptr), ctypes.byref(size)):
            err = ctypes.WinError()
            if err.winerror == 1813:
                raise ResourceError(key)
            raise err
        return ptr, size.value

    def _get_root(self):
        ptr, size = self._query_value("\\")
        if size < ctypes.sizeof(VS_FIXEDFILEINFO):
            raise ValueError("Buffer smaller than VS_FIXEDFILEINFO")
        return ctypes.cast(ptr, ctypes.POINTER(VS_FIXEDFILEINFO)).contents

    def get_translations(self):
        try:
            ptr, size = self._query_value("\\VarFileInfo\\Translation")
        except ResourceError:
            return []
        array = ctypes.cast(ptr, ctypes.POINTER(DWORD * (size // 4))).contents
        return [(_lo(value), _hi(value)) for value in array]

    def get_string(self, name, lang, codepage):
        key = "\\StringFileInfo\\{:04x}{:04x}\\{}".format(lang, codepage, name)
        ptr, size = self._query_value(key)
        return ctypes.wstring_at(ptr, size)

    def iter_strings(self, lang, codepage):
        for key in self.string_names:
            try:
                yield key, self.get_string(key, lang, codepage)
            except ResourceError:
                pass


def escape(string, encoding):
    """Handle Python being bad at printing unicode to Windows terminals."""
    escaped = string.encode(encoding, "backslashreplace")
    if sys.version_info[0] == 2:
        return escaped
    return escaped.decode(encoding)

def get_file_version(filename):
    try:
        info = VersionInfo(filename)
        return '.'.join([repr(x) for x in info.file_version])
    except:
        return None

def main(filename):
    try:
        info = VersionInfo(filename)
        print ("Binary file version: {}".format(info.file_version))
        print ("Binary product version: {}".format(info.product_version))
        translations = info.get_translations()
        for trans in translations:
            print ("Have translation lang: {} charset: {}".format(*trans))
        try:
            lang = translations[0][0]
            codepage = translations[0][1]
        except IndexError:
            raise ValueError("No translations, give --lang and --codepage")
        for key, value in info.iter_strings(lang, codepage):
            print ("{}: {}".format(key, value))
    except (OSError, ValueError) as e:
        sys.stderr.write("error: {}\n".format(e))
        return 1
    return 0



if __name__ == '__main__':

    f = r'C:\Program Files (x86)\WinFsp\bin\winfsp-x64.dll'
    f = r'C:\Users\sant\Documents\golddrive\golddrive\sshfs\bin\cygwin1.dll'
    # main(f)
    print(get_file_version(f))