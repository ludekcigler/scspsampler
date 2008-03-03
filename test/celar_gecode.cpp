#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

#include "../src/gecode/support.hh"
#include "../src/optparse/optparse.h"
#include <gecode/minimodel.hh>
#include <gecode/int.hh>

typedef int VarIdType;
typedef int VarType;
typedef std::set<VarType> Domain;

#include "../src/utils.h"

std::string g_datadir = "/home/luigi/Matfyz/Diplomka/scspsampler/trunk/data/ludek/04/";

class Celar: public Example {
public:
        Celar(const Options &opt): mIntVars(this, 4, 0, 1) {
                
                std::cout << this->status() << ", " << Gecode::SS_FAILED << std::endl;
                // Load domains
                
                std::ifstream domainFile((g_datadir + "/dom.txt").c_str());
                std::string line;

                std::vector<int*> domainList;
                std::vector<size_t> domainSizes;

                while (std::getline(domainFile, line)) {
                        std::vector<std::string> words;
                        tokenize(line, words);

                        std::vector<std::string>::iterator it = words.begin();
                        ++it;
                        int * domain = new int[words.size() - 1];
                        unsigned int i = 0;
                        for (; it != words.end(); ++it, ++i) {
                                domain[i] = parseArg<int>(*it);
                        }

                        domainList.push_back(domain);
                        domainSizes.push_back(words.size() - 1);
                }

                // Load variables
                std::ifstream variablesFile((g_datadir + "/var.txt").c_str());

                std::vector<Gecode::IntVar> variables;
                std::map<VarIdType, size_t> varIdMap;

                while (std::getline(variablesFile, line)) {
                        
                        std::vector<std::string> words;
                        tokenize(line, words);
        
                        if (words.size() < 2)
                                continue; // TODO: throw an exception

                        VarIdType varId = parseArg<VarIdType>(words[0]);
        
                        unsigned int domainId = parseArg<unsigned int>(words[1]);
                        if (domainId >= domainList.size())
                                continue; // TODO: throw an exception

                        Gecode::IntVar v(this, Gecode::IntSet(domainList[domainId], domainSizes[domainId]));
                        variables.push_back(v);
                        mVariableIds.push_back(varId);
                        varIdMap[varId] = variables.size() - 1;
        
                        if (words.size() >= 4) {
                                int targetValue = parseArg<int>(words[2]);
                                int weight = parseArg<int>(words[3]);

                                // Add modification constraint
                                if (weight == 0) {
                                        Gecode::rel(this, v, Gecode::IRT_EQ, targetValue);
                                }
                        }
                }

                // Store integer variables in mIntVars
                mIntVars = Gecode::IntVarArray(this, variables.size());

                for (unsigned int i = 0; i < variables.size(); ++i) {
                        mIntVars[i] = variables[i];
                }

                // Load constraints
                std::ifstream constraintFile((g_datadir + "/ctr.txt").c_str());

                //std::vector<Gecode::BoolVar> boolVars;
                std::vector<Gecode::IntVar> boundVars;

                while (std::getline(constraintFile, line)) {
                        std::vector<std::string> words;
                        tokenize(line, words);

                        if (words.size() < 5) {
                                std::cerr << "Wrong constraint specified: " << line << std::endl;
                                continue;
                        }

                        VarIdType varId1, varId2;
                        Gecode::IntRelType op;
                        int targetValue;
                        unsigned int weight = 0;

                        varId1 = parseArg<VarIdType>(words[0]);
                        varId2 = parseArg<VarIdType>(words[1]);
                
                        if (words[3] == ">")
                                op = Gecode::IRT_GR;
                        else if (words[3] == "<")
                                op = Gecode::IRT_LE;
                        else
                                op = Gecode::IRT_EQ;

                        targetValue = parseArg<int>(words[4]);
                
                        if (words.size() >= 6)
                                weight = parseArg<int>(words[5]);

                        if (weight > 0)
                                continue;

                        Gecode::IntVar var1 = variables[varIdMap[varId1]];
                        Gecode::IntVar var2 = variables[varIdMap[varId2]];

                        Gecode::IntVar maxVar(this, min(var1.min(), var2.min()), max(var1.max(), var2.max()));
                        Gecode::IntVar minVar(this, min(var1.min(), var2.min()), max(var1.max(), var2.max()));
                        boundVars.push_back(maxVar);
                        boundVars.push_back(minVar);

                        Gecode::max(this, var1, var2, maxVar); // max(x, y) == maxVar
                        Gecode::min(this, var1, var2, minVar); // min(x, y) == minVar

                        Gecode::IntArgs koef(2); koef[0] = 1; koef[1] = -1;
                        Gecode::IntVarArgs args(2); args[0] = maxVar; args[1] = minVar;
                        Gecode::linear(this, koef, args, op, targetValue); // max(x, y) - min(x, y) op c
                }

                mBoundVars = Gecode::IntVarArray(this, boundVars.size());

                for (unsigned int i = 0; i < boundVars.size(); ++i) {
                        mBoundVars[i] = boundVars[i];
                }
                
                Gecode::branch(this, mIntVars, Gecode::BVAR_SIZE_MIN, Gecode::BVAL_MIN);
        }

        virtual void print(void) {
               for (unsigned int i = 0; i < mIntVars.size(); ++i) {
                       std::cout << mIntVars[i] << ", ";
               }

               std::cout << std::endl;
        }

        Celar(bool share, Celar &s): Example(share, s) {
                mIntVars.update(this, share, s.mIntVars);
                mVariableIds = s.mVariableIds;
                mBoundVars.update(this, share, s.mBoundVars);
        }

        virtual Space * copy(bool share) {
                return new Celar(share, *this);
        }

        // Add constraint for next better solution
        void constrain(Space* s) {
        }


private:
        Gecode::IntVarArray mIntVars;
        std::vector<VarIdType> mVariableIds;
        Gecode::IntVarArray mBoundVars;
};

int main(int argc, char** argv) {
        OptionParser parser(argc, argv, "scspsampler [OPTIONS] ARGS");

        parser.addOption(/*aShortName*/ 0, /*aLongName*/ "dataset", /*aAlias*/ "dataset",
                        /*aHasArg*/ true, /*aSpecifiedByDefault*/ true,
                        /*aArg*/ "/home/luigi/Matfyz/Diplomka/scspsampler/trunk/data/ludek/04/",
                        /*aHelpText*/ "Path to CELAR dataset directory");
        try {
                parser.parseOptions();
        } catch (const OptionNotRecognized & e) {
        }

        if (parser.isSpecified("help")) {
                std::cout << parser.usage();
        }

        g_datadir = parser.getOptionArg("dataset");

        Options opt("Celar");
        opt.solutions = -1;
        opt.parse(argc,argv);
        Example::run<Celar,Gecode::DFS>(opt);
        return EXIT_SUCCESS;
}
