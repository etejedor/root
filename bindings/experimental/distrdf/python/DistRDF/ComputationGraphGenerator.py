## @author Vincenzo Eduardo Padulano
#  @author Enric Tejedor
#  @date 2021-02

################################################################################
# Copyright (C) 1995-2021, Rene Brun and Fons Rademakers.                      #
# All rights reserved.                                                         #
#                                                                              #
# For the licensing terms see $ROOTSYS/LICENSE.                                #
# For the list of contributors see $ROOTSYS/README/CREDITS.                    #
################################################################################

import ROOT
from .CppWorkflow import CppWorkflow

class ComputationGraphGenerator(object):
    """
    Class that generates a callable to parse a DistRDF graph.

    Attributes:
        headnode: Head node of a DistRDF graph.
    """

    def __init__(self, headnode):
        """
        Creates a new `ComputationGraphGenerator`.

        Args:
            dataframe: DistRDF DataFrame object.
        """
        self.headnode = headnode

    def get_action_nodes(self, node_py=None):
        """
        Recurses through DistRDF graph and collects the DistRDF node objects.

        Args:
            node_py (optional): The current state's DistRDF node. If `None`, it
                takes the value of `self.headnode`.

        Returns:
            list: A list of the action nodes of the graph in DFS order, which
            coincides with the order of execution in the callable function.
        """
        return_nodes = []

        if not node_py:
            # In the first recursive state, just set the
            # current DistRDF node as the head node
            node_py = self.headnode
        else:
            if (node_py.operation.is_action() or
                    node_py.operation.is_instant_action()):
                # Collect all action nodes in order to return them
                return_nodes.append(node_py)

        for n in node_py.children:
            # Recurse through children and collect them
            prev_nodes = self.get_action_nodes(n)

            # Attach the children nodes
            return_nodes.extend(prev_nodes)

        return return_nodes

    def get_callable(self):
        """
        Converts a given graph into a callable and returns the same.

        Returns:
            function: The callable that takes in a PyROOT RDataFrame object
            and executes all operations from the DistRDF graph
            on it, recursively.
        """
        # Prune the graph to check user references
        self.headnode.graph_prune()

        def generate_computation_graph(cpp_headnode, range_id):
            """
            The callable that traverses the DistRDF graph nodes, generates the
            code to create the same graph in C++, compiles it and runs it.
            If the workflow contains an instant action, the event loop will
            be already triggered by this function.

            Args:
                cpp_headnode (ROOT.RDF.RNode): The main RDataFrame node.
                range_id (int): Id of the current range. Needed to assign a name to a
                    partial Snapshot output file.

            Returns:
                list: A list of results coming from the actions in the graph.
            """
            py_headnode = self.headnode
            cpp_workflow = CppWorkflow()
            parent_idx = 0

            # Recurse over children nodes and get store their results
            for py_node in py_headnode.children:
                explore_graph(py_node, cpp_workflow, range_id, parent_idx) 

            print(cpp_workflow)
            results = cpp_workflow.execute(ROOT.RDF.AsRNode(cpp_headnode))
            print("RESULTS", results)
            for elem in results:
                print(elem)

            return results

        def explore_graph(py_node, cpp_workflow, range_id, parent_idx):
            """
            Recursively traverses the DistRDF graph nodes in DFS order and,
            for each of them, adds a new node to the C++ workflow.

            Args:
                py_node (Node): Object that contains the information to add the
                    corresponding node to the C++ workflow.
                cpp_workflow (CppWorkflow): Object that encapsulates the creation
                    of the C++ workflow graph.
                range_id (int): Id of the current range. Needed to assign a name to a
                    partial Snapshot output file.
                parent_idx (int): Index of the parent node in the C++ workflow.
            """
            node_idx = cpp_workflow.add_node(py_node.operation, range_id, parent_idx)

            for child_node in py_node.children:
                explore_graph(child_node, cpp_workflow, range_id, node_idx)

        return generate_computation_graph
