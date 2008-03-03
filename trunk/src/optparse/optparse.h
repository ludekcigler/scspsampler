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

#ifndef OPTPARSE_H_
#define OPTPARSE_H_

#include <string>
#include <list>
#include <map>
#include <exception>

class OptionNotFound: std::exception {
};

class OptionNotRecognized: std::exception {
public:
        OptionNotRecognized(const std::string & aOptionName): mOptionName(aOptionName) {
        };

        virtual ~OptionNotRecognized() throw () {};

        virtual const char * what() const throw () {
                std::string s("Unrecognized option ");
                s = s + mOptionName;
                return s.c_str();
        };
private:
        std::string mOptionName;
};

struct Option {
        Option() {};

        Option(char aShortName, const std::string & aLongName, const std::string & aAlias,
                        bool aHasArg, bool aSpecifiedByDefault, const std::string & aArg,
                        const std::string & aHelpText) :
                shortName(aShortName), longName(aLongName), alias(aAlias), 
                hasArg(aHasArg), specified(aSpecifiedByDefault), specifiedByDefault(aSpecifiedByDefault),
                arg(aArg), helpText(aHelpText) {};

        char shortName;
        std::string longName;
        std::string alias;
        bool hasArg, specified, specifiedByDefault;
        std::string arg, helpText;
};
        
typedef std::map<std::string, Option> OptionMap;
typedef std::list<std::string> ArgList;

/**
 * Rather simple option parser with powerful API
 */
class OptionParser {
public:
        OptionParser(int argc, char ** argv, std::string aUsage = "");

        /**
         * Add an options to the parser
         */
        void addOption(const Option & aOption);
        void addOption(char aShort, const std::string &aLong, const std::string &aAlias,
                        bool aHasArg = false, bool aSpecifiedByDefault = false,
                        const std::string &aDefaultArg = "", const std::string &aHelpText = "");

        /**
         * Reads the argument of an option, returning default if the option was not defined
         */
        std::string getOptionArg(const std::string &aAlias) const throw (OptionNotFound);

        /**
         * Determines whether the option was specified
         */
        bool isSpecified(const std::string &aAlias) const throw (OptionNotFound);

        /**
         * Returns list of remaining arguments
         */
        ArgList getArgs() const;

        /**
         * Parses the options
         */
        void parseOptions() throw (OptionNotRecognized);

        std::string usage() const;
private:
        OptionMap::iterator _findOptionByShortName(char aShortName);
        OptionMap::iterator _findOptionByLongName(const std::string & aLongName);

        int mArgc;
        char ** mArgv;
        std::string mUsage;
        OptionMap mOptions;
        ArgList mArgs;
};


#endif // OPTPARSE_H_

