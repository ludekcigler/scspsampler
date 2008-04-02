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
#include <math.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>

#include "csp.h"
#include "interval_ijgp.h"
#include "utils.h"

#define RECOMPUTE_DOMAIN_INTERVALS 0

IntervalJoinGraph::IntervalJoinGraph(const IntervalJoinGraph & aGraph):
        mOrdering(aGraph.mOrdering),
        mMaxDomainIntervals(aGraph.mMaxDomainIntervals), mMaxValuesFromInterval(aGraph.mMaxValuesFromInterval) {

        // Create nodes
        for (std::map<Scope, IntervalJoinGraphNode *>::const_iterator nodesIt = aGraph.mNodes.begin();
                        nodesIt != aGraph.mNodes.end(); ++nodesIt) {
                IntervalJoinGraphNode * node = new IntervalJoinGraphNode(nodesIt->second);
                mNodes[nodesIt->first] = node;
        }

        // Create edges between these nodes
        for (std::map<Scope, IntervalJoinGraphNode *>::const_iterator nodesIt = aGraph.mNodes.begin();
                        nodesIt != aGraph.mNodes.end(); ++nodesIt) {
                IntervalJoinGraphNode * node = mNodes[nodesIt->first];

                for (std::list<IntervalJoinGraphEdge *>::const_iterator edgeIt = nodesIt->second->mEdges.begin();
                                edgeIt != nodesIt->second->mEdges.end(); ++edgeIt) {
                        Scope edgeScope((*edgeIt)->getScope());
                        Scope targetNodeScope((*edgeIt)->targetNode()->getScope());
                        IntervalJoinGraphEdge * edge = new IntervalJoinGraphEdge(mNodes[targetNodeScope], edgeScope);

                        node->addEdge(edge);
                }
        }

        // Copy messages
        for (std::map<Scope, IntervalJoinGraphNode *>::const_iterator nodesIt = aGraph.mNodes.begin();
                        nodesIt != aGraph.mNodes.end(); ++nodesIt) {
                IntervalJoinGraphNode * node = mNodes[nodesIt->first];

                for (std::map<IntervalJoinGraphNode *, IntervalJoinGraphMessage *>::iterator msgIt = nodesIt->second->mMessages.begin();
                                msgIt != nodesIt->second->mMessages.end(); ++msgIt) {

                        Scope messageNodeScope = msgIt->first->getScope();
                        node->mMessages[mNodes[messageNodeScope]] = new IntervalJoinGraphMessage(*(msgIt->second));
                }

                for (std::map<IntervalJoinGraphNode *, IntervalJoinGraphMessage *>::iterator msgIt = nodesIt->second->mOldMessages.begin();
                                msgIt != nodesIt->second->mOldMessages.end(); ++msgIt) {

                        Scope messageNodeScope = msgIt->first->getScope();
                        node->mOldMessages[mNodes[messageNodeScope]] = new IntervalJoinGraphMessage(*(msgIt->second));
                }
        }
}

IntervalJoinGraphNode::~IntervalJoinGraphNode() {
        purgeMessages();

        for (std::list<IntervalJoinGraphEdge *>::const_iterator edgeIt = mEdges.begin();
                edgeIt != mEdges.end(); ++edgeIt) {
                delete (*edgeIt);
        }
        mEdges.clear();

        for (std::map<IntervalJoinGraphNode *, IntervalJoinGraphMessage *>::iterator msgIt = mMessages.begin();
                        msgIt != mMessages.end(); ++msgIt) {

                delete msgIt->second;
        }
        
        mMessages.clear();

        for (std::map<IntervalJoinGraphNode *, IntervalJoinGraphMessage *>::iterator msgIt = mOldMessages.begin();
                        msgIt != mOldMessages.end(); ++msgIt) {

                delete msgIt->second;
        }
        mOldMessages.clear();
}

IntervalJoinGraph::~IntervalJoinGraph() {
        for (std::map<Scope, IntervalJoinGraphNode *>::iterator nodeIt = mNodes.begin();
                        nodeIt != mNodes.end(); ++nodeIt) {
                delete nodeIt->second;
        }
}

IntervalJoinGraph * IntervalJoinGraph::createJoinGraph(CSPProblem * aProblem, unsigned int aMaxBucketSize,
                unsigned int aMaxDomainIntervals, unsigned int aMaxValuesFromInterval) {
        std::vector<Bucket> miniBuckets;
        std::map<Scope, Scope> outsideBucketArcs;

        Graph * G = Graph::createCSPPrimalGraph(aProblem);

        std::vector<VarIdType> ordering = G->minInducedWidthOrdering();

        aProblem->schematicMiniBucket(aMaxBucketSize, ordering, &miniBuckets, &outsideBucketArcs);

#ifdef DEBUG
        /*
        for (int i = miniBuckets.size() - 1; i >= 0; --i) {
                std::cout << "mini-bucket[" << i << "]: ";
                for (std::vector<Scope>::iterator mbIt1 = miniBuckets[i].begin(); mbIt1 != miniBuckets[i].end(); ++mbIt1) {

                        std::cout << scope_pprint(*mbIt1);
                }
                std::cout << std::endl;
        }
        */
#endif

        IntervalJoinGraph * joinGraph = new IntervalJoinGraph(aMaxDomainIntervals, aMaxValuesFromInterval);
        // Create join-graph node for every mini-bucket
        for (size_t i = 0; i < miniBuckets.size(); ++i) {
                for (std::vector<Scope>::iterator mbIt = miniBuckets[i].begin(); mbIt != miniBuckets[i].end(); ++mbIt) {
                        IntervalJoinGraphNode * node = new IntervalJoinGraphNode(*mbIt, aMaxDomainIntervals, aMaxValuesFromInterval);
                        joinGraph->mNodes[*mbIt] = node; 

                        // Append all constraints from the CSPProblem which match the current node's
                        // scope to that node
                        for (ConstraintList::const_iterator ctrIt = aProblem->getConstraints()->begin();
                                        ctrIt != aProblem->getConstraints()->end();
                                        ++ctrIt) {

                                Scope ctrScope = (*ctrIt)->getScope();
                                Scope nodeScope = node->getScope();

                                if (std::includes(nodeScope.begin(), nodeScope.end(), ctrScope.begin(), ctrScope.end())) {
                                        node->addConstraint(*ctrIt);
                                }
                        }

                        node->initDomainIntervals(aProblem);

                        // If there is an edge from this mini-bucket to some other bucket created before,
                        // add it now
                        std::map<Scope, Scope>::iterator outsideArcIt = outsideBucketArcs.find(*mbIt);
                        if (outsideArcIt != outsideBucketArcs.end()) {
                                Scope edgeScope;
                                std::set_intersection(outsideArcIt->first.begin(), outsideArcIt->first.end(),
                                                outsideArcIt->second.begin(), outsideArcIt->second.end(),
                                                std::inserter(edgeScope, edgeScope.begin()));

                                IntervalJoinGraphEdge * edge1 = new IntervalJoinGraphEdge(joinGraph->mNodes[outsideArcIt->first],
                                                edgeScope);

                                joinGraph->mNodes[outsideArcIt->second]->addEdge(edge1);

                                IntervalJoinGraphEdge * edge2 = new IntervalJoinGraphEdge(joinGraph->mNodes[outsideArcIt->second],
                                                edgeScope);
                                joinGraph->mNodes[outsideArcIt->first]->addEdge(edge2);

                        }
                }

                // Create edges between nodes from a single bucket

                for (std::vector<Scope>::iterator mbIt1 = miniBuckets[i].begin();
                                mbIt1 != miniBuckets[i].end(); ++mbIt1) {
                        std::vector<Scope>::iterator mbIt2 = mbIt1;
                        ++mbIt2;
                        for (; mbIt2 != miniBuckets[i].end(); ++mbIt2) {
                                // Label the edge by the current bucket (ie. ordering[i])
                                Scope edgeScope;
                                edgeScope.insert(ordering[i]);

                                IntervalJoinGraphEdge * edge1 = new IntervalJoinGraphEdge(joinGraph->mNodes[*mbIt2], edgeScope);
                                joinGraph->mNodes[*mbIt1]->addEdge(edge1);

                                IntervalJoinGraphEdge * edge2 = new IntervalJoinGraphEdge(joinGraph->mNodes[*mbIt1], edgeScope);
                                joinGraph->mNodes[*mbIt2]->addEdge(edge2);
                        }
                }
        }

        joinGraph->orderNodes();
        return joinGraph;
}

std::string IntervalJoinGraph::pprint() const {
        std::ostringstream out;
        for (std::map<Scope, IntervalJoinGraphNode *>::const_iterator nodeIt = this->mNodes.begin();
                        nodeIt != this->mNodes.end(); ++nodeIt) {
                out << scope_pprint(nodeIt->first) << std::endl;

                for (std::list<IntervalJoinGraphEdge *>::const_iterator edgeIt = nodeIt->second->mEdges.begin();
                                edgeIt != nodeIt->second->mEdges.end(); ++edgeIt) {
                        out << "\t" << scope_pprint((*edgeIt)->getScope()) << " -> ";
                        out << scope_pprint((*edgeIt)->targetNode()->getScope()) << std::endl;
                }
        }
        return out.str();
}

void IntervalJoinGraphNode::initDomainIntervals(CSPProblem * aProblem) {
        // std::cout << "Initializing node " << scope_pprint(mScope) << std::endl;
        mConstraintDomainIntervals.clear();

        // If there are no constraints in the node, create uniform intervals
        // according to a domain
        if (mConstraints.empty()) { 
                for (Scope::iterator scopeIt = mScope.begin(); scopeIt != mScope.end(); ++scopeIt) {
                        const Domain * domain = aProblem->getVariableById(*scopeIt)->getDomain();
                        mConstraintDomainIntervals[*scopeIt] = uniform_intervals_for_domain(*domain, mMaxDomainIntervals);
                        // std::cout << "Uniform interval for " << *scopeIt << " " << interval_list_pprint(mConstraintDomainIntervals[*scopeIt]) << std::endl;
                }
                mDomainIntervals = mConstraintDomainIntervals;
                return; // No need to do anything else
        } else {
                for (ConstraintList::const_iterator ctrIt = mConstraints.begin();
                        ctrIt != mConstraints.end(); ++ctrIt) {
                        Scope ctrScope = (*ctrIt)->getScope();

                        // Initialize the domain interval table in constraint
                        (*ctrIt)->initDomainIntervals(*aProblem, mMaxDomainIntervals); 

                        for (Scope::const_iterator scopeIt = ctrScope.begin(); scopeIt != ctrScope.end(); ++scopeIt) {
                                VarIdType varId = *scopeIt;
                                //std::cout << "Merging " << varId << std::endl;
                                if (mConstraintDomainIntervals.find(varId) == mConstraintDomainIntervals.end()) {
                                        mConstraintDomainIntervals[varId] = (*ctrIt)->getDomainIntervals(varId);
                                } else {
                                        mConstraintDomainIntervals[varId] = 
                                                merge_intervals(mConstraintDomainIntervals[varId],
                                                                (*ctrIt)->getDomainIntervals(varId));
                                }
                                //std::cout << "Ctr dom interval " << varId << " " << interval_list_pprint(mConstraintDomainIntervals[varId]);
                        }
                }
        }

        for (std::map<VarIdType, DomainIntervalMap>::iterator domIt = mConstraintDomainIntervals.begin();
                        domIt != mConstraintDomainIntervals.end(); ++domIt) {

                const Domain * domain = aProblem->getVariableById(domIt->first)->getDomain();
                domIt->second = adjust_intervals_to_domain(
                                        join_intervals(normalize_intervals(domIt->second), mMaxDomainIntervals),
                                        *domain);
                // Copy the initial constraint domain intervals in the mDomainIntervals map
                mDomainIntervals[domIt->first] = domIt->second;

                // std::cout << "Domain interval " << domIt->first << " " << interval_list_pprint(domIt->second);
        }
}

void IntervalJoinGraphNode::adjustIntervalsToDomains(const CSPProblem * aProblem) {
        for (std::map<VarIdType, DomainIntervalMap>::iterator domIt = mDomainIntervals.begin();
                        domIt != mDomainIntervals.end(); ++domIt) {

                const Domain * domain = aProblem->getVariableById(domIt->first)->getDomain();
                domIt->second = join_intervals(normalize_intervals(adjust_intervals_to_domain(domIt->second, *domain)),
                                mMaxDomainIntervals);
                        //std::cout << "Domain interval after " << domIt->first << ": " << interval_list_pprint(mDomainIntervals[domIt->first]) << std::endl;
        }
}

void IntervalJoinGraphNode::restoreDomainIntervals() {
        for (Scope::iterator scopeIt = mScope.begin(); scopeIt != mScope.end(); ++scopeIt) {
                mDomainIntervals[*scopeIt] = mConstraintDomainIntervals[*scopeIt];
                //std::cout << "Domain interval for " << *scopeIt << ": " << interval_list_pprint(mConstraintDomainIntervals[*scopeIt]) << std::endl;
        }
}

void IntervalJoinGraphNode::setMessage(IntervalJoinGraphNode * aNodeFrom, IntervalJoinGraphMessage * aMessage) {
        
        /*
        std::cout << "IntervalJoinGraphNode::setMessage from node ";
        std::cout << scope_pprint(aNodeFrom->getScope());
        std::cout << " to node ";
        std::cout << scope_pprint(getScope());
        std::cout << ", mesg " << aMessage << "\n";
        */
        std::map<IntervalJoinGraphNode *, IntervalJoinGraphMessage *>::iterator oldIt, newIt;

        oldIt = mOldMessages.find(aNodeFrom);
        if (oldIt != mOldMessages.end()) {
                //std::cout << "Deleting " << mOldMessages[aNodeFrom] << std::endl;
                delete mOldMessages[aNodeFrom];
        }

        newIt = mMessages.find(aNodeFrom);
        if (newIt != mMessages.end())
                mOldMessages[aNodeFrom] = mMessages[aNodeFrom];

        mMessages[aNodeFrom] = aMessage;
}

IntervalJoinGraphMessage * IntervalJoinGraphNode::getMessage(const CSPProblem * aProblem, IntervalJoinGraphEdge * aEdge,
                const Assignment & aEvidence, const Scope & aEvidenceScope) {
        assert(aEdge);

        Scope messageScope;
        Scope edgeScope = aEdge->getScope();

        // Create list of variables which will be the scope of newly created message (ie. all variables
        // from the edge label which are not present in the evidence)
        std::set_difference(edgeScope.begin(), edgeScope.end(), aEvidenceScope.begin(), aEvidenceScope.end(),
                        std::inserter(messageScope, messageScope.begin()));

        // Create list of variables over which we are going to marginalize
        Scope nonMarginalizedScope;
        std::set_union(edgeScope.begin(), edgeScope.end(), aEvidenceScope.begin(), aEvidenceScope.end(),
                        std::inserter(nonMarginalizedScope, nonMarginalizedScope.begin()));

        Scope marginalizedScope;
        std::set_difference(mScope.begin(), mScope.end(), nonMarginalizedScope.begin(), nonMarginalizedScope.end(),
                        std::inserter(marginalizedScope, marginalizedScope.begin()));

        // We start from a partial assignment given by evidence
        Assignment partialAssignment = aEvidence;


        // merge message domain intervals with the domain intervals of the constraints
        // and assign those intervals to the message

        // Re-initialize the domain intervals from the constraint domain intervals
        /*
        for (Scope::iterator scopeIt = mScope.begin(); scopeIt != mScope.end(); ++scopeIt) {
                mDomainIntervals[*scopeIt] = mConstraintDomainIntervals[*scopeIt];
                //std::cout << "Domain interval for " << *scopeIt << ": " << interval_list_pprint(mConstraintDomainIntervals[*scopeIt]) << std::endl;
        }
        
        for (std::map<IntervalJoinGraphNode *, IntervalJoinGraphMessage *>::const_iterator msgIt = mMessages.begin();
                        msgIt != mMessages.end(); ++msgIt) {
                Scope msgScope = msgIt->second->getScope();

                for (Scope::const_iterator scopeIt = msgScope.begin(); scopeIt != msgScope.end(); ++scopeIt) {
                        VarIdType varId = *scopeIt;
                        std::cout << "Domain interval before " << varId << " " << interval_list_pprint(mDomainIntervals[varId]) << std::endl;
                        mDomainIntervals[varId] = merge_intervals(mDomainIntervals[varId], msgIt->second->getDomainIntervalMap(varId));

                        std::cout << "Message interval " << varId << " " << interval_list_pprint(msgIt->second->getDomainIntervalMap(varId)) << std::endl;
                }
        }*/

        for (std::map<VarIdType, DomainIntervalMap>::iterator domIt = mDomainIntervals.begin();
                        domIt != mDomainIntervals.end(); ++domIt) {

                const Domain * domain = aProblem->getVariableById(domIt->first)->getDomain();
                domIt->second = join_intervals(normalize_intervals(adjust_intervals_to_domain(domIt->second, *domain)),
                                mMaxDomainIntervals);
                        //std::cout << "Domain interval after " << domIt->first << ": " << interval_list_pprint(mDomainIntervals[domIt->first]);
        }

        IntervalJoinGraphMessage * message = new IntervalJoinGraphMessage(messageScope, mDomainIntervals);

        if (RECOMPUTE_DOMAIN_INTERVALS) {
                // clear domain interval probabilities
                _clearDomainIntervalProbabilities();
        }

        // Assign values to all unassigned variables (those in the scope of the message
        // and those that need to be marginalized out)

        std::vector<DomainInterval> messageScopeValues;

        _computeMessage(message, aProblem, messageScope, marginalizedScope, messageScopeValues, 
                        partialAssignment, aEdge->targetNode());

        message->normalize();

        if (RECOMPUTE_DOMAIN_INTERVALS) {
                // TODO: normalize domain interval probabilities
                _normalizeDomainIntervalProbabilities(aProblem);
        }

        return message;
}

void IntervalJoinGraphNode::purgeMessages() {
        while (!mOldMessages.empty()) {
                delete (mOldMessages.begin())->second;
                mOldMessages.erase(mOldMessages.begin());
        }

        while (!mMessages.empty()) {
                delete (mMessages.begin())->second;
                mMessages.erase(mMessages.begin());
        }
}

int IntervalJoinGraphNode::KLDivergence(double & outDivergence) {
        outDivergence = 0.0;

        if (mMessages.empty() || mOldMessages.empty())
                return JOIN_GRAPH_ERROR_KL_UNDEFINED;

        for (std::map<IntervalJoinGraphNode *, IntervalJoinGraphMessage *>::iterator msgIt = mMessages.begin();
                        msgIt != mMessages.end(); ++msgIt) {

                IntervalJoinGraphMessage * oldMessage = mOldMessages[msgIt->first];

                outDivergence += msgIt->second->KLDivergence(oldMessage);
        }

        outDivergence = outDivergence / mMessages.size();

        return 0;
}


IntervalProbabilityDistribution IntervalJoinGraphNode::conditionalDistribution(CSPProblem * aProblem, 
                        Variable * aTargetVariable, const Assignment & aEvidence) {

        // We should make sure that the variable is in the current scope
        // If not, we have errors up in node selection
        assert(aTargetVariable);

        VarIdType targetVarId = aTargetVariable->getId();
        assert(mScope.find(targetVarId) != mScope.end());

        IntervalProbabilityDistribution result;
        Assignment partialEvidence = aEvidence;
        Scope evidenceScope;

        for (Assignment::const_iterator aIt = aEvidence.begin(); aIt != aEvidence.end(); ++aIt) {
                evidenceScope.insert(aIt->first);
        }
        // The target variable is in evidence also, in a sense that it is not marginalized out
        evidenceScope.insert(targetVarId); 

        // Create a set of variables which ought to be marginalized out
        Scope marginalizedScope;
        std::set_difference(mScope.begin(), mScope.end(), evidenceScope.begin(), evidenceScope.end(),
                        std::inserter(marginalizedScope, marginalizedScope.begin()));

        const Domain * domain = aTargetVariable->getDomain();
        const DomainIntervalMap & intervalSet = mDomainIntervals[targetVarId];

        Assignment::iterator targetVarIt = partialEvidence.find(targetVarId);
        if (targetVarIt != partialEvidence.end()) {
                // The target variable is already in the evidence, therefore we create
                // only a simple distribution (0, 0, ..., 0, 1, 0, ..., 0)
                VarType targetValue = targetVarIt->second;

                for (DomainIntervalMap::const_iterator intervalIt = intervalSet.begin();
                                intervalIt != intervalSet.end(); ++intervalIt) {
                        if (intervalIt->first.lowerBound <= targetValue && targetValue < intervalIt->first.upperBound) {
                                result[intervalIt->first] = 1.0;
                        } else {
                                result[intervalIt->first] = 0.0;
                        }
                }

                assert(false); // This should never happen during debugging, though...
        } else {
                for (DomainIntervalMap::const_iterator intervalIt = intervalSet.begin();
                                intervalIt != intervalSet.end(); ++intervalIt) {
                        // Assign a given number of values from the selected intervals to the variable
                        double sum = 0.0;
                        for (unsigned int i = 0; i < mMaxValuesFromInterval; ++i) {
                                VarType value = random_select(domain, intervalIt->first.lowerBound, intervalIt->first.upperBound);

                                partialEvidence[targetVarId] = value;
                                sum += _marginalizeOut(aProblem, marginalizedScope, partialEvidence);
                        }
                        result[intervalIt->first] = sum;
                }
        }

        return result;
}

void IntervalJoinGraphNode::_computeMessage(IntervalJoinGraphMessage * aMessage, const CSPProblem * aProblem,
                Scope & aMessageScope, Scope & aMarginalizedScope, std::vector<DomainInterval> & aMessageScopeValues,
                Assignment & aAssignment, const IntervalJoinGraphNode * const aExcludeNode) {

        // If all variables from the message scope have been assigned, we can start marginalizing out
        // the remaining variables
        if (aMessageScope.empty()) {
                double probability = _marginalizeOut(aProblem, aMarginalizedScope, aAssignment, aExcludeNode);
                aMessage->addProbability(aMessageScopeValues, probability);

                //std::cout << "IntervalJoinGraphNode::_computeMessage | aMessage " << aMessage << std::endl;
        } else {
                VarIdType assignedVariable = *(aMessageScope.begin());
                aMessageScope.erase(aMessageScope.begin());

                const DomainIntervalMap & intervalSet = mDomainIntervals[assignedVariable];
                const Domain * domain = aProblem->getVariableById(assignedVariable)->getDomain();

                for (DomainIntervalMap::const_iterator intervalIt = intervalSet.begin(); intervalIt != intervalSet.end(); ++intervalIt) {
                        // Assign a given number of values from the selected intervals to the variable
                        aMessageScopeValues.push_back(intervalIt->first);

                        for (unsigned int i = 0; i < mMaxValuesFromInterval; ++i) {
                                VarType value = random_select(domain, intervalIt->first.lowerBound, intervalIt->first.upperBound);

                                aAssignment[assignedVariable] = value;
                                
                                _computeMessage(aMessage, aProblem, aMessageScope, aMarginalizedScope, 
                                        aMessageScopeValues, aAssignment, aExcludeNode);

                                // Restore previous assignment
                                aAssignment.erase(assignedVariable);
                        }

                        aMessageScopeValues.pop_back();
                }

                aMessageScope.insert(aMessageScope.begin(), assignedVariable);
        }
}

double IntervalJoinGraphNode::_marginalizeOut(const CSPProblem * aProblem, Scope & aMarginalizedScope,
                Assignment & aAssignment, const IntervalJoinGraphNode * const aExcludeNode) {

        // Sum over all possible values of the given variable
        if (aMarginalizedScope.empty()) {
                // We have finally assigned all the variables needed to evaluate the functions
                return _evalAssignment(aAssignment, aExcludeNode, aProblem);
        } else {
                // Otherwise we marginalize out the first variable in the scope
                double sum = 0.0;
                VarIdType marginalizedVariable = *(aMarginalizedScope.begin());
                aMarginalizedScope.erase(aMarginalizedScope.begin());

                const DomainIntervalMap & intervalSet = mDomainIntervals[marginalizedVariable];
                const Domain * domain = aProblem->getVariableById(marginalizedVariable)->getDomain();

                for (DomainIntervalMap::const_iterator intervalIt = intervalSet.begin(); intervalIt != intervalSet.end(); ++intervalIt) {
                        // Assign a given number of values from the selected intervals to the variable
                        for (unsigned int i = 0; i < mMaxValuesFromInterval; ++i) {
                                VarType value = random_select(domain, intervalIt->first.lowerBound, intervalIt->first.upperBound);

                                aAssignment[marginalizedVariable] = value;
                                
                                sum += _marginalizeOut(aProblem, aMarginalizedScope, aAssignment, aExcludeNode);

                                // Restore previous assignment
                                aAssignment.erase(marginalizedVariable);
                        }
                }

                aMarginalizedScope.insert(aMarginalizedScope.begin(), marginalizedVariable);

                return sum;
        }
}

double IntervalJoinGraphNode::_evalAssignment(Assignment & aAssignment, const IntervalJoinGraphNode * const aExcludeNode,
                const CSPProblem * aProblem) {

        double result = 1.0;
        for (ConstraintList::iterator ctrIt = mConstraints.begin(); ctrIt != mConstraints.end(); ++ctrIt) {
                result = result * (**ctrIt)(aAssignment);
        }

        //std::cout << assignment_pprint(aAssignment) << std::endl;
        //std::cout << " IntervalJoinGraphNode::_evalAssignment | mMessages.size(): " << mMessages.size() << std::endl;

        for (std::map<IntervalJoinGraphNode *, IntervalJoinGraphMessage *>::iterator msgIt = mMessages.begin();
                        msgIt != mMessages.end(); ++msgIt) {
                if (msgIt->first == aExcludeNode)
                        continue;

                result = result * msgIt->second->evalAssignment(aAssignment, aProblem);
        }

        //std::cout << "Probability " << result << std::endl;

        if (RECOMPUTE_DOMAIN_INTERVALS) {
                // update probabilities in all domain intervals
                _addDomainIntervalProbability(aAssignment, result);
        }

        return result;
}

void IntervalJoinGraphNode::_clearDomainIntervalProbabilities() {
        for (std::map<VarIdType, DomainIntervalMap>::iterator domIntervalIt = mConstraintDomainIntervals.begin();
                        domIntervalIt != mConstraintDomainIntervals.end(); ++domIntervalIt) {

                for (DomainIntervalMap::iterator domIt = domIntervalIt->second.begin();
                                domIt != domIntervalIt->second.end(); ++domIt) {

                        domIt->second = 0.0;
                }

                mTotalProbabilities[domIntervalIt->first] = 0.0;
        }
}

void IntervalJoinGraphNode::_normalizeDomainIntervalProbabilities(const CSPProblem * aProblem) {
        for (std::map<VarIdType, DomainIntervalMap>::iterator domIntervalIt = mConstraintDomainIntervals.begin();
                        domIntervalIt != mConstraintDomainIntervals.end(); ++domIntervalIt) {

                VarIdType varId = domIntervalIt->first;
                double totalProbability = mTotalProbabilities[varId];

                if (totalProbability <= 0.0) {
                        // Create uniform intervals for all domains
                        domIntervalIt->second = 
                                uniform_intervals_for_domain(*(aProblem->getVariableById(varId)->getDomain()), mMaxDomainIntervals);

                } else {
                        DomainIntervalMap & intervalMap = domIntervalIt->second;
                        for (DomainIntervalMap::iterator domIt = intervalMap.begin();
                                        domIt != intervalMap.end(); ++domIt) {
        
                                domIt->second /= totalProbability;
                        }
                }

                mTotalProbabilities[varId] = 1.0;
        }
}

/**
 * Adds aProbability to the domain interval probability for intervals matching aEvidence
 */
void IntervalJoinGraphNode::_addDomainIntervalProbability(const Assignment &aEvidence, double aProbability) {
        // Find intervals matching the assignment
        
        for (Assignment::const_iterator evidenceIt = aEvidence.begin(); evidenceIt != aEvidence.end(); ++evidenceIt) {
                VarIdType varId = evidenceIt->first;
                VarType value = evidenceIt->second;

                DomainIntervalMap & intervalMap = mConstraintDomainIntervals[varId];
                
                // Find the interval matching the given value
                DomainIntervalMap::iterator intervalIt = intervalMap.lower_bound(DomainInterval(value, value + 1));
                if (intervalIt != intervalMap.end()) {
                        assert(intervalIt->first.lowerBound <= value);
                        intervalIt->second += aProbability;
                }

                mTotalProbabilities[varId] += aProbability;
        }
}

double IntervalJoinGraphMessage::evalAssignment(Assignment &a, const CSPProblem * aProblem) {
        assert(mNormalized);

        unsigned int probabilityDenominator = 1;
        std::vector<DomainInterval> intervalAssignment;
        for (Scope::const_iterator scIt = mScope.begin(); scIt != mScope.end(); ++scIt) {
                // Find interval for the given variable, which matches the assigned value
                // If there is no such interval, return 0
                VarIdType varId = *scIt;
                VarType value = a[varId];

                DomainIntervalMap & varDomainIntervals = mDomainIntervals[varId];
                DomainIntervalMap::iterator domIt = varDomainIntervals.lower_bound(DomainInterval(value, value + 1));

                if (domIt == varDomainIntervals.end()) {
                        // There is no interval containing the assigned value
                        return 0.0;
                } else if (domIt->first.lowerBound > value && domIt == varDomainIntervals.begin()) {
                        // Again, no interval containing the assigned value
                        return 0.0;
                } else if (domIt->first.lowerBound > value) {
                        --domIt;
                        if (domIt->first.upperBound <= value)
                                return 0.0;
                } else {
                        // Otherwise we have found the interval matching the given value
                        intervalAssignment.push_back(domIt->first);
                        probabilityDenominator *= aProblem->getNumValuesInDomainRange(varId, domIt->first.lowerBound, domIt->first.upperBound);
                }
        }
        
        std::map<std::vector<DomainInterval>, double>::const_iterator pIt = mProbabilityTable.find(intervalAssignment);

        assert(pIt != mProbabilityTable.end());

        // Divide the probability by the number of values in the domain in each interval
        assert(probabilityDenominator > 0);
        return pIt->second / probabilityDenominator;
}

void IntervalJoinGraphMessage::normalize() {

        if (mTotalProbability > 0.0) {
                for (std::map<std::vector<DomainInterval>, double>::iterator pIt = mProbabilityTable.begin();
                                pIt != mProbabilityTable.end(); ++pIt) {
                        pIt->second = pIt->second / mTotalProbability;
                }
        } else {
                double uniformProbability = 1.0 / mProbabilityTable.size();

                for (std::map<std::vector<DomainInterval>, double>::iterator pIt = mProbabilityTable.begin();
                                pIt != mProbabilityTable.end(); ++pIt) {
                        pIt->second = uniformProbability;
                }
        }

        mTotalProbability = 1.0;
        mNormalized = true;
}

double IntervalJoinGraphMessage::KLDivergence(const IntervalJoinGraphMessage * aOldMessage) const {
        assert(aOldMessage);
        
        double divergence = 0.0;
        
        std::map<std::vector<DomainInterval>, double>::const_iterator pOldIt = aOldMessage->mProbabilityTable.begin();

        for (std::map<std::vector<DomainInterval>, double>::const_iterator pIt = mProbabilityTable.begin();
                        pIt != mProbabilityTable.end() && pOldIt != aOldMessage->mProbabilityTable.end();
                        ++pIt, ++pOldIt) {
                if (pOldIt->second == 0.0) {
                        divergence += JOIN_GRAPH_KL_DIVERGENCE_MAX;
                } else {
                        divergence += pIt->second * log(pIt->second / pOldIt->second);
                }
        }

        return divergence;
}

void IntervalJoinGraph::purgeMessages() {
        for (std::map<Scope, IntervalJoinGraphNode *>::iterator nodeIt = mNodes.begin();
                        nodeIt != mNodes.end(); ++nodeIt) {

                nodeIt->second->purgeMessages();
        }
}

void IntervalJoinGraph::orderNodes() {
        mOrdering.clear();

        for (std::map<Scope, IntervalJoinGraphNode*>::const_iterator nodeIt = mNodes.begin();
                        nodeIt != mNodes.end(); ++nodeIt) {

                mOrdering.push_back(nodeIt->first);
        }
}

void IntervalJoinGraph::iterativePropagation(CSPProblem * aProblem, Assignment & aEvidence, unsigned int aMaxIterations) {

        Scope evidenceScope;

        for (Assignment::iterator aIt = aEvidence.begin(); aIt != aEvidence.end(); ++aIt) {
                evidenceScope.insert(aIt->first);
        }


        unsigned int numIterations = 0;
        std::cout << "IJGP " << std::endl;

        while (numIterations < aMaxIterations) {

                std::cout << "IJGP iteration " << numIterations << std::endl;
                /*std::cout << ".";
                std::cout.flush();*/

                // Walk along the ordering of the clusters
                for (std::vector<Scope>::iterator nodeScopeIt = mOrdering.begin();
                                nodeScopeIt != mOrdering.end(); ++nodeScopeIt) {

                        IntervalJoinGraphNode * node = mNodes[*nodeScopeIt];
                        //std::cout << "Processing node " << scope_pprint(node->getScope()) << std::endl;

                        for (std::list<IntervalJoinGraphEdge *>::iterator edgeIt = node->mEdges.begin();
                                        edgeIt != node->mEdges.end(); ++edgeIt) {

                                
                                /*
                                std::cout << "Processing edge " << scope_pprint(node->getScope()) << " -> " <<
                                       scope_pprint((*edgeIt)->targetNode()->getScope()) << std::endl;*/
                               
                                // Create a new message
                                IntervalJoinGraphMessage * message = node->getMessage(aProblem, *edgeIt, aEvidence, evidenceScope);

                                //std::cout << message->pprint() << std::endl;

                                //std::cout << "IntervalJoinGraph::iterativePropagation | message " << message << std::endl;

                                // Send the message to target node
                                (*edgeIt)->targetNode()->setMessage(node, message);
                        }
                }

                // Now compute the KL-difference between old messages and new ones,
                // if it's small enough, we may quit the loop

                /*
                double kl_divergence;
                int kl_err = KLDivergence(kl_divergence);
*/
                /*
                if (kl_err >= 0)
                        std::cout << "KL divergence is " << kl_divergence << std::endl;
                */

                /*
                if (kl_err >= 0 && fabs(kl_divergence) < JOIN_GRAPH_KL_DIVERGENCE_MIN) {
                        //std::cout << "Breaking, KL divergence is " << kl_divergence << std::endl;
                        std::cout << ";";
                        break;
                }
                */


                ++numIterations;
        }

        std::cout << " ";
        std::cout << assignment_pprint(aEvidence);
        std::cout << std::endl;
}

int IntervalJoinGraph::KLDivergence(double & outDivergence) {
        outDivergence = 0.0;
        for (std::map<Scope, IntervalJoinGraphNode *>::const_iterator nodeIt = mNodes.begin();
                        nodeIt != mNodes.end(); ++nodeIt) {
                
                double nodeDivergence = 0.0;
                int err = nodeIt->second->KLDivergence(nodeDivergence);
                if (err < 0)
                        return JOIN_GRAPH_ERROR_KL_UNDEFINED;
                else
                        outDivergence += nodeDivergence;
        }

        if (!mNodes.empty())
                outDivergence = outDivergence / mNodes.size();

        return 0;
}

IntervalProbabilityDistribution IntervalJoinGraph::conditionalDistribution(CSPProblem * aProblem, 
                        Variable * aTargetVariable, const Assignment & aEvidence) {

        // Find a node whose scope contains target variable
        assert(aTargetVariable);
        
        for (std::map<Scope, IntervalJoinGraphNode *>::iterator nodeIt = mNodes.begin();
                        nodeIt != mNodes.end(); ++nodeIt) {

                if (nodeIt->first.find(aTargetVariable->getId()) != nodeIt->first.end()) {
                        // We have found the node
                        return nodeIt->second->conditionalDistribution(aProblem, aTargetVariable, aEvidence);
                }
        }

        assert(false); // We should find some node every time
}

std::string IntervalJoinGraphMessage::pprint() const {
        std::ostringstream out;

        out << "Message scope" << scope_pprint(mScope) << std::endl;
        out << "Domain intervals:" << std::endl;
        for (std::map<VarIdType, DomainIntervalMap>::const_iterator diIt = mDomainIntervals.begin();
                        diIt != mDomainIntervals.end(); ++diIt) {
                out << diIt->first << " ";
                out << interval_list_pprint(diIt->second);
        }

        out << "Interval probabilities" << std::endl;
        for (std::map<std::vector<DomainInterval>, double>::const_iterator probIt = mProbabilityTable.begin();
                        probIt != mProbabilityTable.end(); ++probIt) {

                for (std::vector<DomainInterval>::const_iterator intervalIt = probIt->first.begin();
                                intervalIt != probIt->first.end(); ++intervalIt) {

                        out << intervalIt->pprint() << ", ";
                }
                out << ": " << probIt->second << std::endl;
                out << "\n";
        }
        return out.str();
}
