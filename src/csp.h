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

#ifndef CSP_H_
#define CSP_H_

#include <set>
#include <map>
#include <string>
#include <vector>

#include <assert.h>

typedef int VarType;

typedef unsigned int VarIdType;

typedef std::set<VarType> Domain;

typedef std::map<VarIdType, VarType> Assignment;

typedef std::set<VarIdType> Scope;

typedef std::vector<Scope> Bucket;

typedef std::map<VarType, double> ProbabilityDistribution;

class Variable {
public:
        Variable(VarIdType id, Domain * d): mId(id), mDomain(d) {};

        ~Variable() {
                delete mDomain;
        };

        VarIdType getId() const { return mId; };

        const Domain * getDomain() const { return mDomain; };
private:
        VarIdType mId; // Index of the variable
        Domain *mDomain; // Domain of the variable
};

typedef std::map<VarIdType, Variable *> VariableMap;

class Constraint {
public:
        virtual ~Constraint() {};

        virtual double operator()(Assignment &a) const = 0;
        virtual Scope getScope() const = 0;
protected:
        Constraint() {};
};

typedef std::vector<Constraint *> ConstraintList;

class CSPProblem {
public:
        CSPProblem(VariableMap *v, ConstraintList *c):
                mVariables(v), mConstraints(c) {
                assert(v);
                assert(c);
        
        };

        ~CSPProblem();

        /**
         * Split functions into mini-buckets containing at most aMaxBucketSize variables
         * If aMaxBucketSize is less than maximum scope size, maximum scope size is used
         *
         * aMiniBuckets         List of mini-buckets created
         * aOutsideBucketArcs   List of arcs between mini-buckets which were created
         */

        void schematicMiniBucket(unsigned int aMaxBucketSize, const std::vector<VarIdType> & aOrdering,
                std::vector<Bucket> * aMiniBuckets, std::map<Scope, Scope> * aOutsideBucketArcs);

        double evalAssignment(Assignment &a) const;

        const Variable * getVariableById(VarIdType aId) const {
                return (*mVariables)[aId];
        };

        const VariableMap * getVariables() {
                return mVariables;
        };

        const ConstraintList * getConstraints() {
                return mConstraints;
        };

protected:

        VariableMap *mVariables;
        ConstraintList *mConstraints;
};

class CSPSampler {
public:
        CSPSampler(CSPProblem * aProblem): mProblem(aProblem) {};

        virtual ~CSPSampler() {
                delete mProblem;
        };

        /**
         * Gets new sample from the solution space
         */
        virtual const Assignment getSample() = 0;
protected:
        CSPProblem * mProblem;
};

void assignment_pprint(const Assignment & a);

void scope_pprint(const Scope & aScope);

#endif // CSP_H_
