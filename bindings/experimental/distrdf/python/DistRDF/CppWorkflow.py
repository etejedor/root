# @author Enric Tejedor
# @date 2021-07

################################################################################
# Copyright (C) 1995-2021, Rene Brun and Fons Rademakers.                      #
# All rights reserved.                                                         #
#                                                                              #
# For the licensing terms see $ROOTSYS/LICENSE.                                #
# For the list of contributors see $ROOTSYS/README/CREDITS.                    #
################################################################################

import os
import ROOT

class CppWorkflow(object):
    '''
    Class that encapsulates the generation of the code of an RDataFrame workflow
    in C++, together with its compilation into a shared library and execution.

    This class is used by worker processes to execute in C++ the RDataFrame
    graph they receive. This is done for the sake of performance, since running
    the graph from Python necessarily relies on jitted code, which is less
    optimized and thus slower than a shared library compiled with ACLiC.

    Attributes:
    includes (list): include statements needed by the workflow.

    lambdas (list): lambda functions used by the workflow.

    lambda_id (int): counter used to generate ids for each defined lambda
        function.

    graph_nodes (list): statements that add nodes to the graph.

    node_id (int): counter used to generate ids for each graph node.

    res_ptr_id (int): counter used to generate ids for each result generated
        by graph actions.

    snapshots (dict): mapping between the Snapshot result index in the vector
        of workflow results and its modified output file name.

    _FUNCTION_NAME (string): name of the function that encapsulates the
        RDataFrame graph creation (internal)

    _FUNCTION_NAMESPACE (string): namespace of the function that encapsulates
        the RDataFrame graph creation (internal)
    '''

    _FUNCTION_NAME = 'RunGraph'
    _FUNCTION_NAMESPACE = '__distrdf_internal'

    def __init__(self):
        '''
        Initializes the attributes used in the generation of the workflow.
        '''

        self.includes = '''
#include "ROOT/RDataFrame.hxx"
#include "ROOT/RResultHandle.hxx"
'''

        self.lambdas = ''
        self.lambda_id = 0

        self.graph_nodes = ''
        self.node_id = 1  # 0 is the parent node we receive

        self.res_ptr_id = 0

        self.snapshots = {}

    def add_include(self, header):
        '''
        Adds a new include statement.

        Args:
        header (str): header to be included.
        '''

        self.includes += '\n#include "{}"'.format(header)

    def add_lambda(self, lambda_code):
        '''
        Adds a new lambda to be defined, which is needed by some operation
        in the workflow.

        Args:
        lambda_code (str): code of the lambda to be defined.
        '''

        self.lambdas += '\n  auto rdf_lambda{n} = {code};' \
                        .format(n=lambda_id, code=lambda_code)
        lambda_id += 1

    def add_node(self, operation, range_id, parent_id):
        '''
        Adds the corresponding statement to add a new node in the RDataFrame
        graph. What the statement returns depends on the type of the
        operation:
        - Transformation: the statement produces a new dataset node.
        - Action: the statement returns an RResultPtr, which is added to a
        vector of results to be returned at the end of the workflow
        generation function. The type of the elements of the vector is
        ROOT::RDF::RResultHandle, in order to ensure type erasure since
        there could be multiple booked actions with results of different
        types (e.g. integers, TH1D, etc.).

        Args:
        operation (Operation): object representing the operation to be added
            to the graph.
        range_id (int): id of the current range. Needed to assign a name to a
            partial Snapshot output file.
        parent_id (int): id of the parent node in this workflow. Used to
            connect the new node to be added with its parent.
        '''

        self.graph_nodes += '\n  '

        # Snapshot needs special treatment
        if operation.name == 'Snapshot':
            self._handle_snapshot(operation, range_id)

        # Return type depends on the type of the operation
        new_node_id = self.node_id
        if operation.is_transformation():
            self.graph_nodes += 'auto rdf{}'.format(new_node_id)
        else:  # action or instant action
            self.graph_nodes += 'auto res_ptr{}'.format(self.res_ptr_id)

        self.graph_nodes += ' = rdf{n}.{op_name}(' \
                            .format(n=parent_id, op_name=operation.name)
        self._process_args(operation)
        self.graph_nodes += ');'

        if operation.is_transformation():
            self.node_id += 1
        else:
            # Action or instant action.
            # The result is stored in the vector to be returned
            self.graph_nodes += '\n  result_ptrs.emplace_back(res_ptr{});' \
                                .format(self.res_ptr_id)
            self.res_ptr_id += 1

        return new_node_id

    def _handle_snapshot(self, operation, range_id):
        '''
        Does two extra settings needed for Snapshot nodes:
        - Modifies the output file name to be of the form `filename_rangeid`,
        since it is a partial snapshot for a given range.
        - Stores the index of the returned vector<RResultHandle> in which the
        result of this Snapshot is stored, together with the modified file
        path.

        Args:
        operation (Operation): object representing the operation to be added
            to the graph.

        range_id (int): id of the current range. Needed to assign a name to a
            partial Snapshot output file.
        '''

        # Modify file name
        filename = operation.args[1].partition('.root')[0]
        path_with_range = '{filename}_{rangeid}.root'. \
                          format(filename=filename, rangeid=range_id)
        operation.args[1] = path_with_range

        # Store Snapshot result index -> path
        self.snapshots[self.res_ptr_id] = path_with_range

    def _process_args(self, operation):
        '''
        Processes the arguments of an operation node.

        Args:
        operation (Operation): object representing the operation to be added
            to the graph.
        '''

        # Argument type conversion
        for narg, arg in enumerate(operation.args):
            if (narg > 0):
                self.graph_nodes += ', '

            if isinstance(arg, str):
                self.graph_nodes += '"{}"'.format(arg)
            elif isinstance(arg, tuple):
                self.graph_nodes += '{'
                for nelem, elem in enumerate(arg):
                    if nelem > 0:
                        self.graph_nodes += ','
                    if isinstance(elem, str):
                        self.graph_nodes += '"{}"'.format(elem)
                    else:
                        self.graph_nodes += '{}'.format(elem)
                self.graph_nodes += '}'

    def execute(self, rdf):
        '''
        Compiles the workflow generation code and executes it.

        Args:
        rdf (ROOT::RDF::RNode): object that represents the dataset on
            which to execute the workflow.
        '''

        self._compile()
        return self._run_function(rdf)

    def _compile(self):
        '''
        Generates the workflow code C++ file and compiles it with ACLiC
        into a shared library. The library is also loaded as part of the
        `TSystem::CompileMacro` call.

        The name of the generated C++ file contains both a hash of its
        code and the ID of the process that created it. This is done to
        prevent clashes between multiple (non-sandboxed) worker processes
        that try to write to the same file concurrently.
        '''

        code = self._get_code()
        cpp_file_name = 'rdfworkflow_{code_hash}_{pid}.cpp' \
                        .format(code_hash=hash(code), pid=os.getpid())

        if not os.path.isfile(cpp_file_name):
            with open(cpp_file_name, 'w') as f:
                f.write(code)

            if not ROOT.gSystem.CompileMacro(cpp_file_name, 'O'):
                raise RuntimeError(
                    "Error compiling the RDataFrame workflow file: {}".
                    format(cpp_file_name))

    def _run_function(self, rdf):
        '''
        Runs the workflow generation function.

        Args:
        rdf (ROOT::RDF::RNode): object that represents the dataset on
            which to execute the workflow.
        '''

        ns = getattr(ROOT, self._FUNCTION_NAMESPACE)
        func = getattr(ns, self._FUNCTION_NAME)

        # Running the function triggers the event loop, so release the GIL
        func.__release_gil__ = True
        results = func(rdf)

        # Replace the RResultHandle of each Snapshot by its modified output
        # path, since the latter is what we actually need in the reducer
        for i, path in self.snapshots.items():
            results[i] = path

        return results

    def __repr__(self):
        '''
        Returns a string representation of the workflow that
        corresponds to its code.
        '''

        return self._get_code()

    def _get_code(self):
        '''
        Composes the workflow generation code from the different attributes
        of this class. The resulting code contains a function that will be
        called to generate the RDataFrame graph and that returns a vector
        of results from the graph actions.
        '''

        code = '''
{includes}

namespace {namespace} {{

std::vector<ROOT::RDF::RResultHandle> {func_name}(ROOT::RDF::RNode &rdf0)
{{
  std::vector<ROOT::RDF::RResultHandle> result_ptrs;

{lambdas}

{nodes}

  res_ptr0.GetValue(); // to trigger the event loop

  return result_ptrs;
}}

}}
'''.format(func_name=self._FUNCTION_NAME, namespace=self._FUNCTION_NAMESPACE,
           includes=self.includes, lambdas=self.lambdas, nodes=self.graph_nodes)

        return code

