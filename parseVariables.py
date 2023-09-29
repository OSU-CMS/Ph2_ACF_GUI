import ast

class variableParser:
    def parse(self, file_path):
        variable_dict = {}
        
        with open(file_path, 'r') as module_file:
            module_code = module_file.read()

        module_ast = ast.parse(module_code)

        for node in ast.walk(module_ast):
            if isinstance(node, ast.Assign):
                for target in node.targets:
                    if isinstance(target, ast.Name):
                        variable_name = target.id
                        variable_value = self.parse_constant(node.value)
                        if variable_value is not None:
                            variable_dict[variable_name] = variable_value

        return variable_dict

    def parse_constant(self, node):
        try:
            return ast.literal_eval(node)
        except (SyntaxError, ValueError):
            return None

    @staticmethod
    def restoreOriginalType(input_dict):
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
