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

#include <assert.h>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "optparse.h"

OptionParser::OptionParser(int argc, char ** argv, std::string aUsage):
        mArgc(argc), mArgv(argv), mUsage(aUsage) {

        // Add HELP option
        addOption('h', "help", "help", false, false, "", "Show this help message and exit"); 
};

void OptionParser::addOption(const Option & aOption) {
        mOptions[aOption.alias] = aOption;
}

void OptionParser::addOption(char aShort, const std::string &aLong, const std::string &aAlias,
                        bool aHasArg, bool aSpecifiedByDefault,
                        const std::string &aDefaultArg, const std::string &aHelpText) {

        Option o(aShort, aLong, aAlias, aHasArg, aSpecifiedByDefault, aDefaultArg, aHelpText);

        mOptions[aAlias] = o;
}

std::string OptionParser::getOptionArg(const std::string &aAlias) const throw (OptionNotFound) {
        OptionMap::const_iterator optIt = mOptions.find(aAlias);
        if (optIt == mOptions.end())
                throw OptionNotFound();

        return optIt->second.arg;
}

bool OptionParser::isSpecified(const std::string &aAlias) const throw (OptionNotFound) {
        OptionMap::const_iterator optIt = mOptions.find(aAlias);
        if (optIt == mOptions.end())
               throw OptionNotFound();

       return optIt->second.specified;
}

std::list<std::string> OptionParser::getArgs() const {
        return mArgs;
}

void OptionParser::parseOptions() throw (OptionNotRecognized) {
        ++mArgv; --mArgc;

        while (mArgc > 0) {
                std::string arg(*mArgv);

                if (arg.length() >= 3 && arg[0] == '-' && arg[1] == '-') {
                        // Look for long option
                        size_t equalPos = arg.find_first_of('=', 2);
                        std::string longName, optionArgument = "";

                        if (equalPos == std::string::npos) {
                                longName = arg.substr(2);
                        } else {
                                longName = arg.substr(2, equalPos - 2);
                                optionArgument = arg.substr(equalPos + 1);
                        }
                        
                        OptionMap::iterator optIt = _findOptionByLongName(longName);
                        if (optIt == mOptions.end()) {
                                throw OptionNotRecognized(arg);
                        }

                        // If the argument was not specified using "--opt=ARG" syntax,
                        // but should be specified, let's consume another arg
                        if (optIt->second.hasArg && equalPos == std::string::npos && mArgc > 1) {
                                --mArgc, ++mArgv;
                                optionArgument = std::string(*mArgv);
                        }

                        optIt->second.specified = true;
                        optIt->second.arg = optionArgument;
                } else if (arg[0] == '-') {
                        for (size_t i = 1; i < arg.length(); ++i) {
                                std::string optionArgument = "";
                                OptionMap::iterator optIt = _findOptionByShortName(arg[i]);
                                if (optIt == mOptions.end()) {
                                        throw OptionNotRecognized(std::string("") + arg[i]);
                                }

                                optIt->second.specified = true;

                                if (optIt->second.hasArg && i + 1 == arg.length() && mArgc > 1) {
                                        --mArgc, ++mArgv;
                                        optionArgument = std::string(*mArgv);
                                        optIt->second.arg = optionArgument;
                                        break;
                                }
                        }
                } else if (arg == "--") {
                        // End of options
                        --mArgc, ++mArgv;
                        break;
                } else {
                        // Not an option, presumably an argument
                        mArgs.push_back(arg);
                }

                --mArgc, ++mArgv;
        }

        // Parse the rest of the arguments after "--"
        while (mArgc > 0) {
                mArgs.push_back(std::string(*mArgv));
                --mArgc, ++mArgv;
        }
}

std::string OptionParser::usage() const {
        std::ostringstream out;

        if (mUsage == "")
                out << "Usage: %prog [OPTIONS] ARGS" << std::endl;
        else
                out << "Usage: " << mUsage << std::endl;

        out << std::endl << "Options:" << std::endl;
        
        out.setf(std::ios::left);
        for (OptionMap::const_iterator optIt = mOptions.begin(); optIt != mOptions.end(); ++optIt) {
                out << std::setw(30) << (
                                (optIt->second.shortName > 0 ? std::string(" -") + optIt->second.shortName + "," : std::string("    "))
                                + " --" + optIt->second.longName
                                + (optIt->second.hasArg ? "=ARG" : ""));
                out << optIt->second.helpText << (optIt->second.specifiedByDefault ? " [default]" : "") << std::endl;
        }

        return out.str();
}

OptionMap::iterator OptionParser::_findOptionByShortName(char aShortName) {
        for (OptionMap::iterator optIt = mOptions.begin(); optIt != mOptions.end(); ++optIt) {
                if (optIt->second.shortName == aShortName) {
                        return optIt;
                }
        }
        return mOptions.end();
}

OptionMap::iterator OptionParser::_findOptionByLongName(const std::string & aLongName) {
        for (OptionMap::iterator optIt = mOptions.begin(); optIt != mOptions.end(); ++optIt) {
                if (optIt->second.longName == aLongName) {
                        return optIt;
                }
        }
        return mOptions.end();
}

