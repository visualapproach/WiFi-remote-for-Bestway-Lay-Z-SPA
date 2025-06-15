import re
import shlex

class TemplateError(Exception):
    """Custom exception for template rendering errors."""
    pass

class Template:
    """
    A simple block-based templating engine in Python.

    This engine supports:
    - Variable substitution: {{ variable_name }}
    - Block definitions: {% block block_name %} ... {% endblock %}
    - Template inheritance: {% extends "parent_template.html" %}
    - Function execution: {% run function_name(arg1, arg2, ...) %}

    The engine works by parsing the template content to identify tags.
    When rendering, it can extend a parent template, call registered Python
    functions, and finally substitute all variables with values from a given
    context dictionary.
    """
    def __init__(self, template_content, template_dir="."):
        """
        Initializes the Template object.

        Args:
            template_content (str): The string content of the template.
            template_dir (str): The directory where templates are located.
                                This is used for handling the {% extends %} tag.
        """
        self.template_content = template_content
        self.template_dir = template_dir
        self.blocks = self._extract_blocks(self.template_content)

    def _extract_blocks(self, content):
        """
        Extracts all named blocks from the template content.

        Args:
            content (str): The template content to parse.

        Returns:
            dict: A dictionary where keys are block names and values are the
                  content of the blocks.
        """
        blocks = {}
        block_regex = re.compile(r'{% block (\w+) %}(.*?){% endblock %}', re.DOTALL)
        for match in block_regex.finditer(content):
            block_name = match.group(1)
            block_content = match.group(2)
            blocks[block_name] = block_content
        return blocks

    def render(self, context=None, callables=None):
        """
        Renders the template with a given context and callable functions.

        Args:
            context (dict, optional): A dictionary of context variables. Defaults to {}.
            callables (dict, optional): A dictionary of functions that can be called
                                        from the template using {% run %}. The key
                                        is the name used in the template. Defaults to {}.

        Returns:
            str: The rendered HTML or text.

        Raises:
            TemplateError: If there is an issue with rendering.
        """
        context = context or {}
        callables = callables or {}

        content = self._handle_inheritance(self.template_content, context, callables)
        content = self._replace_blocks(content)
        content = self._handle_run_tags(content, context, callables)
        content = self._substitute_variables(content, context)

        return content

    def _handle_inheritance(self, content, context, callables):
        """
        Handles the {% extends "..." %} tag if it exists.
        """
        extends_regex = re.compile(r'{% extends "(.+?)" %}')
        match = extends_regex.match(content)

        if not match:
            return content

        parent_template_name = match.group(1)
        try:
            with open(f"{self.template_dir}/{parent_template_name}", "r") as f:
                parent_content = f.read()
            
            parent_template = Template(parent_content, self.template_dir)
            
            parent_template.blocks.update(self.blocks)
            self.blocks = parent_template.blocks

            return parent_template.template_content

        except FileNotFoundError:
            raise TemplateError(f"Parent template '{parent_template_name}' not found.")

    def _replace_blocks(self, content):
        """
        Replaces all block placeholders with their final content.
        """
        block_regex = re.compile(r'{% block (\w+) %}(.*?){% endblock %}', re.DOTALL)
        
        def replace_func(match):
            block_name = match.group(1)
            return self.blocks.get(block_name, match.group(2))

        return block_regex.sub(replace_func, content)

    def _substitute_variables(self, content, context):
        """
        Substitutes all {{ variable }} placeholders with values from the context.
        """
        variable_regex = re.compile(r'{{ (.+?) }}')

        def replace_func(match):
            var_name = match.group(1).strip()
            value = context.get(var_name)
            if value is None:
                raise TemplateError(f"Variable '{var_name}' not found in context.")
            return str(value)

        return variable_regex.sub(replace_func, content)

    def _parse_run_args(self, arg_string, context):
        """
        Parses the argument string from a run tag.
        Arguments can be string literals (single or double quoted), numbers, or context variables.
        This uses a regex to split arguments by commas, while respecting quotes.
        """
        if not arg_string.strip():
            return []
            
        arg_regex = re.compile(r"""
            (
              "[^"]*"
              |
              '[^']*'
              |
              [^,]+  
            )
        """, re.VERBOSE)
        
        raw_args = [match.group(0).strip() for match in arg_regex.finditer(arg_string)]

        parsed_args = []
        for arg in raw_args:
            if (arg.startswith("'") and arg.endswith("'")) or \
               (arg.startswith('"') and arg.endswith('"')):
                parsed_args.append(arg[1:-1])
                continue

            try:
                parsed_args.append(int(arg))
                continue
            except ValueError:
                pass
            try:
                parsed_args.append(float(arg))
                continue
            except ValueError:
                pass

            if arg in context:
                parsed_args.append(context[arg])
            else:
                raise TemplateError(f"Argument '{arg}' is not a valid literal or context variable.")
        
        return parsed_args

    def _handle_run_tags(self, content, context, callables):
        """
        Finds and executes all {% run ... %} tags.
        """
        run_regex = re.compile(r'{% run (\w+)\((.*?)\) %}')

        def replace_func(match):
            func_name = match.group(1)
            arg_string = match.group(2)

            if func_name not in callables:
                raise TemplateError(f"Function '{func_name}' is not registered as a callable.")
            
            func = callables[func_name]
            args = self._parse_run_args(arg_string, context)
            
            try:
                return str(func(*args))
            except Exception as e:
                raise TemplateError(f"Error executing function '{func_name}': {e}")

        return run_regex.sub(replace_func, content)
