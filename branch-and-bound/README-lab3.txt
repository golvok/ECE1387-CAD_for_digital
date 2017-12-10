
=== Getting Started ===
	To use or compile the SAT-MAX code, please 
		activate devtool set #4 by running `csh_scl enable devtoolset-4`
		cd into branch-and-bound
		run (not source) `./enter-environment` to enter a subshell with a setup environment (adds executables to PATH)

	The code should be built by running `make` in the same branch-and-bound folder
		NOTE: on older versions of make, builds (in particular from a clean directory) may fail 2-3 times before working.

	The code can then be run like this:
		sat-maxer -f data/2.cnf

	The graphics does not auto-refresh, though this can be simulated by panning (middle-mouse)

=== Command Line Arguments ===
	(can be printed by running [exe] --help)

	Program Options:
	  -h [ --help ]                         print help message
	  -f [ --problem-file ] arg             The file with the SAT-MAX problem to 
	                                        solve
	  -r [ --variable-order ] arg (=GBD,MCF,F)
	                                        Comma or (single-token) space separated
	                                        list of sort orders, interpreted as a 
	                                        hierarchy with top level first. Valid 
	                                        strings: FILE, F, GROUPED_BY_DISJUNCTIO
	                                        N, GBD, MOST_COMMON_FIRST, MCF, 
	                                        ALL_BUT_ONE_IN_DISJUNCTION, ABOID, 
	                                        RANDOM
	  --incremental                         Use incremental mode for computing 
	                                        costs

	Meta Options:
	  --graphics                            Enable graphics
	  --debug                               Turn on the most common debugging 
	                                        output options
