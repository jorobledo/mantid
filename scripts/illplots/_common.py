# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from mantid import mtd, config
from mantid.api import WorkspaceGroup

import matplotlib.pyplot as plt

import os


def getWorkspaces(names):
    if isinstance(names, str):
        ws = mtd[names]
        if isinstance(ws, WorkspaceGroup):
            return [e for e in ws]
        return ws
    if isinstance(names, list):
        return [mtd[name] for name in names]
    else:
        raise ValueError('"' + str(names) + '"' + " is not a workspace name or "
                         "a list of workspace names.")


def exportFig(fig, filename):
    plt.tight_layout(0)
    if filename is None:
        fig.show()
    else:
        exportPath = config.getString("defaultsave.directory")
        fullPath = os.path.join(exportPath, filename)
        fig.savefig(fullPath)
