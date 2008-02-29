# -*- coding: utf-8 -*-
#
#  Copyright 2008  Luděk Cigler <luc@matfyz.cz>
#  $Id$
#
#  This file is part of SCSPSampler.
# 
#  SCSPSampler is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
# 
#  Hollo is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program. If not, see <http://www.gnu.org/licenses/>.
#

#  SCons configuration file for SCSPSampler

import os

package = 'scspsampler'
version = '0.1.0'

opts = Options(None, ARGUMENTS)

opts.Add('warnings', 'Compiler warnings', 1)

env = Environment(CPPDEFINES = ['DEBUG'], CCFLAGS = '-g',
                ENV = {'PATH': os.environ['PATH'], 'TERM': os.environ['TERM'], 'HOME': os.environ['HOME']},
                CXXFLAGS = Split('-Wall')
                )

Export("env")

def CheckPKGConfig(context, version):
     context.Message( 'Checking for pkg-config... ' )
     ret = context.TryAction('pkg-config --atleast-pkgconfig-version=%s' % version)[0]
     context.Result( ret )
     return ret

def CheckPKG(context, name):
     context.Message( 'Checking for %s... ' % name )
     ret = context.TryAction('pkg-config --exists \'%s\'' % name)[0]
     context.Result( ret )
     return ret

# Configuration:

conf = Configure(env, custom_tests = { 'CheckPKGConfig' : CheckPKGConfig,
                                       'CheckPKG' : CheckPKG })

SConscript('src/SConscript')

SConscript('test/SConscript')
