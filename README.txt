
=== Getting Started ===
	To use the placer, please first run (not source) `./enter-environment` to enter a subshell with a setup environment (needed to set LD_LIBRARY_PATH, also adds executables to PATH)

	The code should be built by running `make` in the top level or src folders
		note: on older versions of make, builds (in particular from a clean directory) may fail 1-2 times before working.

	The placer can then be run like this:
		anaplace --circuit data/anaplace/cct1.txt

=== Command Line Arguments ===
	(can be printed by running [exe] --help)

	Program Options:
	  -h [ --help ]         print help message
	  --circuit arg         The circuit to use (required)

	Meta Options:
	  --graphics            Enable graphics (off by default)
