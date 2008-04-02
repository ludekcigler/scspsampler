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

#ifndef INTERVAL_IJGP_H_
#define INTERVAL_IJGP_H_

#include "csp.h"
#include "ijgp.h"

enum {
        MAX_DOMAIN_INTERVALS = 10,
        MAX_VALUES_FROM_INTERVAL = 2
};

class IntervalJoinGraphEdge;

class IntervalJoinGraphMessage;

class IntervalJoinGraphNode {
public:
        IntervalJoinGraphNode(const Scope & s, unsigned int aMaxDomainIntervals, unsigned int aMaxValuesFromInterval):
                mScope(s), mMaxDomainIntervals(aMaxDomainIntervals), mMaxValuesFromInterval(aMaxValuesFromInterval) {};

        IntervalJoinGraphNode(const IntervalJoinGraphNode * aNode):
                mScope(aNode->mScope),                 
                mDomainIntervals(aNode->mDomainIntervals),
                mConstraintDomainIntervals(aNode->mConstraintDomainIntervals),
                mTotalProbabilities(aNode->mTotalProbabilities),
                mMaxDomainIntervals(aNode->mMaxDomainIntervals),
                mMaxValuesFromInterval(aNode->mMaxValuesFromInterval) {};



        ~IntervalJoinGraphNode();

        Scope getScope() const {
                return mScope;
        };

        void addEdge(IntervalJoinGraphEdge * e) {
                assert(e);
                mEdges.push_back(e);
        };

        void addConstraint(Constraint * aConstraint) {
                mConstraints.push_back(aConstraint);
        };

        /**
         * Initializes domain intervals for each variable in the scope
         * by merging constraint domain intervals
         */
        void initDomainIntervals(CSPProblem * aProblem);

        void adjustIntervalsToDomains(const CSPProblem * aProblem);

        void restoreDomainIntervals();

        void setMessage(IntervalJoinGraphNode * aNodeFrom, IntervalJoinGraphMessage * aMessage);

        IntervalJoinGraphMessage * getMessage(const CSPProblem * aProblem, IntervalJoinGraphEdge * aEdge,
                const Assignment & aEvidence, const Scope & aEvidenceScope);

        /**
         * Removes all messages stored in the node
         */
        void purgeMessages();

        /**
         * Kullback-Leibler divergence between new and old messages
         */
        int KLDivergence(double & outDivergence);

        /**
         * Compute conditional probability distribution P(X_j|e)
         */
        IntervalProbabilityDistribution conditionalDistribution(CSPProblem * aProblem, 
                        Variable * aTargetVariable, const Assignment & aEvidence);
private:
        friend class IntervalJoinGraph;

        void _computeMessage(IntervalJoinGraphMessage * aMessage, const CSPProblem * aProblem,
                Scope & aMessageScope, Scope & aMarginalizedScope, std::vector<DomainInterval> & aMessageScopeValues,
                Assignment & aAssignment, const IntervalJoinGraphNode * const aExcludeNode = 0);

        double _marginalizeOut(const CSPProblem * aProblem, Scope & aMarginalizedScope,
                Assignment & aAssignment, const IntervalJoinGraphNode * const aExcludeNode = 0);

        double _evalAssignment(Assignment & aAssignment, const IntervalJoinGraphNode * const aExcludeNode,
                const CSPProblem * aProblem);

        void _clearDomainIntervalProbabilities();

        void _normalizeDomainIntervalProbabilities(const CSPProblem * aProblem);

        void _addDomainIntervalProbability(const Assignment &aEvidence, double aProbability);

        /**
         * List of edges the node has
         */
        std::list<IntervalJoinGraphEdge *> mEdges;

        Scope mScope;

        ConstraintList mConstraints;

        std::map<IntervalJoinGraphNode *, IntervalJoinGraphMessage *> mMessages;

        /**
         * Messages from previous iteration, used for deciding whether we should
         * stop iterating or not
         */
        std::map<IntervalJoinGraphNode *, IntervalJoinGraphMessage *> mOldMessages;

        /**
         * Domain intervals for each variable in the scope and stored accumulated domain intervals for all constraints
         */
        std::map<VarIdType, DomainIntervalMap> mDomainIntervals, mConstraintDomainIntervals;

        std::map<VarIdType, double> mTotalProbabilities;

        unsigned int mMaxDomainIntervals, mMaxValuesFromInterval;
};

class IntervalJoinGraphEdge {
public:
        IntervalJoinGraphEdge(IntervalJoinGraphNode * aTargetNode, const Scope & aScope):
                mTargetNode(aTargetNode), mScope(aScope) {
                
                assert(aTargetNode);
        };

        Scope getScope() const {
                return mScope;
        };

        IntervalJoinGraphNode * targetNode() {
                return mTargetNode;
        };

        const IntervalJoinGraphNode * targetNode() const {
                return mTargetNode;
        };
private:
        IntervalJoinGraphNode *mTargetNode;
        Scope mScope;
};

typedef std::map<Scope, JoinGraphNode *> JoinGraphNodeMap;

class IntervalJoinGraph {
public:
        IntervalJoinGraph(unsigned int aMaxDomainIntervals = MAX_DOMAIN_INTERVALS,
                        unsigned int aMaxValuesFromInterval = MAX_VALUES_FROM_INTERVAL):
                mMaxDomainIntervals(aMaxDomainIntervals), mMaxValuesFromInterval(aMaxValuesFromInterval) {};

        // Copy-constructor
        IntervalJoinGraph(const IntervalJoinGraph & aGraph);
        ~IntervalJoinGraph();

        /**
         * Performs iterative join-graph propagation on this graph
         * given evidence
         */
        void iterativePropagation(CSPProblem * aProblem, Assignment & aEvidence,
                        unsigned int aMaxIterations = MAX_PROPAGATION_ITERATIONS);

        /**
         * Cleans up messages from previous computations
         */
        void purgeMessages();

        /**
         * Initializes domain intervals for all graph nodes
         */
        void initDomainIntervals(CSPProblem * aProblem) {
                for (std::map<Scope, IntervalJoinGraphNode *>::iterator nodeIt = mNodes.begin();
                                nodeIt != mNodes.end(); ++nodeIt) {

                        nodeIt->second->initDomainIntervals(aProblem);
                }
        };

        /**
         * Initializes domain intervals for all graph nodes
         */
        void adjustIntervalsToDomains(const CSPProblem * aProblem) {
                for (std::map<Scope, IntervalJoinGraphNode *>::iterator nodeIt = mNodes.begin();
                                nodeIt != mNodes.end(); ++nodeIt) {

                        nodeIt->second->adjustIntervalsToDomains(aProblem);
                }
        };

        /**
         * Initializes domain intervals for all graph nodes
         */
        void restoreDomainIntervals() {
                for (std::map<Scope, IntervalJoinGraphNode *>::iterator nodeIt = mNodes.begin();
                                nodeIt != mNodes.end(); ++nodeIt) {

                        nodeIt->second->restoreDomainIntervals();
                }
        };

        /**
         * Orders nodes along some ordering
         */
        void orderNodes();

        /**
         * Kullback-Leibler divergence between the old and new messages
         */
        int KLDivergence(double & outDivergence);

        /**
         * Creates a join-graph with node size limited to i variables
         */
        static IntervalJoinGraph * createJoinGraph(CSPProblem * aProblem, unsigned int aMaxBucketSize,
                        unsigned int aMaxDomainIntervals = MAX_DOMAIN_INTERVALS,
                        unsigned int aMaxValuesFromInterval = MAX_VALUES_FROM_INTERVAL);

        /**
         * Pretty-prints the join graph
         */
        std::string pprint() const;

        /**
         * Compute conditional probability distribution P(X_j|e)
         */
        IntervalProbabilityDistribution conditionalDistribution(CSPProblem * aProblem, 
                        Variable * aTargetVariable, const Assignment & aEvidence);

private:
        /**
         * An (arbitrary) ordering of the graph nodes
         */
        std::vector<Scope> mOrdering;

        std::map<Scope, IntervalJoinGraphNode *> mNodes;

        unsigned int mMaxDomainIntervals, mMaxValuesFromInterval;
};

class IntervalJoinGraphMessage {
public:
        IntervalJoinGraphMessage(const Scope & aScope, 
                        std::map<VarIdType, DomainIntervalMap> &aDomainIntervals):
                mScope(aScope), mTotalProbability(0.0), mNormalized(false) 
        {
                // Copy the scope intervals
                for (Scope::iterator scopeIt = mScope.begin(); scopeIt != mScope.end(); ++scopeIt) {
                        mDomainIntervals[*scopeIt] = aDomainIntervals[*scopeIt];
                }
        };

        void addProbability(const std::vector<DomainInterval> & aIntervals, double aProbability) {
                assert(!mNormalized); // After normalization, no probability should be adjusted

                std::map<std::vector<DomainInterval>, double>::iterator probIt = mProbabilityTable.find(aIntervals);

                if (probIt == mProbabilityTable.end())
                        mProbabilityTable[aIntervals] = aProbability;
                else
                        probIt->second += aProbability;

                mTotalProbability += aProbability;
        }

        Scope getScope() const {
                return mScope;
        }

        double evalAssignment(Assignment &a, const CSPProblem * aProblem);

        void normalize();

        double KLDivergence(const IntervalJoinGraphMessage * aOldMessage) const;

        const DomainIntervalMap & getDomainIntervalMap(VarIdType aVarId) {
                return mDomainIntervals[aVarId];
        }

        std::string pprint() const;
private:
        std::map<std::vector<DomainInterval>, double> mProbabilityTable;
        std::map<VarIdType, DomainIntervalMap> mDomainIntervals;
        Scope mScope;
        double mTotalProbability;
        bool mNormalized;
};


#endif // INTERVAL_IJGP_H_
