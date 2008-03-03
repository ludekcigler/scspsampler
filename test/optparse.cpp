/*
 * Copyright 2008 Luděk Cigler <luc@matfyz.cz>
 * $Id$
 *
 * This file is part of SCSPSampler.
 *
 * SCSPSampler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Hollo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <iostream>

#include "../src/optparse/optparse.h"

int main(int argc, char ** argv) {
        OptionParser parser(argc, argv, "optparse [OPTIONS] ARGS");

        parser.addOption('f', "file", "file", false, true, false, "", "Constraint file");
        parser.addOption(0, "yep", "yep", false, false, false, "", "Yep, empty short option");

        parser.parseOptions();

        if (parser.isSpecified("help")) {
                std::cout << parser.usage();
        } else if (parser.isSpecified("file")) {
                std::cout << parser.getOptionArg("file") << std::endl;
        }
        return 0;
}
