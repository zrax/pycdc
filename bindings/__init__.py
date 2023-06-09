import sys

from .bindings import decompyle as _decompyle

__version__ = '0.0.1'


def decompyle(code, version=(sys.version_info.major, sys.version_info.minor)):
    """
    Decompyle the given code object.

    Parameters
    ----------
    code : bytes
        The code object to decompile.
    version : tuple, optional
        The Python version to decompile for. Defaults to the current Python version.
        Use None or (0, 0) to infer the Python version from the code object. This will
        not work for marshalled code objects.
    """
    if version is None:
        return _decompyle(code, 0, 0)
    else:
        return _decompyle(code, version[0], version[1])
