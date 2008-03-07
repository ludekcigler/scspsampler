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

#include "types.h"
#include "domain_interval.h"

class Variable {
public:
        Variable(const VarIdType &id, Domain * d): mId(id), mDomain(d) {};

        virtual ~Variable() {
                delete mDomain;
        };

        VarIdType getId() const { return mId; };

        const Domain * getDomain() const { return mDomain; };

        /**
         * Erases a given value from the domain
         */
        void eraseFromDomain(VarType aValue) {
                mDomain->erase(aValue);
        }

        /**
         * Adds a given value to the domain
         */
        void addToDomain(VarType aValue) {
                mDomain->insert(aValue);
        };

        void clearDomain() {
                mDomain->clear();
        };
protected:
        VarIdType mId; // Index of the variable
        Domain *mDomain; // Domain of the variable
};

typedef std::map<VarIdType, Variable *> VariableMap;

class CSPProblem;

class Constraint {
public:
        virtual ~Constraint() {};

        virtual double operator()(Assignment &a) const = 0;
        virtual Scope getScope() const = 0;

        /**
         * Check whether the constraint is soft or hard
         */
        virtual bool isSoft() const {
                return false;
        };

        /**
         * Check whether a given value aValue of variable aVarId has support in the variable
         * domain, given an evidence (assignment of variables)
         */
        virtual bool hasSupport(VarIdType aVarId, VarType aValue, const CSPProblem &aProblem, const Assignment & aEvidence) = 0;

        /**
         * Adds probability aProbability to a given intervals of all constraints' variables
         */
        virtual void addIntervalProbability(DomainIntervalAssignment &aAssignment, double aProbability);

        /**
         * Initializes interval probabilities for all its variables
         * We need the pointer to CSPProblem to know domain for each variable
         */
        virtual void initIntervalProbabilities(CSPProblem &aProblem, unsigned int aMaxIntervals);

        /**
         * Gets a list of intervals for the given variable
         */
        virtual DomainIntervalSet getIntervalProbabilities(VarIdType aVarId) {
                return mIntervalProbabilities[aVarId];
        };
protected:
        Constraint() {};

        std::map<VarIdType, DomainIntervalSet> mIntervalProbabilities;
private:

        void _initIntervalProbabilitiesInternal(Scope &aScope, const CSPProblem &aProblem, Assignment & aAssignment,
                std::map<VarIdType, std::map<VarType, double> > &outVarProbabilities, double &outTotalProbability);
};

typedef std::vector<Constraint *> ConstraintList;

class CSPProblem {
public:
        CSPProblem(VariableMap *v, ConstraintList *c);

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

        /**
         * Non-const version of the previous method
         */
        Variable * getVariableById(VarIdType aId) {
                return (*mVariables)[aId];
        }

        const VariableMap * getVariables() {
                return mVariables;
        };

        const ConstraintList * getConstraints() {
                return mConstraints;
        };

        /**
         * Performs Generalized Arc Consistency on the current problem given some
         * evidence.
         *
         * outRemovedValues is a map of values removed from domains of the variables (this is necessary
         * to restore the original domains later)
         *
         * Returns false is some domain was made empty by the propagation
         */
        bool propagateConstraints(Assignment &aEvidence, std::map<VarIdType, Domain> & outRemovedValues);

        bool propagateConstraints(Assignment &aEvidence, 
                std::map<VarIdType, Domain> & outRemovedValues, VarIdType aChangedVariable);

        /**
         * Add previously removed values back to the domains of the variables
         * (this has the opposite effect to the propagateConstraints method)
         */
        void restoreDomains(const std::map<VarIdType, Domain> &aRemovedValues);

protected:

        VariableMap *mVariables;
        ConstraintList *mConstraints;

        std::map<VarIdType, ConstraintList> mConstraintMap;
private:
        bool _propagateConstraintsInternal(Assignment &aEvidence, 
                std::set<std::pair<Constraint *, VarIdType> > & aConstraintQueue,
                std::map<VarIdType, Domain> & outRemovedValues);
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
