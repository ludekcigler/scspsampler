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
import constraint

CELAR_VARIABLE_LINE = re.compile('^\s*(?P<var_id>\d+)\s+(?P<domain_id>\d+)(\s+(?P<target_value>\d+)\s+(?P<weight>\d+))?')
CELAR_CONSTRAINT_LINE = re.compile('^\s*(?P<var_id_1>\d+)\s+(?P<var_id_2>\d+)\s+\w+\s+(?P<operator>[\=\<\>])\s+(?P<target_value>\d+)(\s+(?P<weight>\d+))?')

class CelarSolver:
    
    def __init__(self, data_path):
        self._problem = constraint.Problem()
        self._domains = {}

        self.load_domains(os.path.join(data_path, "dom.txt"))
        self.load_variables(os.path.join(data_path, "var.txt"))
        self.load_constraints(os.path.join(data_path, "ctr.txt"))

    def load_domains(self, data_path):
        file = open(data_path)

        for line in file:
            l = line.split()
            domain_id = int(l[0])
            self._domains[domain_id] = [int(x) for x in l[1:]]

    def load_variables(self, data_path):
        file = open(data_path)

        for line in file:
            m = CELAR_VARIABLE_LINE.match(line)
            if m:
                m_groups = m.groupdict()
                self._problem.addVariable(m_groups['var_id'], self._domains[int(m_groups['domain_id'])])
                #print "Added variable '%s', %s" % (m_groups['var_id'], self._domains[int(m_groups['domain_id'])])
                if m_groups['weight'] and (int(m_groups['weight']) == 0):
                    # We only add hard constraints here
                    self._problem.addConstraint(lambda a: a == int(m_groups['target_value']), m_groups['var_id'])

    def load_constraints(self, data_path):
        file = open(data_path)

        for line in file:
            m = CELAR_CONSTRAINT_LINE.match(line)
            if m:
                m_groups = m.groupdict()
                if m_groups['weight'] and int(m_groups['weight']) > 0:
                    continue
                operator = m_groups['operator']
                if operator == '=':
                    #print "Added constraint abs('%s' - '%s') == %d" % (m_groups['var_id_1'], m_groups['var_id_2'], int(m_groups['target_value']))
                    self._problem.addConstraint(lambda a, b: abs(a - b) == int(m_groups['target_value']), \
                                (m_groups['var_id_1'], m_groups['var_id_2']))
                elif operator == '>':
                    #print "Added constraint abs('%s' - '%s') > %d" % (m_groups['var_id_1'], m_groups['var_id_2'], int(m_groups['target_value']))
                    self._problem.addConstraint(lambda a, b: abs(a - b) > int(m_groups['target_value']), \
                                (m_groups['var_id_1'], m_groups['var_id_2']))
                elif operator == '<':
                    self._problem.addConstraint(lambda a, b: abs(a - b) < int(m_groups['target_value']), \
                                (m_groups['var_id_1'], m_groups['var_id_2']))

    def get_solution_iter(self):
        return self._problem.getSolutionIter()


if __name__ == '__main__':
    usage = "%prog [OPTIONS]"
    parser = OptionParser(usage=usage, version= "%prog 0.1")
    parser.add_option('-d', '--datadir',
                      action='store',
                      dest='datadir',
                      type='string',
                      metavar='PATH',
                      default='/home/luigi/Matfyz/Diplomka/scspsampler/trunk/data/ludek/02/',
                      help="PATH to data set")

    (options, args) = parser.parse_args()

    solver = CelarSolver(options.datadir)
    solutionIter = solver.get_solution_iter()

    for sol in solutionIter:
        print sol

