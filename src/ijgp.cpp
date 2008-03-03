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
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>

#include "csp.h"
#include "ijgp.h"
#include "utils.h"

unsigned int JoinGraph::MAX_PROPAGATION_ITERATIONS = 10;

JoinGraphNode::~JoinGraphNode() {
        purgeMessages();

        for (std::map<JoinGraphNode *, JoinGraphMessage *>::iterator msgIt = mMessages.begin();
                        msgIt != mMessages.end(); ++msgIt) {

                delete msgIt->second;
        }

        for (std::map<JoinGraphNode *, JoinGraphMessage *>::iterator msgIt = mOldMessages.begin();
                        msgIt != mOldMessages.end(); ++msgIt) {

                delete msgIt->second;
        }
}

JoinGraph::~JoinGraph() {
        for (std::map<Scope, JoinGraphNode *>::iterator nodeIt = mNodes.begin();
                        nodeIt != mNodes.end(); ++nodeIt) {
                delete nodeIt->second;
        }
}

Graph * Graph::createCSPPrimalGraph(CSPProblem * p) {
        std::map<VarIdType, Vertex *> *vertices = new std::map<VarIdType, Vertex *>();

        for (VariableMap::const_iterator varIt = p->getVariables()->begin(); varIt != p->getVariables()->end(); ++varIt) {
                // Create vertex for each variable
                Vertex * v = new Vertex(varIt->first);
                (*vertices)[varIt->first] = v;
        }

        for (ConstraintList::const_iterator ctrIt = p->getConstraints()->begin(); ctrIt != p->getConstraints()->end(); ++ctrIt) {
                // Add an edge for each constraint
                Scope s = (*ctrIt)->getScope();

                for (Scope::iterator scIt1 = s.begin(); scIt1 != s.end(); ++scIt1) {
                        Vertex * v1 = (*vertices)[*scIt1];
                        Scope::iterator scIt2 = scIt1;
                        ++scIt2;
                        for (; scIt2 != s.end(); ++scIt2) {
                                Vertex * v2 = (*vertices)[*scIt2];
                                v1->addNeighbour(v2);
                                v2->addNeighbour(v1);
                        }
                }
        }

        Graph * g = new Graph(vertices);

        return g;
}

std::vector<VarIdType> Graph::minInducedWidthOrdering() {

        std::map<VarIdType, Vertex *> vertices;

        // Copy vertices 
        for (VertexDict::iterator vIt = this->mVertices->begin(); vIt != this->mVertices->end(); ++vIt) {
                Vertex *v = new Vertex(*(vIt->second));
                vertices[v->getId()] = v;
        }

        // Fix the neighbouring links
        for (std::map<VarIdType, Vertex *>::iterator vIt = vertices.begin(); vIt != vertices.end(); ++vIt) {
                for (std::vector<Vertex *>::iterator nIt = vIt->second->neighbours.begin();
                                nIt != vIt->second->neighbours.end(); ++nIt) {

                        *nIt = vertices[(*nIt)->getId()];
                }
        }

        std::vector<VarIdType> ordering(vertices.size());
        for (int i = ordering.size() - 1; i >= 0; --i) {
                // Find a node with the smallest degree

                size_t minDegree = vertices.size() + 1;
                Vertex * minVertex;
                for (std::map<VarIdType, Vertex *>::iterator vIt = vertices.begin(); vIt != vertices.end(); ++vIt) {
                        if (vIt->second->neighbours.size() < minDegree) {
                                minDegree = vIt->second->neighbours.size();
                                minVertex = vIt->second;
                        }
                }

                // Store the found vertex
                ordering[i] = minVertex->getId();

                // Connect the neighbours of the found vertex
                for (std::vector<Vertex *>::iterator nIt1 = minVertex->neighbours.begin();
                                nIt1 != minVertex->neighbours.end(); ++nIt1) {

                        for (std::vector<Vertex *>::iterator nIt2 = (*nIt1)->neighbours.begin();
                                        nIt2 != (*nIt1)->neighbours.end(); ++nIt2) {

                                        if (*nIt2 == minVertex)
                                                continue;

                                        (*nIt1)->addNeighbour(*nIt2);
                        }

                        (*nIt1)->removeNeighbour(minVertex);
                }

                vertices.erase(minVertex->getId());

                delete minVertex;
        }

        return ordering;
}

JoinGraph * JoinGraph::createJoinGraph(CSPProblem * aProblem, unsigned int aMaxBucketSize) {
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

                        scope_pprint(*mbIt1);
                }
                std::cout << std::endl;
        }
        */
#endif

        JoinGraph * joinGraph = new JoinGraph();
        // Create join-graph node for every 
        for (size_t i = 0; i < miniBuckets.size(); ++i) {
                for (std::vector<Scope>::iterator mbIt = miniBuckets[i].begin(); mbIt != miniBuckets[i].end(); ++mbIt) {
                        JoinGraphNode * node = new JoinGraphNode(*mbIt);
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

                        // If there is an edge from this mini-bucket to some other bucket created before,
                        // add it now
                        std::map<Scope, Scope>::iterator outsideArcIt = outsideBucketArcs.find(*mbIt);
                        if (outsideArcIt != outsideBucketArcs.end()) {
                                Scope edgeScope;
                                std::set_intersection(outsideArcIt->first.begin(), outsideArcIt->first.end(),
                                                outsideArcIt->second.begin(), outsideArcIt->second.end(),
                                                std::inserter(edgeScope, edgeScope.begin()));

                                JoinGraphEdge * edge1 = new JoinGraphEdge(joinGraph->mNodes[outsideArcIt->first],
                                                edgeScope);

                                joinGraph->mNodes[outsideArcIt->second]->addEdge(edge1);

                                JoinGraphEdge * edge2 = new JoinGraphEdge(joinGraph->mNodes[outsideArcIt->second],
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

                                JoinGraphEdge * edge1 = new JoinGraphEdge(joinGraph->mNodes[*mbIt2], edgeScope);
                                joinGraph->mNodes[*mbIt1]->addEdge(edge1);

                                JoinGraphEdge * edge2 = new JoinGraphEdge(joinGraph->mNodes[*mbIt1], edgeScope);
                                joinGraph->mNodes[*mbIt2]->addEdge(edge2);
                        }
                }
        }

        joinGraph->orderNodes();
        return joinGraph;
}

void JoinGraph::pprint() {
        for (std::map<Scope, JoinGraphNode *>::iterator nodeIt = this->mNodes.begin();
                        nodeIt != this->mNodes.end(); ++nodeIt) {
                scope_pprint(nodeIt->first);
                std::cout << std::endl;

                for (std::list<JoinGraphEdge *>::iterator edgeIt = nodeIt->second->mEdges.begin();
                                edgeIt != nodeIt->second->mEdges.end(); ++edgeIt) {
                        std::cout << "\t";
                        scope_pprint((*edgeIt)->getScope());
                        std::cout << " -> ";
                        scope_pprint((*edgeIt)->targetNode()->getScope());
                        std::cout << std::endl;
                }
        }
}

void JoinGraphNode::setMessage(JoinGraphNode * aNodeFrom, JoinGraphMessage * aMessage) {
        
        /*
        std::cout << "JoinGraphNode::setMessage from node ";
        scope_pprint(aNodeFrom->getScope());
        std::cout << " to node ";
        scope_pprint(getScope());
        std::cout << ", mesg " << aMessage << "\n";
        */
        std::map<JoinGraphNode *, JoinGraphMessage *>::iterator oldIt, newIt;

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

JoinGraphMessage * JoinGraphNode::getMessage(const CSPProblem * aProblem, JoinGraphEdge * aEdge,
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

        JoinGraphMessage * message = new JoinGraphMessage(messageScope);

        // Assign values to all unassigned variables (those in the scope of the message
        // and those that need to be marginalized out)

        std::vector<VarType> messageScopeValues;

        _computeMessage(message, aProblem, messageScope, marginalizedScope, messageScopeValues, 
                        partialAssignment, aEdge->targetNode());

        message->normalize();

        return message;
}

void JoinGraphNode::purgeMessages() {
        while (!mOldMessages.empty()) {
                delete (mOldMessages.begin())->second;
                mOldMessages.erase(mOldMessages.begin());
        }

        while (!mMessages.empty()) {
                delete (mMessages.begin())->second;
                mMessages.erase(mMessages.begin());
        }
}

int JoinGraphNode::KLDivergence(double & outDivergence) {
        outDivergence = 0.0;

        if (mMessages.empty() || mOldMessages.empty())
                return JOIN_GRAPH_ERROR_KL_UNDEFINED;

        for (std::map<JoinGraphNode *, JoinGraphMessage *>::iterator msgIt = mMessages.begin();
                        msgIt != mMessages.end(); ++msgIt) {

                JoinGraphMessage * oldMessage = mOldMessages[msgIt->first];

                outDivergence += msgIt->second->KLDivergence(oldMessage);
        }

        outDivergence = outDivergence / mMessages.size();

        return 0;
}


ProbabilityDistribution JoinGraphNode::conditionalDistribution(CSPProblem * aProblem, 
                        Variable * aTargetVariable, const Assignment & aEvidence) {

        // We should make sure that the variable is in the current scope
        // If not, we have errors up in node selection
        assert(aTargetVariable);

        VarIdType targetVarId = aTargetVariable->getId();
        assert(mScope.find(targetVarId) != mScope.end());

        ProbabilityDistribution result;
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

        const Domain * d = aTargetVariable->getDomain();

        Assignment::iterator targetVarIt = partialEvidence.find(targetVarId);
        if (targetVarIt != partialEvidence.end()) {
                // The target variable is already in the evidence, therefore we create
                // only a simple distribution (0, 0, ..., 0, 1, 0, ..., 0)
                for (Domain::iterator domIt = d->begin(); domIt != d->end(); ++domIt) {
                        result[*domIt] = 0.0;
                }
                result[targetVarIt->second] = 1.0;

                assert(false); // This should never happen in the debugging, though...
        } else {

                for (Domain::iterator domIt = d->begin(); domIt != d->end(); ++domIt) {
                        partialEvidence[targetVarId] = *domIt;

                        result[*domIt] = _marginalizeOut(aProblem, marginalizedScope, partialEvidence);
                }
        }

        return result;
}

void JoinGraphNode::_computeMessage(JoinGraphMessage * aMessage, const CSPProblem * aProblem,
                Scope & aMessageScope, Scope & aMarginalizedScope, std::vector<VarType> & aMessageScopeValues,
                Assignment & aAssignment, const JoinGraphNode * const aExcludeNode) {

        // If all variables from the message scope have been assigned, we can start marginalizing out
        // the remaining variables
        if (aMessageScope.empty()) {
                double probability = _marginalizeOut(aProblem, aMarginalizedScope, aAssignment, aExcludeNode);
                aMessage->setProbability(aMessageScopeValues, probability);

                //std::cout << "JoinGraphNode::_computeMessage | aMessage " << aMessage << std::endl;
        } else {
                VarIdType assignedVariable = *(aMessageScope.begin());
                aMessageScope.erase(aMessageScope.begin());

                const Domain * d = aProblem->getVariableById(assignedVariable)->getDomain();

                for (Domain::const_iterator domIt = d->begin(); domIt != d->end(); ++domIt) {
                        // Assign selected value from the variable domain to the variable
                        aAssignment[assignedVariable] = *domIt;
                        aMessageScopeValues.push_back(*domIt);

                        _computeMessage(aMessage, aProblem, aMessageScope, aMarginalizedScope, 
                                        aMessageScopeValues, aAssignment, aExcludeNode);

                        // Restore previous assignment
                        aMessageScopeValues.pop_back();
                        aAssignment.erase(assignedVariable);
                }

                aMessageScope.insert(aMessageScope.begin(), assignedVariable);
        }
}

double JoinGraphNode::_marginalizeOut(const CSPProblem * aProblem, Scope & aMarginalizedScope,
                Assignment & aAssignment, const JoinGraphNode * const aExcludeNode) {

        // Sum over all possible values of the given variable
        if (aMarginalizedScope.empty()) {
                // We have finally assigned all the variables needed to evaluate the functions
                return _evalAssignment(aAssignment, aExcludeNode);
        } else {
                // Otherwise we marginalize out the first variable in the scope
                double sum = 0.0;
                VarIdType marginalizedVariable = *(aMarginalizedScope.begin());
                aMarginalizedScope.erase(aMarginalizedScope.begin());

                const Domain * d = aProblem->getVariableById(marginalizedVariable)->getDomain();

                for (Domain::const_iterator domIt = d->begin(); domIt != d->end(); ++domIt) {
                        // Assign selected value from the variable domain to the variable
                        aAssignment[marginalizedVariable] = *domIt;

                        sum += _marginalizeOut(aProblem, aMarginalizedScope, aAssignment, aExcludeNode);

                        // Restore previous assignment
                        aAssignment.erase(marginalizedVariable);
                }

                aMarginalizedScope.insert(aMarginalizedScope.begin(), marginalizedVariable);

                return sum;
        }
}

double JoinGraphNode::_evalAssignment(Assignment & aAssignment, const JoinGraphNode * const aExcludeNode) {
        double result = 1.0;
        for (ConstraintList::iterator ctrIt = mConstraints.begin(); ctrIt != mConstraints.end(); ++ctrIt) {
                result = result * (**ctrIt)(aAssignment);
        }

        //assignment_pprint(aAssignment);
        //std::cout << "JoinGraphNode::_evalAssignment | mMessages.size(): " << mMessages.size() << std::endl;

        for (std::map<JoinGraphNode *, JoinGraphMessage *>::iterator msgIt = mMessages.begin();
                        msgIt != mMessages.end(); ++msgIt) {
                if (msgIt->first == aExcludeNode)
                        continue;

                result = result * (*(msgIt->second))(aAssignment);
        }

        return result;
}

double JoinGraphMessage::operator()(Assignment &a) const {
        assert(mNormalized);

        std::vector<VarType> scopeAssignment;
        for (Scope::const_iterator scIt = mScope.begin(); scIt != mScope.end(); ++scIt) {
                scopeAssignment.push_back(a[*scIt]);
        }
        
        std::map<std::vector<VarType>, double>::const_iterator pIt = mProbabilityTable.find(scopeAssignment);

        assert(pIt != mProbabilityTable.end());

        return pIt->second;
}

void JoinGraphMessage::normalize() {

        if (mTotalProbability > 0.0) {
                for (std::map<std::vector<VarType>, double>::iterator pIt = mProbabilityTable.begin();
                                pIt != mProbabilityTable.end(); ++pIt) {
                        pIt->second = pIt->second / mTotalProbability;
                }
        } else {
                double uniformProbability = 1.0 / mProbabilityTable.size();

                for (std::map<std::vector<VarType>, double>::iterator pIt = mProbabilityTable.begin();
                                pIt != mProbabilityTable.end(); ++pIt) {
                        pIt->second = uniformProbability;
                }
        }

        mTotalProbability = 1.0;
        mNormalized = true;
}

double JoinGraphMessage::KLDivergence(const JoinGraphMessage * aOldMessage) const {
        assert(aOldMessage);
        
        double divergence = 0.0;
        
        std::map<std::vector<VarType>, double>::const_iterator pOldIt = aOldMessage->mProbabilityTable.begin();

        for (std::map<std::vector<VarType>, double>::const_iterator pIt = mProbabilityTable.begin();
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

void JoinGraph::purgeMessages() {
        for (std::map<Scope, JoinGraphNode *>::iterator nodeIt = mNodes.begin();
                        nodeIt != mNodes.end(); ++nodeIt) {

                nodeIt->second->purgeMessages();
        }
}

void JoinGraph::orderNodes() {
        mOrdering.clear();

        for (std::map<Scope, JoinGraphNode*>::const_iterator nodeIt = mNodes.begin();
                        nodeIt != mNodes.end(); ++nodeIt) {

                mOrdering.push_back(nodeIt->first);
        }
}

void JoinGraph::iterativePropagation(CSPProblem * aProblem, Assignment & aEvidence) {

        Scope evidenceScope;

        for (Assignment::iterator aIt = aEvidence.begin(); aIt != aEvidence.end(); ++aIt) {
                evidenceScope.insert(aIt->first);
        }


        unsigned int numIterations = 0;
        std::cout << "IJGP ";

        while (numIterations < MAX_PROPAGATION_ITERATIONS) {

                std::cout << ".";
                std::cout.flush();

                // Walk along the ordering of the clusters
                for (std::vector<Scope>::iterator nodeScopeIt = mOrdering.begin();
                                nodeScopeIt != mOrdering.end(); ++nodeScopeIt) {

                        JoinGraphNode * node = mNodes[*nodeScopeIt];

                        for (std::list<JoinGraphEdge *>::iterator edgeIt = node->mEdges.begin();
                                        edgeIt != node->mEdges.end(); ++edgeIt) {

                                /*
                                std::cout << "Processing edge ";
                                scope_pprint(node->getScope());
                                std::cout << " -> ";
                                scope_pprint((*edgeIt)->targetNode()->getScope());
                                std::cout << std::endl;
                                */
                                // Create a new message
                                JoinGraphMessage * message = node->getMessage(aProblem, *edgeIt, aEvidence, evidenceScope);

                                //std::cout << "JoinGraph::iterativePropagation | message " << message << std::endl;

                                // Send the message to target node
                                (*edgeIt)->targetNode()->setMessage(node, message);
                        }
                }

                // Now compute the KL-difference between old messages and new ones,
                // if it's small enough, we may quit the loop

                double kl_divergence;
                int kl_err = KLDivergence(kl_divergence);

                /*
                if (kl_err >= 0)
                        std::cout << "KL divergence is " << kl_divergence << std::endl;
                */

                if (kl_err >= 0 && fabs(kl_divergence) < JOIN_GRAPH_KL_DIVERGENCE_MIN) {
                        std::cout << "Breaking, KL divergence is " << kl_divergence << std::endl;
                        break;
                }


                ++numIterations;
        }

        assignment_pprint(aEvidence);
        std::cout << std::endl;
}

int JoinGraph::KLDivergence(double & outDivergence) {
        outDivergence = 0.0;
        for (JoinGraphNodeMap::const_iterator nodeIt = mNodes.begin();
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

ProbabilityDistribution JoinGraph::conditionalDistribution(CSPProblem * aProblem, 
                        Variable * aTargetVariable, const Assignment & aEvidence) {

        // Find a node whose scope contains target variable
        assert(aTargetVariable);
        
        for (std::map<Scope, JoinGraphNode *>::iterator nodeIt = mNodes.begin();
                        nodeIt != mNodes.end(); ++nodeIt) {

                if (nodeIt->first.find(aTargetVariable->getId()) != nodeIt->first.end()) {
                        // We have found the node
                        return nodeIt->second->conditionalDistribution(aProblem, aTargetVariable, aEvidence);
                }
        }

        assert(false); // We should find some node every time
}
