import ast

'''
The class variableParser parses variables from a python file, specifically settings and returns a dictionary.
Is capable of restoring data types of variables inside any input dictionary. 
'''

class variableParser:
    def parse(self, file_path):
        variable_dict = {}
        
        with open(file_path, 'r') as module_file:
            module_code = module_file.read()        #reads in the file

        module_ast = ast.parse(module_code)         #parses instances

        for node in ast.walk(module_ast):
            if isinstance(node, ast.Assign):
                for target in node.targets:
                    if isinstance(target, ast.Name):
                        variable_name = target.id
                        variable_value = self.parse_constant(node.value)
                        variable_dict[variable_name] = variable_value

        return variable_dict

    def parse_constant(self, node):
        try:
            return ast.literal_eval(node)
        except (SyntaxError, ValueError):
            return None

    @staticmethod
    def restoreOriginalType(input_dict):        #restores the datatype of a given dictionary
        converted_dict = {}
        for key, value in input_dict.items():
            converted_value = value
            if isinstance(value, str):
                try:
                    converted_value = ast.literal_eval(value)
                except (SyntaxError, ValueError):
                    pass
            converted_dict[key] = converted_value

        return converted_dict
