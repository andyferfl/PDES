#!/bin/python3
# -*- Mode:python; c-file-style:"gnu"; indent-tabs-mode:nil; -*-
#
# Copyright (c) 2021 Lawrence Livermore National Laboratory
# All rights reserved.
#
# Author:  Peter D. Barnes, Jr. <pdbarnes@llnl.gov>


"""
Configuration for the classic PHOLD benchmark.
"""
# import sst is managed below

import argparse
import os
import pprint
import sys
import textwrap

phold_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, phold_dir)
import progress_dot as dot

import sst
from sst import *


# print(sst.__file__.dir(sst))

# from sst import Component

def phprint(args):
    """Print progress information, prefixed by our name."""
    script = os.path.basename(__file__)
    print(script, ': ', args)

    # print(script, ': ', end='')
    # for arg in args:
    #     print(arg, sep='', end='')
    # print('')

def vprint(level: int, args):
    """Print a verbose message at verbosity level."""
    if phold.pyVerbose >= level:
        phprint(args)

def dprint(message: str, dictionary: dict):
    """Print a header message, followed by a pretty printed dictionary d."""
    if phold.pyVerbose:
        phprint(message)
        dict_pretty = pprint.pformat(dictionary, indent=2, width=1)
        print(textwrap.indent(dict_pretty, '    '))



# PHOLD parameters
class PholdArgs(dict):
    """PHOLD argument processing and validation.

    Methods
    -------
    parse() : None
        Parse the arguments and store the values.

    print() : None
        Pretty print the configuration.

    Attributes
    ----------
    See print() or _init_argparse(), or run with '--help'
    """

    # pylint: disable=too-many-instance-attributes

    def __init__(self):
        """Initialize the configuration with default values.

        Attributes
        ----------
        See print() or _init_argparse(), or run with '--help'
        """
        super().__init__()
        self.remote = 0.9
        self.minimum = 1
        self.thread = 3
        self.average = 100
        self.stop = 1000000
        self.number = 1000
        self.events = 1
        self.buffer = 0
        self.stats = True
        self.delays = False
        self.delaysAutoscale = False
        self.pverbose = 0
        self.pyVerbose = 0
        self.TIMEBASE = "s"

    def __str__(self) -> str:
        return f"remote: {self.remote}, " \
               f"min: {self.minimum}, " \
               f"min: {self.thread}, " \
               f"avg: {self.average}, " \
               f"stop: {self.stop}, " \
               f"nodes: {self.number}, " \
               f"events: {self.events}, " \
               f"buffer: {self.buffer}, " \
               f"stats: {self.stats}, " \
               f"delays: {self.delays}, " \
               f"delaysAuto: {self.delaysAutscale}, " \
               f"verbose: {self.pverbose}, " \
               f"pyVerbose: {self.pyVerbose}"

    def print(self):
        """Pretty print the configuration."""

        # Syncrhonization duty factor: window size divided by total expected delay
        duty_factor = self.minimum / (self.minimum + self.average)
        # Expected number of events at each LP per window
        ev_per_win = self.events * duty_factor
        
        # Min target number of events per window
        min_ev_per_win = 0
        # Suggested minimum number of events per LP
        min_events = round(min_ev_per_win / duty_factor)
        phprint(f"  duty_factor : {duty_factor}")
        print(f"    Remote LP fraction:                   {self.remote}")
        print(f"    Minimum inter-event delay:            {self.minimum} {self.TIMEBASE}")
        print(f"    Inter-thread min delay:               {self.thread} {self.TIMEBASE}")
        print(f"    Additional exponential average delay: {self.average} {self.TIMEBASE}")
        print(f"    Stop time:                            {self.stop} {self.TIMEBASE}")
        print(f"    Number of LPs:                        {self.number}")
        print(f"    Number of initial events per LP:      {self.events}")
        print(f"    Size of event data buffer:            {self.buffer}")

        print(f"    Approx. events per LP per window:     {ev_per_win:.2f}")
        if ev_per_win < min_ev_per_win:
            print(f"    (Too low!  Suggest setting '--events={min_events}')")

        print(f"    Output statistics:                    {self.stats}")
        print(f"      Include delay histogram:            {self.delays}")
        print(f"      Autoscale:                          {self.delaysAutoscale}")
        print(f"    Verbosity level:                      {self.pverbose}")
        print(f"    Python script verbosity level:        {self.pyVerbose}")

    @property
    def _validate(self) -> bool:
        """Validate the configuration."""
        valid = True
        if self.remote < 0 or self.remote > 1:
            phprint(f"Invalid remote fraction: {self.remote}, must be in [0,1]")
            valid = False
        if self.minimum <= 0:
            phprint(f"Invalid minimum delay: {self.minimum}, must be > 0")
            valid = False
        if self.thread <= 0:
            phprint(f"Invalid inter-thread delay: {self.thread}, must be > 0")
            valid = False
        if self.average < 0:
            phprint(f"Invalid average delay: {self.average}, must be >= 0")
            valid = False
        if self.stop <= 0:
            phprint(f"Invalid stop time: {self.stop}, must be > 0")
            valid = False

        self.number = int(self.number)
        if self.number < 2:
            phprint(f"Invalid number: {self.number}, need at least 2")
            valid = False

        self.events = int(self.events)
        if self.events < 1:
            phprint(f"Invalid initial events: {self.events}, need at least 1")
            valid = False
        
        self.buffer = int(self.buffer)
        if self.buffer < 0:
            phprint(f"Invalid event buffer size: {self.buffer}, can't be negative")
            valid = False

        return valid

    def _init_argparse(self) -> argparse.ArgumentParser:
        """Configure the argument parser with our arguments."""
        script = os.path.basename(__file__)
        parser = argparse.ArgumentParser(
            usage=f"sst {script} [OPTION]...",
            description="Execute the standard PHOLD benchmark.")
        parser.add_argument(
            '-r', '--remote', action='store', type=float,
            help=f"Fraction of events which should be scheduled for other LPs. "
            f"Must be in [0,1], default {self.remote}.")
        parser.add_argument(
            '-m', '--minimum', action='store', type=float,
            help=f"Minimum inter-event delay, in {self.TIMEBASE}. "
            f"Must be >0, default {self.minimum}.")
        parser.add_argument(
            '-t', '--thread', action='store', type=float,
            help=f"Inter-thread minimum delay, in {self.TIMEBASE}. "
            f"Must by >0, default {self.thread}.")
        parser.add_argument(
            '-a', '--average', action='store', type=float,
            help=f"Average additional inter-event delay, in {self.TIMEBASE}. "
            f"This will be added to the min delay, and must be >= 0, "
            f"default {self.average}.")
        parser.add_argument(
            '-s', '--stop', action='store', type=float,
            help=f"Total simulation time, in {self.TIMEBASE}. "
            f"Must be > 0, default {self.stop}.")
        parser.add_argument(
            '-n', '--number', action='store', type=int,
            help=f"Total number of LPs. "
            f"Must be > 2, default {self.number}.")
        parser.add_argument(
            '-e', '--events', action='store', type=int,
            help=f"Number of initial events per LP. "
            f"Must be > 0, default {self.events}")
        parser.add_argument(
            '-b', '--buffer', action='store', type=int,
            help=f"Size of event data buffer. "
            f"Must be non-negative, default {self.buffer}")
        parser.add_argument(
            # '--verbose' conflicts with SST, even after --
            '-v', '--pverbose', action='count',
            help=f"Phold module verbosity, default {self.pverbose}.")
        parser.add_argument(
            '-S', '--stats', action='store_true',
            help=f"Output statistics, default {self.stats}.")
        parser.add_argument(
            '-d', '--delays', action='store_true',
            help=f"Whether to output delay histogram, default {self.delays}.")
        parser.add_argument(
            '-A', '--delaysAutoscale', action='store_true',
            help=f"Autscale delay histogram, default {self.delaysAutoscale}.")
        parser.add_argument(
            '-V', '--pyVerbose', action='count',
            help=f"Python script verbosity, default {self.pyVerbose}.")
        return parser

    def parse(self):
        """Parse and validate the script arguments.

        If the configuration is not valid print the help and exit.

        If pyVerbose is on show the configuration. (The C++ Phold class
        will always print the final configuration.)
        """
        parser = self._init_argparse()
        parser.parse_args(namespace=self)
        if not self._validate:
            parser.print_help()
            sys.exit(1)

        if self.pyVerbose > 1:
            phprint(f"Configuration at parsing:")
            self.print()


# -- Main --

print(f"")
phprint(f"Creating PHOLD Benchmark")

# If sst not found, we're just debugging this script
try:
    import sst
    just_script = False
    phprint(f"Importing SST module")
except ImportError as error:
    just_script = True
    phprint(f"No SST module, just debugging this script")


# Phold arguments instance
phold = PholdArgs()
phold.parse()

if phold.pyVerbose:
    phprint(f"Configuration:")
    phold.print()

if just_script:
    phprint(f"Nothing left to do, exiting")
    sys.exit(1)

# Create the LPs
phprint(f"Creating {phold.number} LPs")
dotter = dot.Dot(phold.number, phold.pyVerbose)
lps = []
for i in range(phold.number):
    if dotter.dot(1):
        vprint(2, f"  Creating LP {i}")
    lp = sst.Component("phold_" + str(i), 'phold.Phold')
    lp.addParams(vars(phold))  # pass ph as simple dictionary
    lps.append(lp)
dotter.done()

# min latency
latency = str(phold.minimum) + ' ' + phold.TIMEBASE
thread_latency = str(phold.thread) + ' ' + phold.TIMEBASE
nranks = sst.getMPIRankCount()

# Assume block partitioning: 
#   Each rank holds 
nLP_rank = phold.number / nranks
#   Rank 0 holds 0-nLP_rank - 1

# Add links
num_links = int(phold.number * (phold.number - 1) / 2)
phprint(f"Creating complete graph with latency {latency} ({num_links} total)")
dotter = dot.Dot(num_links, phold.pyVerbose)
for i in range(phold.number):
    rank_i = i // nLP_rank
    for j in range(i + 1, phold.number):
        rank_j = j // nLP_rank
        lat = thread_latency if (rank_i == rank_j) else latency

        if dotter.dot(2, False):
            vprint(2, f"  Creating link {i}_{j} between ranks {rank_i} and {rank_j} with latency {lat}")
        link = sst.Link("link_" + str(i) + '_' + str(j))

        # links cross connect ports:
        # port number gives the LP id on the other side of the link
        li = lps[i], 'port_' + str(j), lat
        lj = lps[j], 'port_' + str(i), lat

        if dotter.dot(3):
            vprint(3, f"    creating tuples")
            vprint(3, f"      {li}")
            vprint(3, f"      {lj}")
            vprint(3, f"    connecting {i} to {j}")

        link.connect(li, lj)
dotter.done()

# Enable statistics
stat_level = 1 + phold.delays
phprint(f"Enabling statistics at level {stat_level}")
sst.setStatisticLoadLevel(stat_level)
sst.setStatisticOutput('sst.statOutputCSV')

# Common stats configuration:
# rate: 0us:    Only report results at end
# stopat:       Stop collecting at stop time
stats_config = {'rate': '0ms'}
#,
#                'stopat' : str(phold.stop + 1) + phold.TIMEBASE}

# We always enable the send/recv counters
# Stat type accumulator is the default, so don't need state it explicitly
dprint("  Accumulator config:", stats_config)

sst.enableStatisticsForComponentType('phold.Phold', ['SendCount'], stats_config)
sst.enableStatisticsForComponentType('phold.Phold', ['RecvCount'], stats_config)

if phold.delays:
    delay_mean = phold.minimum + phold.average
    numbins = 50
    binwidth = round(5 * delay_mean / numbins)
    autoscale = 0
    if phold.delaysAutoscale:
        autoscale = 1

    delays_config = {**stats_config,
                     **{'type' : 'sst.HistogramStatistic',
                        'minvalue' : '0',
                        'binwidth' : str(binwidth),
                        'numbins'  : str(numbins),
                        'autoscale': str(autoscale)}}

    dprint("Delay histogram config:", delays_config)

    sst.enableStatisticsForComponentType('phold.Phold', ['Delays'], delays_config)


# Set overall program options
phprint(f"Setting SST options")
sst_timebase = "1ms"
phprint(f"  Time base: {sst_timebase}")
sst.setProgramOption('timebase', sst_timebase)
phprint(f"  Print timing: yes")
sst.setProgramOption('print-timing-info', '1')

phprint(f"Done\n")
