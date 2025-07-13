#!/usr/bin/python3


import os
import shutil

from webInterface.template_engine import Template, TemplateError

def copy_static(target_dir, source_dir="./static", only_files=True):
    if not os.path.exists(source_dir):
        print("No source dir to copy static files from")

    for root, dirs, files in os.walk(source_dir):
        if only_files:
            for file in files:
                src_file = os.path.join(root, file)
                shutil.copy2(src_file, target_dir)
        else:
            for dir in dirs:
                src_dir = os.path.join(root, dir)
                dst_dir = os.path.join(target_dir, os.path.relpath(src_dir, source_dir))
                if not os.path.exists(dst_dir):
                    os.makedirs(dst_dir)
            for file in files:
                src_file = os.path.join(root, file)
                dst_file = os.path.join(target_dir, os.path.relpath(src_file, source_dir))
                shutil.copy2(src_file, dst_file)



def build_templates(output_dir, base_dir):
    context_data = {
        
    }

    def embed_scripts(*scripts):
        script_tags = []
        for script in scripts:
            script_path = os.path.join(base_dir, "js", f"{script}.js")
            if os.path.exists(script_path):
                with open(script_path, 'r') as f:
                    script_content = f.read()
                script_tag = f"<script type=\"text/javascript\">\n{script_content}</script>"
                script_tags.append(script_tag)
            else:
                print(f"Warning: Script file {script_path} does not exist.")
        return "\n".join(script_tags)

    callable_functions = {
        'embed_scripts': embed_scripts
    }

    pages_dir = os.path.join(base_dir, "pages")

    for root, dirs, files in os.walk(pages_dir):
        for file in files:
            file_path = os.path.join(root, file)
            with open(file_path, 'r') as f:
                child_template_content = f.read()

                child_template = Template(child_template_content, template_dir=root)

                callable_functions = {
                    'embed_scripts': embed_scripts
                }

                def use_component(component_name, *component_arguments):
                    component_path = os.path.join(base_dir, "component", component_name+".html")
                    with open(component_path, 'r') as component_file:
                        component_file_content = component_file.read()

                        component_template = Template(component_file_content, template_dir=os.path.join(base_dir, "component"))
                        print(f"component {component_name} component_arguments: {component_arguments}")

                        component_context_data = context_data

                        for index, component_argument in enumerate(component_arguments):
                            component_context_data[f'_argument_{index}'] = component_argument

                        try:
                            final_output = component_template.render(context_data, callables=callable_functions)
                            return final_output
                            # print(f"Rendered and saved: {output_file_path}")
                        except TemplateError as e:
                            print(f"Error rendering component template {component_path}: {e}")
                    
                    return f"ERROR: use_component {component_name} {component_arguments}"
                    

                callable_functions['use_component'] = use_component

                try:
                    final_output = child_template.render(context_data, callables=callable_functions)
                    output_file_path = os.path.join(output_dir, file)
                    with open(output_file_path, 'w') as output_file:
                        output_file.write(final_output)
                    print(f"Rendered and saved: {output_file_path}")
                except TemplateError as e:
                    print(f"Error rendering template {file_path}: {e}")


def build_web_interface(target_dir="../data", base_dir="./"):
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)

    copy_static(target_dir, source_dir = os.path.join(base_dir, "static"))

    build_templates(target_dir, base_dir)
