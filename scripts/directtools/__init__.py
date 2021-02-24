from mantid import logger

import sys


logger.warning("Deprecation warning. Please import directools with 'import "
               "illplots.direct' from now.")

sys.modules[__name__] = __import__('illplots.direct', fromlist=['*'])

