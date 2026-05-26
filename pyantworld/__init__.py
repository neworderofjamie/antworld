# python imports
from importlib import metadata

# pyfenn interface
from ._antworld import (AntAgent, PlogSeverity,
                        init_logging, set_clear_colour, set_fog)

__all__ = ["AntAgent", "PlogSeverity",
           "init_logging", "set_clear_colour", "set_fog"]

__version__ = metadata.version("pyantworld")
