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

#ifndef IJGP_H_
#define IJGP_H_

#include <assert.h>
#include <list>
#include <set>
#include <vector>
#include <algorithm>

#include "csp.h"

class Vertex;

class Edge;

class Vertex {
public:
        Vertex(VarIdType id): mId(id) {};

        VarIdType getId() const {
                return mId;
        }

        void addNeighbour(Vertex *v) {
                assert(v);

                if (find(neighbours.begin(), neighbours.end(), v) == neighbours.end() &&
                                v->getId() != mId) {
                        neighbours.push_back(v);
                }
        }

        void removeNeighbour(Vertex *v) {
                assert(v);

                std::vector<Vertex *>::iterator f = find(neighbours.begin(), neighbours.end(), v);
                if (f != neighbours.end()) {
                        neighbours.erase(f);
                }
        }

        std::vector<Vertex *> neighbours;
protected:
        VarIdType mId;
};

typedef std::map<VarIdType, Vertex *> VertexDict;

class Graph {
public:
        Graph(VertexDict * vertices): mVertices(vertices) {};

        /**
         * Create primal graph of the CSP
         */
        static Graph * createCSPPrimalGraph(CSPProblem * p);

        /**
         * Heuristically computes min-induced-width ordering of a given graph
         */
        std::vector<VarIdType> minInducedWidthOrdering();

private:
        VertexDict *mVertices;
};

class JoinGraphEdge;

class JoinGraphMessage;

class JoinGraphNode {
public:
        JoinGraphNode(const Scope & s): mScope(s) {};

        ~JoinGraphNode();

        Scope getScope() const {
                return mScope;
        };

        void addEdge(JoinGraphEdge * e) {
                assert(e);
                mEdges.push_back(e);
        };

        void addConstraint(Constraint * aConstraint) {
                mConstraints.push_back(aConstraint);
        };

        void setMessage(JoinGraphNode * aNodeFrom, JoinGraphMessage * aMessage);

        JoinGraphMessage * getMessage(const CSPProblem * aProblem, JoinGraphEdge * aEdge,
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
        ProbabilityDistribution conditionalDistribution(CSPProblem * aProblem, 
                        Variable * aTargetVariable, const Assignment & aEvidence);
private:
        friend class JoinGraph;

        void _computeMessage(JoinGraphMessage * aMessage, const CSPProblem * aProblem,
                Scope & aMessageScope, Scope & aMarginalizedScope, std::vector<VarType> & aMessageScopeValues,
                Assignment & aAssignment, const JoinGraphNode * const aExcludeNode = 0);

        double _marginalizeOut(const CSPProblem * aProblem, Scope & aMarginalizedScope,
                Assignment & aAssignment, const JoinGraphNode * const aExcludeNode = 0);

        double _evalAssignment(Assignment & aAssignment, const JoinGraphNode * const aExcludeNode = 0);

        /**
         * List of edges the node has
         */
        std::list<JoinGraphEdge *> mEdges;

        Scope mScope;

        ConstraintList mConstraints;

        std::map<JoinGraphNode *, JoinGraphMessage *> mMessages;

        /**
         * Messages from previous iteration, used for deciding whether we should
         * stop iterating or not
         */
        std::map<JoinGraphNode *, JoinGraphMessage *> mOldMessages;
};

class JoinGraphEdge {
public:
        JoinGraphEdge(JoinGraphNode * aTargetNode, const Scope & aScope):
                mTargetNode(aTargetNode), mScope(aScope) {
                
                assert(aTargetNode);
        };

        Scope getScope() const {
                return mScope;
        };

        JoinGraphNode * targetNode() {
                return mTargetNode;
        };
private:
        JoinGraphNode *mTargetNode;
        Scope mScope;
};

typedef std::map<Scope, JoinGraphNode *> JoinGraphNodeMap;

class JoinGraph {
public:
        ~JoinGraph();

        /**
         * Performs iterative join-graph propagation on this graph
         * given evidence
         */
        void iterativePropagation(CSPProblem * aProblem, Assignment & aEvidence);

        /**
         * Cleans up messages from previous computations
         */
        void purgeMessages();

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
        static JoinGraph * createJoinGraph(CSPProblem * aProblem, unsigned int aMaxBucketSize);

        /**
         * Pretty-prints the join graph
         */
        void pprint();

        /**
         * Compute conditional probability distribution P(X_j|e)
         */
        ProbabilityDistribution conditionalDistribution(CSPProblem * aProblem, 
                        Variable * aTargetVariable, const Assignment & aEvidence);
private:
        /**
         * An (arbitrary) ordering of the graph nodes
         */
        std::vector<Scope> mOrdering;

        std::map<Scope, JoinGraphNode *> mNodes;
};

class JoinGraphMessage: public Constraint {
public:
        JoinGraphMessage(const Scope & aScope):
                mScope(aScope), mTotalProbability(0.0), mNormalized(false) {};

        void setProbability(const std::vector<VarType> & aScopeValues, double aProbability) {
                assert(!mNormalized); // After normalization, no probability should be adjusted

                mProbabilityTable[aScopeValues] = aProbability;
                mTotalProbability += aProbability;
        }

        virtual Scope getScope() const {
                return mScope;
        }

        virtual double operator()(Assignment &a) const;

        void normalize();

        double KLDivergence(const JoinGraphMessage * aOldMessage) const;
private:
        std::map<std::vector<VarType>, double> mProbabilityTable;
        Scope mScope;
        double mTotalProbability;
        bool mNormalized;
};

extern const unsigned int IJGP_MAX_ITERATIONS;

const int JOIN_GRAPH_ERROR_KL_UNDEFINED = -1;
const double JOIN_GRAPH_KL_DIVERGENCE_MAX = 1e10;
const double JOIN_GRAPH_KL_DIVERGENCE_MIN = 1e-2;

const double KL_EPSILON = 1e-10;

#endif // IJGP_H_

