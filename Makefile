
build: third_party/node-unittest/README.rst lib/pmars.node

third_party/node-unittest/README.rst:
	git submodule update --init --recursive

lib/pmars.node:
	cd third_party/pmars-0.9.2/src && node-waf configure build
	cp third_party/pmars-0.9.2/src/build-waf/default/pmars.node lib/pmars.node
