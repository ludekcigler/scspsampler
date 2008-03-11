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

#ifndef GRAPH_H_
#define GRAPH_H_

#include <vector>
#include <map>

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

#endif // GRAPH_H_
