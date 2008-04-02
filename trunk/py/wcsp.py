#! /usr/bin/env python
# -*- coding: utf-8 -*-
##
## Copyright (C) 2008 Luděk Cigler <luc@matfyz.cz>
##
## This file is part of SCSPSampler.
##
## SCSPSampler is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## Hollo is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program. If not, see <http://www.gnu.org/licenses/>.
##

import re
import os.path
from optparse import OptionParser
import random
import math

import psyco
psyco.full()


WCSP_PROBLEM_LINE = re.compile('^(?P<name>\S+)\s+(?P<num_vars>\d+)\s+(?P<max_domain_size>\d+)\s+(?P<total_constraints>\d+)\s+(?P<upper_bound>\d+)')

EXP_ROOT = 2.0
EXP_K = 0.001

class WCSPVariable:
    def __init__(self, var_id, domain_size):
        self._var_id = var_id
        self._domain = range(0, domain_size)

    def random_value(self):
        return random.choice(self._domain)

class WCSPConstraint:
    def __init__(self, scope, default_cost, upper_bound):
        self._scope = scope
        self._default_cost = int(default_cost)
        self._upper_bound = upper_bound
        self._disallowed_tuples = set()
        self._different_tuples = {}

    def add_different_tuple(self, a_tuple, cost):
        self._different_tuples[a_tuple] = cost
        if cost >= self._upper_bound:
            # Add disallowed tuple
            self._disallowed_tuples.add(a_tuple)

    def eval_assignment(self, assignment):
        a_tuple = []
        for val in self._scope:
            a_tuple.append(assignment[val])

        a_tuple = tuple(a_tuple) 
        if a_tuple in self._disallowed_tuples:
            return 0
        else:
            cost = (self._different_tuples.has_key(a_tuple) and self._different_tuples[a_tuple]) or self._default_cost
            return math.exp(math.log(EXP_ROOT) * EXP_K * (-cost))

class WCSPProblem:
    def __init__(self, filename):
        self._variables = []
        self._constraints = []
        file = open(filename)

        line = file.readline()
        # Read the WCSP prologue
        m = WCSP_PROBLEM_LINE.match(line)
        if not m:
            return
        m_groups = m.groupdict()
        self._num_vars = int(m_groups['num_vars'])
        self._max_domain_size = int(m_groups['max_domain_size'])
        self._total_constraints = int(m_groups['total_constraints'])
        self._upper_bound = int(m_groups['upper_bound'])

        # Load variables
        line = file.readline()
        domain_sizes = line.split()
        self._variables = [WCSPVariable(var_id, int(domain_size)) for domain_size, var_id in zip(domain_sizes, xrange(0, len(domain_sizes)))]

        while file:
            ctr_line = file.readline()
            ctr_data = ctr_line.split()
            if len(ctr_data) <= 0:
                break

            default_cost = ctr_data[-2]
            num_different_tuples = int(ctr_data[-1])
            c = WCSPConstraint(set([int(x) for x in ctr_data[1:-2]]), default_cost, self._upper_bound)
            for i in xrange(0, num_different_tuples):
                tuple_line = file.readline()
                tuple_line = tuple_line.split()
                tuple_cost = int(tuple_line[-1])
                tuple_data = [int(x) for x in tuple_line[0:-1]]
                c.add_different_tuple(tuple(tuple_data), tuple_cost)
            self._constraints.append(c)

    def eval_assignment(self, assignment):
        result = 1.0
        for c in self._constraints:
            result = result * c.eval_assignment(assignment)
        return result

    def random_sample(self, MAX_SAMPLES):
        for i in xrange(0, MAX_SAMPLES):
            sample = []
            for var in self._variables:
                sample.append(var.random_value())
            yield sample
                
if __name__ == '__main__':
    usage = "%prog [OPTIONS]"
    parser = OptionParser(usage=usage, version= "%prog 0.1")
    parser.add_option('-d', '--dataset',
                      action='store',
                      dest='dataset',
                      type='string',
                      metavar='PATH',
                      default='/home/luigi/Matfyz/Diplomka/scspsampler/trunk/data/wcsp/celar_small.wcsp',
                      help="PATH to data set")

    parser.add_option('-n', '--numsamples',
                      action='store',
                      dest='numsamples',
                      type='int',
                      metavar='PATH',
                      default='1000',
                      help="Number of generated samples")

    parser.add_option('-k', '--koef',
                      action='store',
                      dest='koef',
                      type='float',
                      metavar='VALUE',
                      default='0.01',
                      help="Coefitient of the algorithm")

    (options, args) = parser.parse_args()

    problem = WCSPProblem(options.dataset)
    EXP_K = options.koef

    for sample in problem.random_sample(options.numsamples):
        e = problem.eval_assignment(sample)
        if e > 0.0:
            print "%e, %s" % (e, ", ".join([str(x) for x in sample]))

