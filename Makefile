
build: lib/pmars.node


lib/pmars.node:
	cd third_party/pmars-0.9.2/src && node-waf configure build
	cp third_party/pmars-0.9.2/src/build-waf/default/pmars.node lib/pmars.node
