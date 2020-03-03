import inspect
import importlib
import keyword

def get_functions(module):
	isfunc = lambda x: inspect.isfunction(x) or inspect.ismethod(x) or inspect.isbuiltin(x)
	return [x[0] for x in inspect.getmembers(module, isfunc)]

def get_classes(module):
	isclass = lambda x: inspect.isclass(x)
	return [x[0] for x in inspect.getmembers(module, isclass)]

def generate_set(name, items, line_begin = '\t', line_size = 80):
	print('const std::unordered_set<std::string> {} = {{'.format(name))
	curline = ''
	for item in ('"{}", '.format(item) for item in items):
		if len(curline) + len(item) > line_size:
			print(line_begin + curline)
			curline = ''
		curline += item
	if curline:
		print(line_begin + curline)
	print('};\n')

def generate_find_func(*sets):
	print('const char* get_keyword_group(const std::string& name)')
	print('{')
	for set in sets:
		print('\tif ({}.find(name) != {}.end())'.format(set[0], set[0]))
		print('\t\treturn "{}";'.format(set[1]))
	print('\treturn nullptr;')
	print('}')



if __name__ == "__main__":
	import importlib
	import os
	import sys

	modules = ["builtins"]
	outdir = '.'
	if len(sys.argv) == 1:
		print("Usage: {} [module1 [module2 [...]]] [--outdir] [--path]".format(sys.argv[0]))
		print("\tSpecify the output directory for the Keywords.hh and Keywords.cc files with --outdir")
		print("\tSpecify additional search paths for modules with --path; search paths should be semicolon separated")
		exit(-1)
	for arg in sys.argv[1:]:
		if arg.startswith("--outdir="):
			outdir = arg[len("--outdir="):]
		elif arg.startswith("--path="):
			sys.path.extend(arg[len("--path="):].split(';'))
		else:
			modules.append(arg)

	outdir = os.path.abspath(outdir)
	for i in range(len(modules)):
		try:
			modules[i] = importlib.import_module(modules[i])
		except ImportError:
			print("Could not import module {}".format(modules[i]))
			exit(-1)

	functions = []
	classes = []

	for module in modules:
		functions.extend(get_functions(module))
		classes.extend(get_classes(module))

	print("Generating in directory '{}'".format(outdir))

	# Generate header file
	sys.stdout = open(os.path.join(outdir, 'Keywords.hh'), 'w+')
	print("#pragma once\n\n#include <string>\n")
	print("// Returns the group name belongs to, or nullptr if not found")
	print("const char* get_keyword_group(const std::string& name);")

	#Generate code file
	sys.stdout = open(os.path.join(outdir, 'Keywords.cc'), 'w+')

	print('#include <unordered_set>')
	print('#include "Keywords.hh"\n\n')

	generate_set("functions", functions)
	generate_set("classes", classes)
	generate_set("keywords", keyword.kwlist)

	generate_find_func(
		("functions", "function"),
		("classes", "class"),
		("keywords", "keyword")
	)
	
	sys.stdout = sys.__stdout__
	print("Generated Keywords.hh and Keywords.cc")
