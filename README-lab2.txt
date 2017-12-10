
=== Getting Started ===
	To use the placer, please first run (not source) `./enter-environment` to enter a subshell with a setup environment (needed to set LD_LIBRARY_PATH, also adds executables to PATH)

	The code should be built by running `make` in the top level or src folders
		note: on older versions of make, builds (in particular from a clean directory) may fail 1-2 times before working.

	The placer can then be run like this:
		anaplace --circuit data/anaplace/cct1.txt

	The graphics displays the solution to the current problem, with next iteration's weights and connections. Then when it accepts a solution, the final fully-legal snapped solution is displayed.
		Note: graphics is running in a separate thread, which results in that the GUI isn't unresponsive while the program is doing work
		Note: most labels (and anchors) are not visible from a zoom level that contains the entire device, and one should zoom in to see them (also, the scroll wheel works for zooming)
		colours/shapes:
			green circles -> non-fixed blocks
			blue circles -> anchors
			red text on block -> fixed block located here
			black lines -> connections between fixed and movable blocks
			grey lines -> connections between movable blocks and anchors

=== Command Line Arguments ===
	(can be printed by running [exe] --help)

	Program Options:
	  -h [ --help ]         print help message
	  --circuit arg         The circuit to use (required)

	Meta Options:
	  --graphics            Enable graphics (off by default)
