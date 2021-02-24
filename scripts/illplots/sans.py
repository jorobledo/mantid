# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from ._common import getWorkspaces, exportFig

from mantid import logger
from mantid import plots

import matplotlib.pyplot as plt

import math


def plotIQ(wsNames, filename=None):
    try:
        wss = getWorkspaces(wsNames)
    except Exception as ex:
        logger.error("Unable to plot I(Q), check your input : " + str(ex))
        return None, None

    fig, ax = plt.subplots()
    ax.set_title("I (Q)")
    ax.set_xscale("log")
    ax.set_yscale("log")
    for ws in wss:
        plots.axesfunctions.errorbar(ax, ws, specNum=1, label=ws.getName())
    ax.legend()

    exportFig(fig, filename)
    return fig, ax


def plotKratky(wsNames, filename=None):
    try:
        wss = getWorkspaces(wsNames)
    except Exception as ex:
        logger.error("Unable to plot IQ**2(Q), check your input : " + str(ex))
        return None, None

    fig, ax = plt.subplots()
    ax.set_title("Kratky plot")
    ax.set_xscale("linear")
    ax.set_yscale("linear")
    for ws in wss:
        x = ws.dataX(0)
        y = ws.dataY(0) * ws.dataX(0)**2
        ax.plot(x, y, label=ws.getName())

    exportFig(fig, filename)
    return fig, ax


def plotIQxQy(wsNames, filename=None):
    try:
        wss = getWorkspaces(wsNames)
    except Exception as ex:
        logger.error("Unable to plot I(Qx, Qy), check your input : " + str(ex))
        return None, None

    n = len(wss)
    if n == 1:
        fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
        ax.pcolormesh(wss[0])
        ax.set_title(wss[0].getName())
    else:
        nr = int(math.ceil(math.sqrt(n)))
        nc = int(n / nr)
        if n%nr != 0:
            nc += 1
        fig, axs = plt.subplots(nr, nc, subplot_kw={'projection':'mantid'})
        axs = axs.flatten()
        for i in range(len(axs)):
            if i < n:
                axs[i].pcolormesh(wss[i])
                axs[i].set_aspect(1)
                axs[i].set_title(wss[i].getName())
            else:
                axs[i].set_visible(False)

    exportFig(fig, filename)
    return fig, axs.reshape(nr, nc)
